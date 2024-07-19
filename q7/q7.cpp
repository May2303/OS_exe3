#include <iostream>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <thread>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>

using namespace std;

// Global graph data
int n = 0, m = 0;                 // Number of vertices (n) and edges (m)
vector<pair<int, int>> edges;     // store edges as pairs of integers (u, v)
mutex graphMutex;                 // Mutex for safe access to graph data

// Depth-first search (DFS) to populate finishStack for Kosaraju's algorithm
void dfs1(int v, const vector<list<int>>& adj, vector<bool>& visited, stack<int>& finishStack) {
    visited[v] = true;  // Mark the current vertex as visited
    // Traverse all adjacent vertices recursively
    for (int u : adj[v]) {
        if (!visited[u]) {
            dfs1(u, adj, visited, finishStack);  // Recursive call for unvisited adjacent vertex
        }
    }
    finishStack.push(v);  // Push the vertex onto the stack once all its adjacent vertices are visited
}

// DFS for the reverse graph to build strongly connected components (SCCs)
void dfs2(int v, const vector<list<int>>& revAdj, vector<bool>& visited, vector<int>& component) {
    visited[v] = true;  // Mark the current vertex as visited
    component.push_back(v);  // Add the vertex to the current component
    // Traverse all adjacent vertices in the reverse graph recursively
    for (int u : revAdj[v]) {
        if (!visited[u]) {
            dfs2(u, revAdj, visited, component);  // Recursive call for unvisited adjacent vertex
        }
    }
}

// Kosaraju's algorithm to find all SCCs in a directed graph
vector<vector<int>> kosaraju(int n, const vector<pair<int, int>>& edges) {
    vector<list<int>> adj(n + 1), revAdj(n + 1);  // Adjacency list and reverse adjacency list for the graph

    // Constructing the adjacency list and reverse adjacency list from edges
    for (const auto& edge : edges) {
        adj[edge.first].push_back(edge.second);    // Original graph adjacency list
        revAdj[edge.second].push_back(edge.first); // Reverse graph adjacency list
    }

    stack<int> finishStack;             // Stack to store vertices based on finish times
    vector<bool> visited(n + 1, false); // Array to track visited vertices

    // Step 1: Perform DFS on the original graph and populate finishStack
    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            dfs1(i, adj, visited, finishStack);  // Perform DFS on unvisited vertices
        }
    }

    fill(visited.begin(), visited.end(), false);  // Reset visited array for the second DFS
    vector<vector<int>> sccs;  // Vector to store all strongly connected components (SCCs)

    // Step 2: Process vertices in order of decreasing finish time (from finishStack)
    while (!finishStack.empty()) {
        int v = finishStack.top();  // Get the vertex with the highest finish time
        finishStack.pop();          // Remove the vertex from the stack

        if (!visited[v]) {
            vector<int> component;  // Vector to store the current SCC
            dfs2(v, revAdj, visited, component);  // Perform DFS on the reverse graph to find SCC
            sccs.push_back(component);  // Add the SCC to the list of SCCs found
        }
    }

    return sccs;  // Return all strongly connected components (SCCs) found
}

// Function to handle client commands and interact with the graph
void handleClient(int clientSocket) {
    char buffer[1024] = {0};  // Buffer to store incoming data from client
    string command;           // String to store parsed command from client

    while (true) {
        // Read data from client socket
        int valread = read(clientSocket, buffer, 1024);
        if (valread <= 0) {
            cout << "Client disconnected" << endl;
            close(clientSocket);  // Close socket on client disconnect
            return;
        }

        // Process command received from client
        buffer[valread] = '\0';  // Ensure buffer is null-terminated
        command = buffer;        // Convert buffer to string
        stringstream ss(command); // Use stringstream for parsing

        ss >> command;  // Extract the command from stringstream

        if (command == "Newgraph") {
            // Command to update the graph structure
            int new_n, new_m;
            ss >> new_n >> new_m;  // Read new number of vertices and edges

            vector<pair<int, int>> new_edges;  // Vector to store new edges
            for (int i = 0; i < new_m; ++i) {
                int u, v;
                ss >> u >> v;  // Read each edge (u, v) from client
                new_edges.emplace_back(u, v);  // Add edge to new_edges vector
            }

            // Update global graph data in a locked scope to ensure thread safety
            {
               // Create a lock_guard object with graphMutex to automatically acquire the mutex
                lock_guard<mutex> lock(graphMutex);
                // Update number of vertices (n), number of edges (m), and edges vector
                n = new_n;           // Update the number of vertices to the new value provided by the client
                m = new_m;           // Update the number of edges to the new value provided by the client
                edges = new_edges;   // Replace the current edges vector with the new set of edges provided by the client
            }  // The lock_guard object goes out of scope here and releases the lock automatically

            string response = "Graph updated\n";
            send(clientSocket, response.c_str(), response.length(), 0);  // Send response to client
     
        } else if (command == "Kosaraju") {
            // Command to find all strongly connected components (SCCs) in the current graph
            vector<vector<int>> sccs;
            {
                lock_guard<mutex> lock(graphMutex);
                sccs = kosaraju(n, edges);  // Invoke Kosaraju's algorithm to find SCCs
            }

            // Prepare response containing all SCCs found
            stringstream response;
            response << "scc:\n";
            for (const auto& scc : sccs) {
                for (int v : scc) {
                    response << v << " ";  // Append each vertex in SCC to response
                }
                response << "\n";  // Add newline after each SCC
            }

            // Send response to client
            send(clientSocket, response.str().c_str(), response.str().length(), 0);
        
        } else if (command == "Newedge") {
            // Command to add a new edge to the graph
            int u, v;
            ss >> u >> v;  // Read vertices (u, v) to be added as an edge

            // Update global graph data within a locked scope to ensure thread safety
            {
                lock_guard<mutex> lock(graphMutex);
                edges.emplace_back(u, v);  // Add edge (u, v) to edges vector
            }

            string response = "Edge added\n";
            send(clientSocket, response.c_str(), response.length(), 0);  // Send response to client
        } else if (command == "Removeedge") {
            // Command to remove an edge from the graph
            int u, v;
            ss >> u >> v;  // Read vertices (u, v) of edge to be removed

            // Update global graph data within a locked scope to ensure thread safety
            {
                lock_guard<mutex> lock(graphMutex);
                // Search for the edge (u, v) and remove it if found
                auto it = find(edges.begin(), edges.end(), make_pair(u, v));
                if (it != edges.end()) {
                    edges.erase(it);  // Erase edge from edges vector
                    string response = "Edge removed\n";
                    send(clientSocket, response.c_str(), response.length(), 0);  // Send response to client
                } else {
                    string response = "Edge not found\n";
                    send(clientSocket, response.c_str(), response.length(), 0);  // Send response to client
                }
            }
        } else {
            // Invalid command received from client
            string error = "Invalid command\n";
            send(clientSocket, error.c_str(), error.length(), 0);  // Send error response to client
        }
    }
}

// Main function to set up server socket and handle incoming client connections
int main() {
    int serverSocket, newSocket;    // Variables for server and client sockets
    struct sockaddr_in address;     // Structure for server address information
    int opt = 1;                    // Option for socket setsockopt function
    int addrlen = sizeof(address);  // Length of address structure

    // Creating socket file descriptor for server
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");  // Print error message if socket creation fails
        exit(EXIT_FAILURE);       // Exit the program with failure status
    }

    // Set socket options to reuse address and port
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");   // Print error message if setsockopt fails
        exit(EXIT_FAILURE);     // Exit the program with failure status
    }

    // Configure server address settings
    address.sin_family = AF_INET;            // Address family IPv4
    address.sin_addr.s_addr = INADDR_ANY;    // Accept connections from any IP address
    address.sin_port = htons(9034);          // Port number in network byte order

    // Bind socket to specified IP address and port number
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");  // Print error message if bind fails
        exit(EXIT_FAILURE);     // Exit the program with failure status
    }

    // Listen for incoming connections with a queue size of 3
    if (listen(serverSocket, 3) < 0) {
        perror("listen");   // Print error message if listen fails
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    cout << "Server started on port 9034" << endl;  // Server startup message

    // Accept incoming client connections and spawn threads to handle each client
    while (true) {
        // Accept a new connection and create a socket for communication with client
        if ((newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");  // Print error message if accept fails
            exit(EXIT_FAILURE); // Exit the program with failure status
        }

        cout << "New client connected" << endl;  // Client connection message

        // Create a new thread to handle client communication independently
        thread clientThread(handleClient, newSocket);
        clientThread.detach();  // Detach thread to allow it to run independently
    }

    return 0;  
}
