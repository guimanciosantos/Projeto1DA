#include "Menu.h"
#include "Parser.h"
#include "Solver.h"
#include <iostream>
#include <string>
#include <map>

/**
 * @brief Entry point for the Scientific Conference Assignment Tool.
 *
 * Usage:
 *   Interactive mode:  ./DA_Project1
 *   Batch mode:        ./DA_Project1 -b input.csv output.csv
 *
 * In batch mode, all output (assignments + risk analysis) is written to output.csv.
 * Error messages are directed to stderr.
 */
int main(int argc, char* argv[]) {

    // ── Batch mode ────────────────────────────────────────────
    if (argc >= 3 && std::string(argv[1]) == "-b") {
        std::string inputFile  = argv[2];
        std::string outputFile = (argc >= 4) ? argv[3] : "output.csv";

        std::vector<Submission> submissions;
        std::vector<Reviewer>   reviewers;
        Parameters              params;
        ControlOptions          control;
        control.outputFileName = outputFile;

        bool ok = Parser::parse(inputFile, submissions, reviewers, params, control);
        if (!ok) {
            std::cerr << "Fatal: failed to parse input file.\n";
            return 1;
        }

        int mode = (control.generateAssignments == 0) ? 1 : control.generateAssignments;
        Solver solver(submissions, reviewers, params, control);
        auto assignments = solver.solve(mode);

        std::vector<std::vector<int>> riskGroups;
        if (control.riskAnalysis > 0) {
            riskGroups = solver.riskAnalysis(control.riskAnalysis, assignments);
        }

        solver.writeOutput(outputFile, assignments, {}, riskGroups, control.riskAnalysis);
        return 0;
    }

    // ── Interactive mode ──────────────────────────────────────
    if (argc > 1 && std::string(argv[1]) != "-b") {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << "                       (interactive)\n"
                  << "  " << argv[0] << " -b input.csv [out.csv] (batch)\n";
        return 1;
    }

    Menu menu;
    menu.run();
    return 0;
}
