#include "NetworkManager.h"
#include <iostream>
#include <queue>
#include <algorithm>
#include <fstream>

using namespace std;

Graph<string> buildGraph(const vector<Submission>& subs, const vector<Reviewer>& revs, const AppConfig& config) {
    Graph<string> g;
    
    // 1. Add Source and Sink nodes
    g.addVertex("SOURCE");
    g.addVertex("SINK");

    // 2. Add Submissions and connect them from the Source
    for (const auto& sub : subs) {
        string subNode = "SUB_" + to_string(sub.id);
        g.addVertex(subNode);
        
        // Edge from Source to Submission. Capacity = MinReviewsPerSubmission
        g.addDirectedFlowEdge("SOURCE", subNode, config.minReviewsPerSubmission);
    }

    // 3. Add Reviewers and connect them to the Sink
    for (const auto& rev : revs) {
        string revNode = "REV_" + to_string(rev.id);
        g.addVertex(revNode);
        
        // Edge from Reviewer to Sink. Capacity = MaxReviewsPerReviewer
        g.addDirectedFlowEdge(revNode, "SINK", config.maxReviewsPerReviewer);
    }

    // 4. Connect Submissions to Reviewers based on Primary Domains
    for (const auto& sub : subs) {
        string subNode = "SUB_" + to_string(sub.id);
        for (const auto& rev : revs) {
            string revNode = "REV_" + to_string(rev.id);
            
            // If the submission's domain matches the reviewer's expertise
            if (sub.primaryDomain == rev.primaryExpertise) {
                // Capacity is 1 (a reviewer can only review a specific paper once)
                g.addDirectedFlowEdge(subNode, revNode, 1); 
            }
        }
    }
    
    return g;
}

// ---------------------------------------------------------
// Edmonds-Karp Algorithm Implementations
// ---------------------------------------------------------

bool findAugmentingPath(Graph<string>& g, string s, string t) {
    // 1. Reset all vertices
    for (auto v : g.getVertexSet()) {
        v->setVisited(false);
        v->setPath(nullptr);
    }

    auto sVertex = g.findVertex(s);
    sVertex->setVisited(true);
    queue<Vertex<string>*> q;
    q.push(sVertex);

    // 2. BFS to find a path to the sink
    while (!q.empty() && !g.findVertex(t)->isVisited()) {
        auto v = q.front();
        q.pop();

        for (auto e : v->getAdj()) {
            auto w = e->getDest();
            // If the node isn't visited AND there is available capacity
            if (!w->isVisited() && (e->getWeight() - e->getFlow()) > 0) {
                w->setVisited(true);
                w->setPath(e); // Remember how we got here
                q.push(w);
            }
        }
    }
    // Return true if we reached the sink
    return g.findVertex(t)->isVisited();
}

void edmondsKarp(Graph<string>& g, string s, string t) {
    // 1. Reset all flows to 0
    for (auto v : g.getVertexSet()) {
        for (auto e : v->getAdj()) {
            e->setFlow(0);
        }
    }

    // 2. Loop until no more augmenting paths exist
    while (findAugmentingPath(g, s, t)) {
        double bottleneck = INF;

        // 2a. Find the bottleneck capacity along the path
        auto curr = g.findVertex(t);
        while (curr->getInfo() != s) {
            auto e = curr->getPath();
            bottleneck = min(bottleneck, e->getWeight() - e->getFlow());
            curr = e->getOrig();
        }

        // 2b. Push the flow along the path and update residual edges
        curr = g.findVertex(t);
        while (curr->getInfo() != s) {
            auto e = curr->getPath();
            e->setFlow(e->getFlow() + bottleneck);
            e->getReverse()->setFlow(e->getReverse()->getFlow() - bottleneck);
            curr = e->getOrig();
        }
    }
}

// ---------------------------------------------------------
// Output Formatting (Task 2.1)
// ---------------------------------------------------------

// Helper struct to store assignments for sorting
struct Assignment {
    int subId;
    int revId;
    int matchDomain;
};

// Sort by Submission ID, then Reviewer ID
bool compareSubmissions(const Assignment& a, const Assignment& b) {
    if (a.subId != b.subId) return a.subId < b.subId;
    return a.revId < b.revId;
}

// Sort by Reviewer ID, then Submission ID
bool compareReviewers(const Assignment& a, const Assignment& b) {
    if (a.revId != b.revId) return a.revId < b.revId;
    return a.subId < b.subId;
}

// ---------------------------------------------------------
// Task 2.2: Risk Analysis = 1
// ---------------------------------------------------------
vector<int> runRiskAnalysis1(const DataLoader& data) {
    vector<int> riskyReviewers;

    // The total number of reviews we strictly need for the conference to be successful
    int targetFlow = data.submissions.size() * data.config.minReviewsPerSubmission;

    // Test removing each reviewer one by one
    for (const auto& rev : data.reviewers) {
        // Build a fresh base graph
        Graph<string> g = buildGraph(data.submissions, data.reviewers, data.config);

        // Completely remove this reviewer from the graph to simulate them dropping out
        g.removeVertex("REV_" + to_string(rev.id));

        // Run max flow on this damaged graph
        edmondsKarp(g, "SOURCE", "SINK");

        // Calculate how much flow we managed to push without them
        int currentFlow = 0;
        auto sourceVertex = g.findVertex("SOURCE");
        if (sourceVertex) {
            for (auto e : sourceVertex->getAdj()) {
                currentFlow += e->getFlow();
            }
        }

        // If we couldn't meet the target flow, this reviewer's absence breaks the assignment!
        if (currentFlow < targetFlow) {
            riskyReviewers.push_back(rev.id);
        }
    }

    sort(riskyReviewers.begin(), riskyReviewers.end());
    return riskyReviewers;
}

void generateAssignmentsOutput(Graph<string>& g, const DataLoader& data, const vector<int>& riskyReviewers) {
    vector<Assignment> assignments;

    for (const auto& sub : data.submissions) {
        string subNode = "SUB_" + to_string(sub.id);
        auto v = g.findVertex(subNode);
        for (auto e : v->getAdj()) {
            if (e->getFlow() > 0 && e->getDest()->getInfo().find("REV_") == 0) {
                int revId = stoi(e->getDest()->getInfo().substr(4));
                assignments.push_back({sub.id, revId, sub.primaryDomain});
            }
        }
    }

    string outName = data.config.outputFileName.empty() ? "output.csv" : data.config.outputFileName;
    ofstream outFile(outName);

    // EXACT SPACING: #SubmissionId,ReviewerId,Match
    vector<Assignment> subSorted = assignments;
    sort(subSorted.begin(), subSorted.end(), compareSubmissions);

    outFile << "#SubmissionId,ReviewerId,Match\n";
    for (const auto& a : subSorted) {
        outFile << a.subId << ", " << a.revId << ", " << a.matchDomain << "\n";
    }

    // EXACT SPACING: #ReviewerId,SubmissionId,Match
    vector<Assignment> revSorted = assignments;
    sort(revSorted.begin(), revSorted.end(), compareReviewers);

    outFile << "#ReviewerId,SubmissionId,Match\n";
    for (const auto& a : revSorted) {
        outFile << a.revId << ", " << a.subId << ", " << a.matchDomain << "\n";
    }

    outFile << "#Total: " << assignments.size() << "\n";

    // MISSING REVIEWS logic
    struct Missing { int subId; int domain; int missingReviews; };
    vector<Missing> missingList;
    auto sourceVertex = g.findVertex("SOURCE");
    for (auto e : sourceVertex->getAdj()) {
        if (e->getFlow() < e->getWeight()) {
            int missing = e->getWeight() - e->getFlow();
            int subId = stoi(e->getDest()->getInfo().substr(4));
            int domain = 0;
            for(const auto& s : data.submissions) {
                if (s.id == subId) { domain = s.primaryDomain; break; }
            }
            missingList.push_back({subId, domain, missing});
        }
    }

    if (!missingList.empty()) {
        outFile << "#SubmissionId,Domain,MissingReviews\n";
        sort(missingList.begin(), missingList.end(), [](const Missing& a, const Missing& b){ return a.subId < b.subId; });
        for(const auto& m : missingList) {
            outFile << m.subId << ", " << m.domain << ", " << m.missingReviews << "\n";
        }
    }

    // TASK 2.2: RISK ANALYSIS OUTPUT
    if (data.config.riskAnalysis == 1) {
        outFile << "#Risk Analysis: 1\n";
        if (!riskyReviewers.empty()) {
            for (size_t i = 0; i < riskyReviewers.size(); ++i) {
                outFile << riskyReviewers[i] << (i + 1 == riskyReviewers.size() ? "\n" : ", ");
            }
        } else {
            outFile << "\n"; // If no risky reviewers, just print an empty line
        }
    }

    cout << "Finished! Results saved to " << outName << endl;
    outFile.close();
}