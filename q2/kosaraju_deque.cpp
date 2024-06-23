#include <iostream>
#include <vector>
#include <deque>
#include <stack>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace chrono;

void dfs1_deque(int v, const vector<deque<int>>& adj, vector<bool>& visited, stack<int>& finishStack) {
    visited[v] = true;
    for (int u : adj[v]) {
        if (!visited[u]) {
            dfs1_deque(u, adj, visited, finishStack);
        }
    }
    finishStack.push(v);
}

void dfs2_deque(int v, const vector<deque<int>>& revAdj, vector<bool>& visited, vector<int>& component) {
    visited[v] = true;
    component.push_back(v);
    for (int u : revAdj[v]) {
        if (!visited[u]) {
            dfs2_deque(u, revAdj, visited, component);
        }
    }
}

vector<vector<int>> kosaraju_deque(int n, const vector<pair<int, int>>& edges) {
    vector<deque<int>> adj(n + 1), revAdj(n + 1);
    for (const auto& edge : edges) {
        adj[edge.first].push_back(edge.second);
        revAdj[edge.second].push_back(edge.first);
    }

    stack<int> finishStack;
    vector<bool> visited(n + 1, false);
    
    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            dfs1_deque(i, adj, visited, finishStack);
        }
    }

    fill(visited.begin(), visited.end(), false);
    vector<vector<int>> sccs;

    while (!finishStack.empty()) {
        int v = finishStack.top();
        finishStack.pop();

        if (!visited[v]) {
            vector<int> component;
            dfs2_deque(v, revAdj, visited, component);
            sccs.push_back(component);
        }
    }

    return sccs;
}

int main() {
    int n, m;
    cin >> n >> m;

    vector<pair<int, int>> edges(m);
    for (int i = 0; i < m; ++i) {
        int u, v;
        cin >> u >> v;
        edges[i] = {u, v};
    }

    auto start = high_resolution_clock::now();
    vector<vector<int>> sccs = kosaraju_deque(n, edges);
    auto end = high_resolution_clock::now();
    cout << "Deque implementation took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    for (const auto& scc : sccs) {
        for (int v : scc) {
            cout << v << " ";
        }
        cout << endl;
    }

    return 0;
}
