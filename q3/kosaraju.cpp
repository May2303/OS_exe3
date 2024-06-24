#include <iostream>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>

using namespace std;

void dfs1(int v, const vector<list<int>>& adj, vector<bool>& visited, stack<int>& finishStack) {
    visited[v] = true;
    for (int u : adj[v]) {
        if (!visited[u]) {
            dfs1(u, adj, visited, finishStack);
        }
    }
    finishStack.push(v);
}

void dfs2(int v, const vector<list<int>>& revAdj, vector<bool>& visited, vector<int>& component) {
    visited[v] = true;
    component.push_back(v);
    for (int u : revAdj[v]) {
        if (!visited[u]) {
            dfs2(u, revAdj, visited, component);
        }
    }
}

vector<vector<int>> kosaraju(int n, const vector<pair<int, int>>& edges) {
    vector<list<int>> adj(n + 1), revAdj(n + 1);
    for (const auto& edge : edges) {
        adj[edge.first].push_back(edge.second);
        revAdj[edge.second].push_back(edge.first);
    }

    stack<int> finishStack;
    vector<bool> visited(n + 1, false);
    
    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            dfs1(i, adj, visited, finishStack);
        }
    }

    fill(visited.begin(), visited.end(), false);
    vector<vector<int>> sccs;

    while (!finishStack.empty()) {
        int v = finishStack.top();
        finishStack.pop();

        if (!visited[v]) {
            vector<int> component;
            dfs2(v, revAdj, visited, component);
            sccs.push_back(component);
        }
    }

    return sccs;
}

int main() {
    string command;
    int n = 0, m = 0;
    vector<pair<int, int>> edges;

    while (cin >> command) {
        if (command == "Newgraph") {
            cin >> n >> m;
            edges.clear();
        } else if (command == "Kosaraju") {
            vector<vector<int>> sccs = kosaraju(n, edges);
            cout << "scc:\n";
            for (const auto& scc : sccs) {
                for (int v : scc) {
                    cout << v << " ";
                }
                cout << endl;
            }
        } else if (command == "Newedge") {
            int u, v;
            cin >> u >> v;
            edges.emplace_back(u, v);
        } else if (command == "Removeedge") {
            int u, v;
            cin >> u >> v;
            auto it = find(edges.begin(), edges.end(), make_pair(u, v));
            if (it != edges.end()) {
                edges.erase(it);
            }
        } else {
            // Invalid command, skip line
            getline(cin, command);
        }
    }

    return 0;
}
