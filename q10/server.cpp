#include "reactor.hpp"
#include <iostream>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>

// Global graph data
int n = 0, m = 0;
std::vector<std::pair<int, int>> edges;
std::mutex edgesMutex;  // Mutex for protecting access to edges

void dfs1(int v, const std::vector<std::list<int>>& adj, std::vector<bool>& visited, std::stack<int>& finishStack) {
    visited[v] = true;
    for (int u : adj[v]) {
        if (!visited[u]) {
            dfs1(u, adj, visited, finishStack);
        }
    }
    finishStack.push(v);
}

void dfs2(int v, const std::vector<std::list<int>>& revAdj, std::vector<bool>& visited, std::vector<int>& component) {
    visited[v] = true;
    component.push_back(v);
    for (int u : revAdj[v]) {
        if (!visited[u]) {
            dfs2(u, revAdj, visited, component);
        }
    }
}

std::vector<std::vector<int>> kosaraju(int n, const std::vector<std::pair<int, int>>& edges) {
    std::vector<std::list<int>> adj(n + 1), revAdj(n + 1);
    for (const auto& edge : edges) {
        adj[edge.first].push_back(edge.second);
        revAdj[edge.second].push_back(edge.first);
    }

    std::stack<int> finishStack;
    std::vector<bool> visited(n + 1, false);
    
    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            dfs1(i, adj, visited, finishStack);
        }
    }

    std::fill(visited.begin(), visited.end(), false);
    std::vector<std::vector<int>> sccs;

    while (!finishStack.empty()) {
        int v = finishStack.top();
        finishStack.pop();

        if (!visited[v]) {
            std::vector<int> component;
            dfs2(v, revAdj, visited, component);
            sccs.push_back(component);
        }
    }

    return sccs;
}

void handleClient(int clientSocket) {
    char buffer[1024] = {0};
    std::string command;

    while (true) {
        int valread = read(clientSocket, buffer, 1024);
        if (valread <= 0) {
            std::cout << "Client disconnected" << std::endl;
            close(clientSocket);
            return;
        }

        // Process command
        buffer[valread] = '\0';
        command = buffer;
        std::stringstream ss(command);
        ss >> command;

        if (command == "Newgraph") {
            ss >> n >> m;
            edgesMutex.lock();  // Lock mutex before accessing shared data
            edges.clear();
            
            // Read edges directly from command input
            for (int i = 0; i < m; ++i) {
                int u, v;
                if (!(ss >> u >> v)) {
                    std::cerr << "Error reading edge " << i << std::endl;
                    break;  // Exit loop on error
                }
                edges.push_back({u, v});
            }
            edgesMutex.unlock();  // Unlock mutex after accessing shared data
            std::string response = "Graph updated\n";
            send(clientSocket, response.c_str(), response.length(), 0);

        } else if (command == "Kosaraju") {
            edgesMutex.lock();  // Lock mutex before accessing shared data
            std::vector<std::vector<int>> sccs = kosaraju(n, edges);
            edgesMutex.unlock();  // Unlock mutex after accessing shared data

            std::stringstream response;
            response << "scc:\n";
            for (const auto& scc : sccs) {
                for (int v : scc) {
                    response << v << " ";
                }
                response << "\n";
            }
            send(clientSocket, response.str().c_str(), response.str().length(), 0);

            // Start a thread to monitor SCC conditions
            std::thread checkSCCThread([&sccs]() {
                int total_vertices = n; // Total number of vertices in the graph
                while (true) {
                    int largest_component_size = 0; // Size of the largest SCC found
                    for (const auto& component : sccs) {
                        if (component.size() > largest_component_size) {
                            largest_component_size = component.size();
                        }
                    }
                    if (largest_component_size >= total_vertices / 2) {
                        std::cout << "At least 50% of the graph belongs to the same SCC" << std::endl;
                    } else {
                        std::cout << "At least 50% of the graph no longer belongs to the same SCC" << std::endl;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(5)); // Check every 5 seconds
                }
            });

            checkSCCThread.detach(); // Detach thread to run independently
        } else if (command == "Newedge") {
            int u, v;
            ss >> u >> v;
            edgesMutex.lock();  // Lock mutex before accessing shared data
            edges.emplace_back(u, v);
            edgesMutex.unlock();  // Unlock mutex after accessing shared data
            std::string response = "Edge added\n";
            send(clientSocket, response.c_str(), response.length(), 0);

        } else if (command == "Removeedge") {
            int u, v;
            ss >> u >> v;
            edgesMutex.lock();  // Lock mutex before accessing shared data
            auto it = std::find(edges.begin(), edges.end(), std::make_pair(u, v));
            if (it != edges.end()) {
                edges.erase(it);
                std::cout << "Edge " << u << " -> " << v << " removed" << std::endl; // Print statement after removing an edge
            }
            edgesMutex.unlock();  // Unlock mutex after accessing shared data
            std::string response = "Edge removed\n";
            send(clientSocket, response.c_str(), response.length(), 0);

        } else {
            // Invalid command
            std::string error = "Invalid command\n";
            send(clientSocket, error.c_str(), error.length(), 0);
        }
    }
}

void acceptConnection(int serverSocket) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int newSocket;
    
    if ((newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        return;
    }

    // Print client connection information
    char clientAddr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), clientAddr, INET_ADDRSTRLEN);
    std::cout << "New connection from " << clientAddr << ":" << ntohs(address.sin_port) << std::endl;

    // Handle client in a new thread
    std::thread clientThread(handleClient, newSocket);
    clientThread.detach();
}

int main() {
    int serverSocket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 9034
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9034);

    // Forcefully attaching socket to the port 9034
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    void* reactor = startReactor();
    addFdToReactor(reactor, serverSocket, acceptConnection);
    
    Reactor* r = static_cast<Reactor*>(reactor);
    r->run();

    stopReactor(reactor);

    return 0;
}
