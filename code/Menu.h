#ifndef MENU_H
#define MENU_H

#include "Types.h"
#include "Solver.h"
#include <vector>
#include <string>

/**
 * @brief Interactive command-line menu for the Conference Assignment Tool.
 *
 * Provides a user-friendly interface to load datasets, view data,
 * run assignments, and perform risk analysis.
 */
class Menu {
public:
    Menu();

    /**
     * @brief Starts the interactive menu loop.
     */
    void run();

private:
    std::vector<Submission> submissions;
    std::vector<Reviewer>   reviewers;
    Parameters              params;
    ControlOptions          control;
    bool                    dataLoaded;

    // ---- Menu action handlers ----

    /// @brief Prompts user for a file path and loads/parses it.
    void loadFile();

    /// @brief Displays all loaded submissions.
    void listSubmissions() const;

    /// @brief Displays all loaded reviewers.
    void listReviewers() const;

    /// @brief Displays current parameter and control settings.
    void listParameters() const;

    /// @brief Runs the assignment (GenerateAssignments mode from control settings).
    void runAssignment();

    /// @brief Runs risk analysis (K from control settings).
    void runRiskAnalysis();

    /// @brief Saves results to the output file specified in control settings.
    void saveResults(const std::vector<Assignment>& assignments,
                     const std::vector<std::vector<int>>& riskGroups);

    // ---- Utilities ----
    void printSeparator() const;
    void waitForEnter() const;
    int  readInt(const std::string& prompt) const;
    std::string readString(const std::string& prompt) const;
    void printHeader() const;
};

#endif // MENU_H
