#include <iostream>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <thread>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <mutex>
#include "reactor.hpp"

using namespace std;

// Global graph data
int n = 0, m = 0;               // Number of vertices and edges
vector<pair<int, int>> edges;   // Edges of the graph
mutex graphMutex;               // Mutex for ensuring thread safety

// Depth-first search function to fill finishing order in finishStack
void dfs1(int v, const vector<list<int>> &adj, vector<bool> &visited, stack<int> &finishStack)
{
    visited[v] = true;
    for (int u : adj[v])
    {
        if (!visited[u])
        {
            dfs1(u, adj, visited, finishStack); // Recursive call for unvisited neighbors
        }
    }
    finishStack.push(v); // Push vertex to stack after all neighbors are visited
}

// Depth-first search function to find strongly connected components (SCCs)
void dfs2(int v, const vector<list<int>> &revAdj, vector<bool> &visited, vector<int> &component)
{
    visited[v] = true;
    component.push_back(v); // Add vertex to current SCC
    for (int u : revAdj[v])
    {
        if (!visited[u])
        {
            dfs2(u, revAdj, visited, component); // Recursive call for unvisited neighbors
        }
    }
}

// Kosaraju's algorithm to find all SCCs in the graph
vector<vector<int>> kosaraju(int n, const vector<pair<int, int>> &edges)
{
    vector<list<int>> adj(n + 1), revAdj(n + 1); // Adjacency list and reverse adjacency list

    // Construct adjacency list and reverse adjacency list from edges
    for (const auto &edge : edges)
    {
        adj[edge.first].push_back(edge.second);   // Original graph
        revAdj[edge.second].push_back(edge.first); // Transposed graph
    }

    stack<int> finishStack;         // Stack to store finishing order of vertices
    vector<bool> visited(n + 1, false); // Visited array to track visited vertices

    // Perform first DFS to fill finishStack with vertices in finishing order
    for (int i = 1; i <= n; ++i)
    {
        if (!visited[i])
        {
            dfs1(i, adj, visited, finishStack);
        }
    }

    fill(visited.begin(), visited.end(), false); // Reset visited array
    vector<vector<int>> sccs; // Vector of vectors to store SCCs

    // Process vertices in order of decreasing finish times (top of finishStack)
    while (!finishStack.empty())
    {
        int v = finishStack.top();
        finishStack.pop();

        if (!visited[v])
        {
            vector<int> component; // Vector to store current SCC
            dfs2(v, revAdj, visited, component); // Perform DFS on transposed graph
            sccs.push_back(component); // Add current SCC to list of SCCs
        }
    }

    return sccs; // Return all SCCs found in the graph
}

// Function to handle client connections and process commands
void *handleClient(int clientSocket)
{
    char buffer[1024] = {0}; // Buffer to read client data
    string command; // String to store parsed command

    while (true)
    {
        int valread = read(clientSocket, buffer, 1024); // Read data from client
        if (valread <= 0)
        {
            cout << "Client disconnected" << endl;
            close(clientSocket); // Close client socket on disconnect
            return nullptr; // Exit thread
        }

        // Process command received from client
        buffer[valread] = '\0'; // Null-terminate buffer
        command = buffer; // Convert buffer to string
        stringstream ss(command); // String stream for parsing

        ss >> command; // Extract first token as command

        if (command == "Newgraph")
        {
            int new_n, new_m;
            ss >> new_n >> new_m; // Read new number of vertices and edges

            vector<pair<int, int>> new_edges; // Vector to store new edges
            for (int i = 0; i < new_m; ++i)
            {
                int u, v;
                ss >> u >> v; // Read each edge (u, v)
                new_edges.emplace_back(u, v); // Add edge to new_edges vector
            }

            // Update global graph data atomically
            {
                lock_guard<mutex> lock(graphMutex); // Lock mutex for thread safety
                n = new_n; // Update number of vertices
                m = new_m; // Update number of edges
                edges = new_edges; // Update edges vector
            }

            string response = "Graph updated\n"; // Prepare response
            send(clientSocket, response.c_str(), response.length(), 0); // Send response to client
        }
        else if (command == "Kosaraju")
        {
            vector<vector<int>> sccs; // Vector to store SCCs

            // Compute SCCs using Kosaraju's algorithm
            {
                lock_guard<mutex> lock(graphMutex); // Lock mutex for thread safety
                sccs = kosaraju(n, edges); // Compute SCCs for current graph state
            }

            // Prepare and send response with SCCs to client
            stringstream response;
            response << "scc:\n";
            for (const auto &scc : sccs)
            {
                for (int v : scc)
                {
                    response << v << " "; // Append vertex to response
                }
                response << "\n"; // Newline after each SCC
            }
            send(clientSocket, response.str().c_str(), response.str().length(), 0); // Send response
        }
        else if (command == "Newedge")
        {
            int u, v;
            ss >> u >> v; // Read vertices u and v

            // Add new edge (u, v) to graph
            {
                lock_guard<mutex> lock(graphMutex); // Lock mutex for thread safety
                edges.emplace_back(u, v); // Add edge to edges vector
            }

            string response = "Edge added\n"; // Prepare response
            send(clientSocket, response.c_str(), response.length(), 0); // Send response to client
        }
        else if (command == "Removeedge")
        {
            int u, v;
            ss >> u >> v; // Read vertices u and v

            // Remove specified edge (u, v) from graph
            {
                lock_guard<mutex> lock(graphMutex); // Lock mutex for thread safety
                auto it = find(edges.begin(), edges.end(), make_pair(u, v)); // Find edge in edges vector
                if (it != edges.end())
                {
                    edges.erase(it); // Erase edge if found
                    string response = "Edge removed\n"; // Prepare response
                    send(clientSocket, response.c_str(), response.length(), 0); // Send response to client
                }
                else
                {
                    string response = "Edge not found\n"; // Prepare response
                    send(clientSocket, response.c_str(), response.length(), 0); // Send response to client
                }
            }
        }
        else
        {
            // Invalid command received
            string error = "Invalid command\n"; // Prepare error response
            send(clientSocket, error.c_str(), error.length(), 0); // Send error response to client
        }
    }

    return nullptr; // Never reached, but included for completeness
}

// Main function to run the server
int main()
{
    int serverSocket, newSocket; // Server and client sockets
    struct sockaddr_in address; // Address structure
    int opt = 1; // Option variable
    int addrlen = sizeof(address); // Address length

    // Creating socket file descriptor
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed"); // Print error if socket creation fails
        exit(EXIT_FAILURE); // Exit program
    }

    // Forcefully attaching socket to the port 9034
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt"); // Print error if setsockopt fails
        exit(EXIT_FAILURE); // Exit program
    }

    address.sin_family = AF_INET; // Set address family
    address.sin_addr.s_addr = INADDR_ANY; // Set IP address to any interface
    address.sin_port = htons(9034); // Set port number, converting to network byte order

    // Binding the socket to the port 9034
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed"); // Print error if bind fails
        exit(EXIT_FAILURE); // Exit program
    }

    // Listening for incoming connections
    if (listen(serverSocket, 3) < 0)
    {
        perror("listen"); // Print error if listen fails
        exit(EXIT_FAILURE); // Exit program
    }

    cout << "Server started on port 9034" << endl; // Inform that server is running

    Reactor *reactor = Reactor::startReactor(); // Start Reactor for handling client connections

    while (true)
    {
        // Accepting a new connection
        if ((newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept"); // Print error if accept fails
            exit(EXIT_FAILURE); // Exit program
        }

        cout << "New client connected" << endl; // Inform that a new client connected

        reactor->startProactor(newSocket, handleClient); // Start a proactor thread to handle client requests
    }

    reactor->stopReactor(); // Stop Reactor (not reached in current implementation)
    delete reactor; // Delete Reactor instance

    return 0; 
}
