#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <queue>
#include <limits>
#include <algorithm>
#include <iostream>
#include <functional>

/**
 * @brief Represents an edge in the flow network.
 *
 * Each edge stores capacity, flow, and a pointer to its reverse edge
 * (required for residual graph in Max-Flow algorithms).
 *
 * Time Complexity: O(1) for all operations.
 */
struct Edge {
    int dest;       ///< Destination vertex index
    int capacity;   ///< Edge capacity
    int flow;       ///< Current flow through the edge
    int rev;        ///< Index of reverse edge in dest's adjacency list

    Edge(int dest, int capacity, int rev)
        : dest(dest), capacity(capacity), flow(0), rev(rev) {}

    /// @brief Returns the residual capacity of this edge.
    int residual() const { return capacity - flow; }
};

/**
 * @brief Represents a vertex in the graph.
 */
struct Vertex {
    std::vector<Edge> adj; ///< Adjacency list (outgoing edges)
    bool visited;          ///< Used in BFS/DFS traversals
    int parent;            ///< Parent vertex index (for path reconstruction)
    int parentEdge;        ///< Index of edge used to reach this vertex

    Vertex() : visited(false), parent(-1), parentEdge(-1) {}
};

/**
 * @brief Directed graph with flow network support.
 *
 * Based on the adjacency-list graph structure from TP lectures.
 * Extended to support Max-Flow (Edmonds-Karp / Ford-Fulkerson with BFS).
 *
 * Vertex indices are 0-based internally.
 */
class Graph {
public:
    /**
     * @brief Constructs a graph with n vertices.
     * @param n Number of vertices.
     * Time Complexity: O(n)
     */
    explicit Graph(int n) : vertices(n) {}

    /// @brief Returns the number of vertices.
    int size() const { return (int)vertices.size(); }

    /**
     * @brief Adds a directed edge with given capacity, plus its reverse edge (capacity 0).
     * @param u Source vertex (0-based)
     * @param v Destination vertex (0-based)
     * @param cap Capacity of the edge
     * Time Complexity: O(1)
     */
    void addEdge(int u, int v, int cap) {
        int uRev = (int)vertices[v].adj.size();
        int vRev = (int)vertices[u].adj.size();
        vertices[u].adj.emplace_back(v, cap, uRev);
        vertices[v].adj.emplace_back(u, 0, vRev); // reverse edge
    }

    /**
     * @brief BFS to find an augmenting path from source to sink.
     * @param s Source vertex
     * @param t Sink vertex
     * @return true if a path exists
     * Time Complexity: O(V + E)
     */
    bool bfs(int s, int t) {
        for (auto& v : vertices) {
            v.visited = false;
            v.parent = -1;
            v.parentEdge = -1;
        }
        vertices[s].visited = true;
        std::queue<int> q;
        q.push(s);
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int i = 0; i < (int)vertices[u].adj.size(); i++) {
                Edge& e = vertices[u].adj[i];
                if (!vertices[e.dest].visited && e.residual() > 0) {
                    vertices[e.dest].visited = true;
                    vertices[e.dest].parent = u;
                    vertices[e.dest].parentEdge = i;
                    if (e.dest == t) return true;
                    q.push(e.dest);
                }
            }
        }
        return false;
    }

    /**
     * @brief Computes maximum flow from s to t using Edmonds-Karp algorithm.
     * @param s Source vertex
     * @param t Sink vertex
     * @return Maximum flow value
     * Time Complexity: O(V * E^2)
     */
    int maxFlow(int s, int t) {
        int totalFlow = 0;
        while (bfs(s, t)) {
            // Find bottleneck
            int pathFlow = std::numeric_limits<int>::max();
            int v = t;
            while (v != s) {
                int u = vertices[v].parent;
                int ei = vertices[v].parentEdge;
                pathFlow = std::min(pathFlow, vertices[u].adj[ei].residual());
                v = u;
            }
            // Augment flow
            v = t;
            while (v != s) {
                int u = vertices[v].parent;
                int ei = vertices[v].parentEdge;
                Edge& fwd = vertices[u].adj[ei];
                Edge& rev = vertices[fwd.dest].adj[fwd.rev];
                fwd.flow += pathFlow;
                rev.flow -= pathFlow;
                v = u;
            }
            totalFlow += pathFlow;
        }
        return totalFlow;
    }

    /**
     * @brief Resets all edge flows to 0 (useful for re-running flow after changes).
     * Time Complexity: O(V + E)
     */
    void resetFlow() {
        for (auto& v : vertices) {
            for (auto& e : v.adj) {
                e.flow = 0;
            }
        }
    }

    /// @brief Direct access to vertex data.
    Vertex& getVertex(int i) { return vertices[i]; }
    const Vertex& getVertex(int i) const { return vertices[i]; }

private:
    std::vector<Vertex> vertices;
};

#endif // GRAPH_H
