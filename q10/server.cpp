#include <iostream>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <mutex>
#include <pthread.h>


using namespace std;

// Global variables for graph data
mutex graphMutex;               // Mutex for thread-safe access to graph data
int n = 0, m = 0;               // Number of vertices and edges
vector<pair<int, int>> edges;   // Vector to store graph edges

// Depth-first search function to fill finishing order in finishStack
void dfs1(int v, const vector<list<int>>& adj, vector<bool>& visited, stack<int>& finishStack) {
    visited[v] = true;
    for (int u : adj[v]) {
        if (!visited[u]) {
            dfs1(u, adj, visited, finishStack); // Recursive call for unvisited neighbors
        }
    }
    finishStack.push(v); // Push vertex to stack after all neighbors are visited
}

// Depth-first search function to find strongly connected components (SCCs)
void dfs2(int v, const vector<list<int>>& revAdj, vector<bool>& visited, vector<int>& component) {
    visited[v] = true;
    component.push_back(v); // Add vertex to current SCC
    for (int u : revAdj[v]) {
        if (!visited[u]) {
            dfs2(u, revAdj, visited, component); // Recursive call for unvisited neighbors
        }
    }
}

// Kosaraju's algorithm to find all SCCs in the graph
vector<vector<int>> kosaraju(int n, const vector<pair<int, int>>& edges) {
    vector<list<int>> adj(n + 1), revAdj(n + 1); // Adjacency list and reverse adjacency list

    // Construct adjacency list and reverse adjacency list from edges
    for (const auto& edge : edges) {
        adj[edge.first].push_back(edge.second);   // Original graph
        revAdj[edge.second].push_back(edge.first); // Transposed graph
    }

    stack<int> finishStack;         // Stack to store finishing order of vertices
    vector<bool> visited(n + 1, false); // Visited array to track visited vertices

    // Perform first DFS to fill finishStack with vertices in finishing order
    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            dfs1(i, adj, visited, finishStack);
        }
    }

    fill(visited.begin(), visited.end(), false); // Reset visited array
    vector<vector<int>> sccs; // Vector of vectors to store SCCs

    // Process vertices in order of decreasing finish times (top of finishStack)
    while (!finishStack.empty()) {
        int v = finishStack.top();
        finishStack.pop();

        if (!visited[v]) {
            vector<int> component; // Vector to store current SCC
            dfs2(v, revAdj, visited, component); // Perform DFS on transposed graph
            sccs.push_back(component); // Add current SCC to list of SCCs
        }
    }

    // Check if at least 50% of vertices are in the same SCC
    int maxComponentSize = 0;
    for (const auto& scc : sccs) {
        if (scc.size() > maxComponentSize) {
            maxComponentSize = scc.size();
        }
    }
    bool atLeastHalfInSameSCC = (maxComponentSize >= n / 2);

    // Print appropriate message if the condition changes
    static bool prevAtLeastHalfInSameSCC = false;
    if (prevAtLeastHalfInSameSCC && !atLeastHalfInSameSCC) {
        cout << "At least 50% of the graph no longer belongs to the same SCC" << endl;
    } else if (!prevAtLeastHalfInSameSCC && atLeastHalfInSameSCC) {
        cout << "At least 50% of the graph belongs to the same SCC" << endl;
    }

    // Update the previous state
    prevAtLeastHalfInSameSCC = atLeastHalfInSameSCC;

    return sccs; // Return all SCCs found in the graph
}

// Function executed by each client thread
void* clientThread(void* arg) {
    int sockfd = *((int*)arg); // Extract socket file descriptor from argument
    char buffer[1024] = {0};   // Buffer to store incoming data
    string command;            // String to store parsed command

    while (true) {
        int valread = read(sockfd, buffer, 1024); // Read data from client
        if (valread <= 0) {
            cout << "Client disconnected" << endl; // Print message on client disconnect
            close(sockfd); // Close socket
            return nullptr; // Exit thread
        }

        buffer[valread] = '\0'; // Null-terminate buffer
        command = buffer; // Convert buffer to string
        stringstream ss(command); // String stream for parsing

        ss >> command; // Extract first token as command

        // Process commands received from client
        if (command == "Newgraph") {
            {
                lock_guard<mutex> lock(graphMutex); // Lock mutex for thread-safe access
                ss >> n >> m; // Read new number of vertices and edges
                edges.clear(); // Clear current edges vector
                edges.resize(m); // Resize edges vector

                // Read and populate edges vector
                for (int i = 0; i < m; ++i) {
                    int u, v;
                    ss >> u >> v;
                    edges[i] = {u, v}; // Store edge (u, v)
                }
            }

            string response = "Graph updated\n"; // Prepare response
            send(sockfd, response.c_str(), response.length(), 0); // Send response to client
        } else if (command == "Kosaraju") {
            vector<vector<int>> sccs; // Vector to store SCCs

            // Compute SCCs using Kosaraju's algorithm
            {
                lock_guard<mutex> lock(graphMutex); // Lock mutex for thread-safe access
                sccs = kosaraju(n, edges); // Compute SCCs for current graph state
            }

            // Prepare and send response with SCCs to client
            stringstream response;
            response << "scc:\n";
            for (const auto& scc : sccs) {
                for (int v : scc) {
                    response << v << " "; // Append vertex to response
                }
                response << "\n"; // Newline after each SCC
            }
            send(sockfd, response.str().c_str(), response.str().length(), 0); // Send response
        } else if (command == "Newedge") {
            {
                lock_guard<mutex> lock(graphMutex); // Lock mutex for thread-safe access
                int u, v;
                ss >> u >> v;
                edges.emplace_back(u, v); // Add new edge to edges vector
            }

            string response = "Edge added\n"; // Prepare response
            send(sockfd, response.c_str(), response.length(), 0); // Send response to client
        } else if (command == "Removeedge") {
            {
                lock_guard<mutex> lock(graphMutex); // Lock mutex for thread-safe access
                int u, v;
                ss >> u >> v;
                auto it = find(edges.begin(), edges.end(), make_pair(u, v)); // Find edge in edges vector
                if (it != edges.end()) {
                    edges.erase(it); // Erase edge if found
                    string response = "Edge removed\n"; // Prepare response
                    send(sockfd, response.c_str(), response.length(), 0); // Send response to client
                } else {
                    string response = "Edge not found\n"; // Prepare response
                    send(sockfd, response.c_str(), response.length(), 0); // Send response to client
                }
            }
        } else {
            string error = "Invalid command\n"; // Prepare error response
            send(sockfd, error.c_str(), error.length(), 0); // Send error response to client
        }
    }
}

// Main function to run the server
int main() {
    int server_fd, new_socket; // Server socket and new client socket
    struct sockaddr_in address; // Address structure for server
    int opt = 1; // Socket option
    int addrlen = sizeof(address); // Length of address structure

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); // Print error message if socket creation fails
        exit(EXIT_FAILURE); // Exit program
    }

    // Set socket options to reuse address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt"); // Print error message if setsockopt fails
        exit(EXIT_FAILURE); // Exit program
    }

    address.sin_family = AF_INET; // Address family is IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address
    address.sin_port = htons(9034); // Port number in network byte order

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); // Print error message if bind fails
        exit(EXIT_FAILURE); // Exit program
    }

    // Listen for incoming connections, maximum 3 pending connections
    if (listen(server_fd, 3) < 0) {
        perror("listen"); // Print error message if listen fails
        exit(EXIT_FAILURE); // Exit program
    }

    // Accept incoming connections and create a new thread for each client
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept"); // Print error message if accept fails
            exit(EXIT_FAILURE); // Exit program
        }

        pthread_t tid; // Thread ID
        pthread_create(&tid, NULL, clientThread, (void*)&new_socket); // Create new thread for client
    }

    return 0; 
}
