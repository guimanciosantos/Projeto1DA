#ifndef SOLVER_H
#define SOLVER_H

#include "Graph.h"
#include "Types.h"
#include <vector>
#include <string>

/**
 * @brief Builds and solves the Max-Flow based review assignment problem.
 *
 * Graph topology (GenerateAssignments = 1, primary only):
 *
 *   source (S) --[minReviews]--> submission node --[1]--> reviewer node --[maxReviews]--> sink (T)
 *
 * Edges between submission and reviewer nodes exist only when their topics match.
 *
 * Each submission node has demand = MinReviewsPerSubmission (modelled as capacity on S->sub edge).
 * Each reviewer node has capacity MaxReviewsPerReviewer on the reviewer->T edge.
 *
 * A feasible assignment exists iff maxFlow(S, T) == sum of all minimum reviews.
 */
class Solver {
public:
    /**
     * @brief Constructs the solver with parsed data.
     */
    Solver(const std::vector<Submission>& submissions,
           const std::vector<Reviewer>& reviewers,
           const Parameters& params,
           const ControlOptions& control);

    /**
     * @brief Builds the flow graph and runs max-flow. Returns assignments.
     * @param mode GenerateAssignments value (1=primary only, 2=sub secondary, 3=all)
     * @return List of assignments; empty if infeasible.
     * Time Complexity: O(V * E^2) — Edmonds-Karp
     */
    std::vector<Assignment> solve(int mode);

    /**
     * @brief Performs Risk Analysis with parameter K.
     * Removes each subset of K reviewers and checks feasibility.
     * @param K Number of reviewers to remove simultaneously.
     * @param assignments The baseline assignment to analyse.
     * @return List of reviewer IDs whose removal makes assignment infeasible.
     * Time Complexity: O(C(M,K) * V * E^2) where M = #reviewers
     */
    std::vector<std::vector<int>> riskAnalysis(int K, const std::vector<Assignment>& assignments);

    /**
     * @brief Writes the assignment output to a file in the specified format.
     * @param filename Output file path
     * @param assignments Assignment list
     * @param missing Submissions that could not be fully assigned (if any)
     * @param riskGroups Risky reviewer groups from risk analysis
     * @param K Risk parameter used
     */
    void writeOutput(const std::string& filename,
                     const std::vector<Assignment>& assignments,
                     const std::vector<Assignment>& missing,
                     const std::vector<std::vector<int>>& riskGroups,
                     int K) const;

    /**
     * @brief Prints assignment results to stdout in the same format.
     */
    void printAssignments(const std::vector<Assignment>& assignments) const;

    /**
     * @brief Prints missing review information to stdout.
     */
    void printMissing() const;

private:
    const std::vector<Submission>& subs;
    const std::vector<Reviewer>&   revs;
    const Parameters&              params;
    const ControlOptions&          control;

    // Helpers
    /**
     * @brief Returns true if reviewer r can review submission s under given mode.
     * @param s Submission
     * @param r Reviewer
     * @param mode Match mode (1, 2, or 3)
     * @param matchDomain Output: the domain on which they matched
     */
    bool canMatch(const Submission& s, const Reviewer& r, int mode, int& matchDomain) const;

    /**
     * @brief Builds the flow graph for a given mode, optionally excluding some reviewers.
     * @param mode Match mode
     * @param excludedReviewerIds Set of reviewer IDs to exclude
     * @return Constructed Graph
     */
    Graph buildGraph(int mode, const std::vector<int>& excludedReviewerIds = {}) const;

    /**
     * @brief Extracts assignments from the flow graph after max-flow has been run.
     * @param g Graph after flow computation
     * @param subOffset Offset of first submission node in graph
     * @param revOffset Offset of first reviewer node in graph
     * @param mode Match mode (for determining match domain)
     */
    std::vector<Assignment> extractAssignments(const Graph& g, int subOffset, int revOffset, int mode) const;

    /// @brief Required total flow for a complete assignment.
    int requiredFlow() const;
};

#endif // SOLVER_H
