#include <stdio.h>
#include <stdlib.h>

#define MAX_VERTICES 1000 // Adjust as necessary

// Structure for adjacency list node
struct AdjListNode {
    int dest;
    struct AdjListNode* next;
};

// Structure for adjacency list
struct AdjList {
    struct AdjListNode *head;
};

// Structure for graph
struct Graph {
    int num_vertices;
    struct AdjList* adj_list;
};

// Function to add an edge to the graph
void addEdge(struct Graph* graph, int src, int dest) {
    struct AdjListNode* newNode = (struct AdjListNode*) malloc(sizeof(struct AdjListNode));
    newNode->dest = dest;
    newNode->next = graph->adj_list[src].head;
    graph->adj_list[src].head = newNode;
}

// Function to perform Depth-First Search
void DFS(struct Graph* graph, int v, int visited[], struct Graph* transposedGraph);

// Function to initialize a graph with a given number of vertices
struct Graph* createGraph(int num_vertices) {
    struct Graph* graph = (struct Graph*) malloc(sizeof(struct Graph));
    graph->num_vertices = num_vertices;
    graph->adj_list = (struct AdjList*) malloc(num_vertices * sizeof(struct AdjList));

    for (int i = 0; i < num_vertices; ++i)
        graph->adj_list[i].head = NULL;

    return graph;
}

// Function to transpose the graph
struct Graph* transposeGraph(struct Graph* graph) {
    struct Graph* transposedGraph = createGraph(graph->num_vertices);

    for (int v = 0; v < graph->num_vertices; v++) {
        struct AdjListNode* temp = graph->adj_list[v].head;
        while (temp != NULL) {
            addEdge(transposedGraph, temp->dest, v);
            temp = temp->next;
        }
    }

    return transposedGraph;
}

// Function to fill order of vertices in stack
void fillOrder(struct Graph* graph, int v, int visited[], struct Graph* transposedGraph, int* stack, int* stack_top) {
    visited[v] = 1;

    struct AdjListNode* temp = graph->adj_list[v].head;
    while (temp != NULL) {
        if (!visited[temp->dest])
            fillOrder(graph, temp->dest, visited, transposedGraph, stack, stack_top);
        temp = temp->next;
    }

    // All vertices reachable from v are processed by now, push v to stack
    stack[++(*stack_top)] = v;
}

// Function to print strongly connected components
void printSCCs(struct Graph* graph) {
    int num_vertices = graph->num_vertices;
    int* stack = (int*) malloc(num_vertices * sizeof(int));
    int stack_top = -1;

    int* visited = (int*) malloc(num_vertices * sizeof(int));
    for (int i = 0; i < num_vertices; i++)
        visited[i] = 0;

    // Fill vertices in stack according to their finishing times
    for (int v = 0; v < num_vertices; v++) {
        if (!visited[v])
            fillOrder(graph, v, visited, NULL, stack, &stack_top);
    }

    // Create a transposed graph
    struct Graph* transposedGraph = transposeGraph(graph);

    // Mark all the vertices as not visited (for the second DFS)
    for (int i = 0; i < num_vertices; i++)
        visited[i] = 0;

    // Now process all vertices in order defined by stack
    while (stack_top >= 0) {
        int v = stack[stack_top--];

        if (!visited[v]) {
            DFS(transposedGraph, v, visited, transposedGraph);
            printf("\n");
        }
    }
}

// Recursive function to perform DFS
void DFS(struct Graph* graph, int v, int visited[], struct Graph* transposedGraph) {
    visited[v] = 1;
    printf("%d ", v + 1); // Print vertex (adjust for 1-based indexing)

    struct AdjListNode* temp = graph->adj_list[v].head;
    while (temp != NULL) {
        if (!visited[temp->dest])
            DFS(graph, temp->dest, visited, transposedGraph);
        temp = temp->next;
    }
}

// Driver program to test above functions
int main() {
    int num_vertices, num_arcs;
    scanf("%d %d", &num_vertices, &num_arcs);

    struct Graph* graph = createGraph(num_vertices);

    int src, dest;
    for (int i = 0; i < num_arcs; ++i) {
        scanf("%d %d", &src, &dest);
        addEdge(graph, src - 1, dest - 1); // Adjust for 0-based indexing
    }

    printf("Strongly Connected Components:\n");
    printSCCs(graph);
    return 0;
}