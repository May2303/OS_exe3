#include <iostream>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace chrono;

// DFS functions for list implementation
void dfs1_list(int v, const vector<list<int>>& adj, vector<bool>& visited, stack<int>& finishStack) {
    visited[v] = true;
    for (int u : adj[v]) {
        if (!visited[u]) {
            dfs1_list(u, adj, visited, finishStack);
        }
    }
    finishStack.push(v);
}

void dfs2_list(int v, const vector<list<int>>& revAdj, vector<bool>& visited, vector<int>& component) {
    visited[v] = true;
    component.push_back(v);
    for (int u : revAdj[v]) {
        if (!visited[u]) {
            dfs2_list(u, revAdj, visited, component);
        }
    }
}

// Kosaraju's algorithm for list implementation
vector<vector<int>> kosaraju_list(int n, const vector<pair<int, int>>& edges) {
    vector<list<int>> adj(n + 1), revAdj(n + 1);
    for (const auto& edge : edges) {
        adj[edge.first].push_back(edge.second);
        revAdj[edge.second].push_back(edge.first);
    }

    stack<int> finishStack;
    vector<bool> visited(n + 1, false);
    
    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            dfs1_list(i, adj, visited, finishStack);
        }
    }

    fill(visited.begin(), visited.end(), false);
    vector<vector<int>> sccs;

    while (!finishStack.empty()) {
        int v = finishStack.top();
        finishStack.pop();

        if (!visited[v]) {
            vector<int> component;
            dfs2_list(v, revAdj, visited, component);
            sccs.push_back(component);
        }
    }

    return sccs;
}

// Function to read input, execute Kosaraju's algorithm, and output the SCCs
int main() {
    int n, m;
    cin >> n >> m;

    vector<pair<int, int>> edges(m);
    for (int i = 0; i < m; ++i) {
        int u, v;
        cin >> u >> v;
        edges[i] = {u, v};
    }

    vector<vector<int>> sccs = kosaraju_list(n, edges);

    for (const auto& scc : sccs) {
        for (int v : scc) {
            cout << v << " ";
        }
        cout << endl;
    }

    return 0;
}
