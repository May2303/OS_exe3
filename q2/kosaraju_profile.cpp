#include <iostream>
#include <vector>
#include <list>
#include <deque>
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

// DFS functions for deque implementation
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

// Kosaraju's algorithm for deque implementation
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

// DFS functions for matrix implementation
void dfs1_matrix(int v, const vector<vector<int>>& adj, vector<bool>& visited, stack<int>& finishStack) {
    visited[v] = true;
    for (int u = 0; u < adj[v].size(); ++u) {
        if (adj[v][u] && !visited[u]) {
            dfs1_matrix(u, adj, visited, finishStack);
        }
    }
    finishStack.push(v);
}

void dfs2_matrix(int v, const vector<vector<int>>& revAdj, vector<bool>& visited, vector<int>& component) {
    visited[v] = true;
    component.push_back(v);
    for (int u = 0; u < revAdj[v].size(); ++u) {
        if (revAdj[v][u] && !visited[u]) {
            dfs2_matrix(u, revAdj, visited, component);
        }
    }
}

// Kosaraju's algorithm for matrix implementation
vector<vector<int>> kosaraju_matrix(int n, const vector<pair<int, int>>& edges) {
    vector<vector<int>> adj(n + 1, vector<int>(n + 1, 0)), revAdj(n + 1, vector<int>(n + 1, 0));
    for (const auto& edge : edges) {
        adj[edge.first][edge.second] = 1;
        revAdj[edge.second][edge.first] = 1;
    }

    stack<int> finishStack;
    vector<bool> visited(n + 1, false);
    
    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            dfs1_matrix(i, adj, visited, finishStack);
        }
    }

    fill(visited.begin(), visited.end(), false);
    vector<vector<int>> sccs;

    while (!finishStack.empty()) {
        int v = finishStack.top();
        finishStack.pop();

        if (!visited[v]) {
            vector<int> component;
            dfs2_matrix(v, revAdj, visited, component);
            sccs.push_back(component);
        }
    }

    return sccs;
}

// Function to profile both implementations
void profile_list_vs_deque() {
    int n = 10000;
    vector<pair<int, int>> edges;
    for (int i = 1; i < n; ++i) {
        edges.emplace_back(i, i + 1);
    }
    edges.emplace_back(n, 1);

    auto start = high_resolution_clock::now();
    kosaraju_list(n, edges);
    auto end = high_resolution_clock::now();
    cout << "List implementation took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    start = high_resolution_clock::now();
    kosaraju_deque(n, edges);
    end = high_resolution_clock::now();
    cout << "Deque implementation took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;
}

// Function to profile both graph realizations
void profile_matrix_vs_list() {
    int n = 1000;
    vector<pair<int, int>> edges;
    for (int i = 1; i < n; ++i) {
        edges.emplace_back(i, i + 1);
    }
    edges.emplace_back(n, 1);

    auto start = high_resolution_clock::now();
    kosaraju_matrix(n, edges);
    auto end = high_resolution_clock::now();
    cout << "Matrix implementation took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    start = high_resolution_clock::now();
    kosaraju_list(n, edges);
    end = high_resolution_clock::now();
    cout << "List implementation took " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;
}

int main() {
    cout << "Profiling list vs deque:" << endl;
    profile_list_vs_deque();
    cout << endl;

    cout << "Profiling matrix vs list:" << endl;
    profile_matrix_vs_list();
    return 0;
}
