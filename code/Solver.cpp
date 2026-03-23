#include "Solver.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <set>
#include <numeric>
#include <map>

// ──────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────

Solver::Solver(const std::vector<Submission>& submissions,
               const std::vector<Reviewer>&   reviewers,
               const Parameters&              params,
               const ControlOptions&          control)
    : subs(submissions), revs(reviewers), params(params), control(control) {}

// ──────────────────────────────────────────────────────────────
// canMatch
// Determines if reviewer r can review submission s under the given mode.
// Mode 1: only primary topics matched
// Mode 2: submission primary OR secondary vs reviewer primary
// Mode 3: all primary/secondary combinations
// Time Complexity: O(1)
// ──────────────────────────────────────────────────────────────

bool Solver::canMatch(const Submission& s, const Reviewer& r, int mode, int& matchDomain) const {
    auto check = [&](int sDomain, int rDomain) -> bool {
        if (sDomain != 0 && rDomain != 0 && sDomain == rDomain) {
            matchDomain = sDomain;
            return true;
        }
        return false;
    };

    // Mode 1: primary submission domain ↔ primary reviewer expertise
    if (check(s.primaryTopic, r.primaryExpertise)) return true;
    if (mode == 1) return false;

    // Mode 2: add secondary submission domain ↔ primary reviewer expertise
    if (check(s.secondaryTopic, r.primaryExpertise)) return true;
    if (mode == 2) return false;

    // Mode 3: also secondary reviewer expertise
    if (check(s.primaryTopic, r.secondaryExpertise)) return true;
    if (check(s.secondaryTopic, r.secondaryExpertise)) return true;

    return false;
}

// ──────────────────────────────────────────────────────────────
// requiredFlow
// Total flow needed for a complete valid assignment.
// Time Complexity: O(N)
// ──────────────────────────────────────────────────────────────

int Solver::requiredFlow() const {
    return (int)subs.size() * params.minReviewsPerSubmission;
}

// ──────────────────────────────────────────────────────────────
// buildGraph
// Graph layout:
//   Node 0         = source (S)
//   Nodes 1..N     = submission nodes
//   Nodes N+1..N+M = reviewer nodes
//   Node N+M+1     = sink (T)
//
// Edges:
//   S -> sub_i      capacity = MinReviewsPerSubmission
//   sub_i -> rev_j  capacity = 1  (if domains match)
//   rev_j -> T      capacity = MaxReviewsPerReviewer
//
// Time Complexity: O(N * M)
// ──────────────────────────────────────────────────────────────

Graph Solver::buildGraph(int mode, const std::vector<int>& excludedReviewerIds) const {
    int N = (int)subs.size();
    int M = (int)revs.size();
    int totalNodes = 1 + N + M + 1;
    int S = 0;
    int T = N + M + 1;

    Graph g(totalNodes);
    std::set<int> excluded(excludedReviewerIds.begin(), excludedReviewerIds.end());

    // S -> submission nodes
    for (int i = 0; i < N; ++i) {
        g.addEdge(S, 1 + i, params.minReviewsPerSubmission);
    }

    // submission nodes -> reviewer nodes (domain matching)
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            if (excluded.count(revs[j].id)) continue;
            int matchDomain = 0;
            if (canMatch(subs[i], revs[j], mode, matchDomain)) {
                g.addEdge(1 + i, 1 + N + j, 1);
            }
        }
    }

    // reviewer nodes -> T
    for (int j = 0; j < M; ++j) {
        if (excluded.count(revs[j].id)) continue;
        g.addEdge(1 + N + j, T, params.maxReviewsPerReviewer);
    }

    return g;
}

// ──────────────────────────────────────────────────────────────
// extractAssignments
// Reads the flow values on sub->rev edges to produce assignments.
// Time Complexity: O(N * M)
// ──────────────────────────────────────────────────────────────

std::vector<Assignment> Solver::extractAssignments(const Graph& g, int subOffset, int revOffset, int mode) const {
    std::vector<Assignment> result;
    int N = (int)subs.size();

    for (int i = 0; i < N; ++i) {
        int subNode = subOffset + i;
        const Vertex& v = g.getVertex(subNode);
        for (const Edge& e : v.adj) {
            if (e.flow > 0 && e.dest >= revOffset) {
                int revIdx = e.dest - revOffset;
                Assignment a;
                a.submissionId = subs[i].id;
                a.reviewerId   = revs[revIdx].id;
                // Determine match domain
                canMatch(subs[i], revs[revIdx], mode, a.matchDomain);
                result.push_back(a);
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const Assignment& a, const Assignment& b) {
        if (a.submissionId != b.submissionId) return a.submissionId < b.submissionId;
        return a.reviewerId < b.reviewerId;
    });

    return result;
}

// ──────────────────────────────────────────────────────────────
// solve
// Main solving routine.
// Time Complexity: O(V * E^2) — Edmonds-Karp
// ──────────────────────────────────────────────────────────────

std::vector<Assignment> Solver::solve(int mode) {
    int N = (int)subs.size();
    int M = (int)revs.size();
    int S = 0;
    int T = N + M + 1;

    Graph g = buildGraph(mode);
    int flow = g.maxFlow(S, T);
    int required = requiredFlow();

    if (flow < required) {
        std::cout << "Warning: incomplete assignment. Achieved flow=" << flow
                  << " / required=" << required << "\n";
    }

    return extractAssignments(g, 1, 1 + N, mode);
}

// ──────────────────────────────────────────────────────────────
// riskAnalysis
// For K=1: removes each reviewer one at a time and checks feasibility.
// For K>1: removes all combinations of K reviewers (outline; full impl optional).
// Time Complexity: O(M * V * E^2) for K=1
// ──────────────────────────────────────────────────────────────

std::vector<std::vector<int>> Solver::riskAnalysis(int K, const std::vector<Assignment>& /*assignments*/) {
    std::vector<std::vector<int>> riskyGroups;
    int N = (int)subs.size();
    int M = (int)revs.size();
    int S = 0;
    int T = N + M + 1;
    int mode = control.generateAssignments;
    int required = requiredFlow();

    if (K == 1) {
        // Check each reviewer individually
        for (int j = 0; j < M; ++j) {
            Graph g = buildGraph(mode, {revs[j].id});
            int flow = g.maxFlow(S, T);
            if (flow < required) {
                riskyGroups.push_back({revs[j].id});
            }
        }
    } else {
        // K > 1: enumerate all subsets of size K (exponential — only feasible for small M)
        // For large M, an approximation or heuristic should be used.
        std::vector<int> indices(M);
        std::iota(indices.begin(), indices.end(), 0);

        // Generate combinations of size K using bitmask (only practical for small M+K)
        std::vector<bool> selector(M, false);
        std::fill(selector.begin(), selector.begin() + K, true);
        std::sort(selector.begin(), selector.end());

        do {
            std::vector<int> excluded;
            for (int j = 0; j < M; ++j)
                if (selector[j]) excluded.push_back(revs[j].id);

            Graph g = buildGraph(mode, excluded);
            int flow = g.maxFlow(S, T);
            if (flow < required) {
                std::sort(excluded.begin(), excluded.end());
                riskyGroups.push_back(excluded);
            }
        } while (std::next_permutation(selector.begin(), selector.end()));
    }

    return riskyGroups;
}

// ──────────────────────────────────────────────────────────────
// printAssignments
// ──────────────────────────────────────────────────────────────

void Solver::printAssignments(const std::vector<Assignment>& assignments) const {
    if (assignments.empty()) {
        std::cout << "No assignments generated.\n";
        return;
    }

    std::cout << "#SubmissionId,ReviewerId,Match\n";
    for (const auto& a : assignments)
        std::cout << a.submissionId << ", " << a.reviewerId << ", " << a.matchDomain << "\n";

    // Dual view: reviewer -> submissions
    std::cout << "#ReviewerId,SubmissionId,Match\n";
    std::vector<Assignment> byReviewer = assignments;
    std::sort(byReviewer.begin(), byReviewer.end(), [](const Assignment& a, const Assignment& b) {
        if (a.reviewerId != b.reviewerId) return a.reviewerId < b.reviewerId;
        return a.submissionId < b.submissionId;
    });
    for (const auto& a : byReviewer)
        std::cout << a.reviewerId << ", " << a.submissionId << ", " << a.matchDomain << "\n";

    std::cout << "#Total: " << assignments.size() << "\n";
}

// ──────────────────────────────────────────────────────────────
// writeOutput
// ──────────────────────────────────────────────────────────────

void Solver::writeOutput(const std::string& filename,
                         const std::vector<Assignment>& assignments,
                         const std::vector<Assignment>& /*missing*/,
                         const std::vector<std::vector<int>>& riskGroups,
                         int K) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: cannot open output file '" << filename << "'\n";
        return;
    }

    if (!assignments.empty()) {
        out << "#SubmissionId,ReviewerId,Match\n";
        for (const auto& a : assignments)
            out << a.submissionId << ", " << a.reviewerId << ", " << a.matchDomain << "\n";

        out << "#ReviewerId,SubmissionId,Match\n";
        std::vector<Assignment> byReviewer = assignments;
        std::sort(byReviewer.begin(), byReviewer.end(), [](const Assignment& a, const Assignment& b) {
            if (a.reviewerId != b.reviewerId) return a.reviewerId < b.reviewerId;
            return a.submissionId < b.submissionId;
        });
        for (const auto& a : byReviewer)
            out << a.reviewerId << ", " << a.submissionId << ", " << a.matchDomain << "\n";

        out << "#Total: " << assignments.size() << "\n";
    }

    // Missing reviews
    {
        // Compute per-submission review counts
        std::map<int, int> reviewCount;
        for (const auto& a : assignments) reviewCount[a.submissionId]++;
        bool anyMissing = false;
        for (const auto& s : subs) {
            int got = reviewCount.count(s.id) ? reviewCount[s.id] : 0;
            if (got < params.minReviewsPerSubmission) {
                if (!anyMissing) {
                    out << "#SubmissionId,Domain,MissingReviews\n";
                    anyMissing = true;
                }
                out << s.id << ", " << s.primaryTopic << ", "
                    << (params.minReviewsPerSubmission - got) << "\n";
            }
        }
    }

    // Risk analysis output
    if (!riskGroups.empty()) {
        out << "#Risk Analysis: " << K << "\n";
        for (const auto& group : riskGroups) {
            bool first = true;
            for (int id : group) {
                if (!first) out << ", ";
                out << id;
                first = false;
            }
            out << "\n";
        }
    }

    std::cout << "Output written to '" << filename << "'\n";
}

void Solver::printMissing() const {
    // Placeholder: called after solve() to show incomplete assignments
    // Full implementation would track per-submission flow deficit
}
