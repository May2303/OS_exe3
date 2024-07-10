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

mutex graphMutex;
int n = 0, m = 0;
vector<pair<int, int>> edges;

void dfs1(int v, const vector<list<int>> &adj, vector<bool> &visited, stack<int> &finishStack)
{
    visited[v] = true;
    for (int u : adj[v])
    {
        if (!visited[u])
        {
            dfs1(u, adj, visited, finishStack);
        }
    }
    finishStack.push(v);
}

void dfs2(int v, const vector<list<int>> &revAdj, vector<bool> &visited, vector<int> &component)
{
    visited[v] = true;
    component.push_back(v);
    for (int u : revAdj[v])
    {
        if (!visited[u])
        {
            dfs2(u, revAdj, visited, component);
        }
    }
}

vector<vector<int>> kosaraju(int n, const vector<pair<int, int>> &edges)
{
    vector<list<int>> adj(n + 1), revAdj(n + 1);
    for (const auto &edge : edges)
    {
        adj[edge.first].push_back(edge.second);
        revAdj[edge.second].push_back(edge.first);
    }

    stack<int> finishStack;
    vector<bool> visited(n + 1, false);

    for (int i = 1; i <= n; ++i)
    {
        if (!visited[i])
        {
            dfs1(i, adj, visited, finishStack);
        }
    }

    fill(visited.begin(), visited.end(), false);
    vector<vector<int>> sccs;

    while (!finishStack.empty())
    {
        int v = finishStack.top();
        finishStack.pop();

        if (!visited[v])
        {
            vector<int> component;
            dfs2(v, revAdj, visited, component);
            sccs.push_back(component);
        }
    }

    return sccs;
}

void *handleClient(int fd)
{
    char buffer[1024] = {0};
    string command;

    while (true)
    {
        int valread = read(fd, buffer, 1024);
        if (valread <= 0)
        {
            cout << "Client disconnected" << endl;
            close(fd);
            return nullptr;
        }

        buffer[valread] = '\0';
        command = buffer;
        stringstream ss(command);
        ss >> command;

        if (command == "Newgraph")
        {
            lock_guard<mutex> lock(graphMutex);
            ss >> n >> m;
            edges.clear();
            edges.resize(m);

            for (int i = 0; i < m; ++i)
            {
                int u, v;
                ss >> u >> v;
                edges[i] = {u, v};
            }
            string response = "Graph updated\n";
            send(fd, response.c_str(), response.length(), 0);
        }
        else if (command == "Kosaraju")
        {
            vector<vector<int>> sccs;
            {
                lock_guard<mutex> lock(graphMutex);
                sccs = kosaraju(n, edges);
            }
            stringstream response;
            response << "scc:\n";
            for (const auto &scc : sccs)
            {
                for (int v : scc)
                {
                    response << v << " ";
                }
                response << "\n";
            }
            send(fd, response.str().c_str(), response.str().length(), 0);
        }
        else if (command == "Newedge")
        {
            lock_guard<mutex> lock(graphMutex);
            int u, v;
            ss >> u >> v;
            edges.emplace_back(u, v);
            string response = "Edge added\n";
            send(fd, response.c_str(), response.length(), 0);
        }
        else if (command == "Removeedge")
        {
            lock_guard<mutex> lock(graphMutex);
            int u, v;
            ss >> u >> v;
            auto it = find(edges.begin(), edges.end(), make_pair(u, v));
            if (it != edges.end())
            {
                edges.erase(it);
                string response = "Edge removed\n";
                send(fd, response.c_str(), response.length(), 0);
            }
            else
            {
                string response = "Edge not found\n";
                send(fd, response.c_str(), response.length(), 0);
            }
        }
        else
        {
            string error = "Invalid command\n";
            send(fd, error.c_str(), error.length(), 0);
        }
    }

    return nullptr;
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9034);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    Reactor *reactor = Reactor::startReactor();

    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        reactor->startProactor(new_socket, handleClient);
    }

    reactor->stopReactor();
    delete reactor;

    return 0;
}
