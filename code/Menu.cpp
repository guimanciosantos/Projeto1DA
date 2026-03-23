#include "Menu.h"
#include "Parser.h"
#include <iostream>
#include <limits>

// ──────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────

Menu::Menu() : dataLoaded(false) {}

// ──────────────────────────────────────────────────────────────
// run — main interactive loop
// ──────────────────────────────────────────────────────────────

void Menu::run() {
    printHeader();
    while (true) {
        printSeparator();
        std::cout << "MAIN MENU\n";
        printSeparator();
        std::cout << " 1. Load input file\n";
        std::cout << " 2. List submissions\n";
        std::cout << " 3. List reviewers\n";
        std::cout << " 4. Show parameters\n";
        std::cout << " 5. Run assignment\n";
        std::cout << " 6. Run risk analysis\n";
        std::cout << " 0. Exit\n";
        printSeparator();

        int choice = readInt("Select option: ");
        switch (choice) {
            case 1: loadFile();         break;
            case 2: listSubmissions();  break;
            case 3: listReviewers();    break;
            case 4: listParameters();   break;
            case 5: runAssignment();    break;
            case 6: runRiskAnalysis();  break;
            case 0:
                std::cout << "Goodbye!\n";
                return;
            default:
                std::cout << "Invalid option. Please try again.\n";
        }
        waitForEnter();
    }
}

// ──────────────────────────────────────────────────────────────
// loadFile
// ──────────────────────────────────────────────────────────────

void Menu::loadFile() {
    std::string filename = readString("Enter path to input .csv file: ");
    submissions.clear();
    reviewers.clear();
    params   = Parameters{};
    control  = ControlOptions{};

    bool ok = Parser::parse(filename, submissions, reviewers, params, control);
    if (ok) {
        dataLoaded = true;
        std::cout << "File loaded successfully.\n";
        std::cout << "  Submissions: " << submissions.size() << "\n";
        std::cout << "  Reviewers:   " << reviewers.size()   << "\n";
    } else {
        std::cerr << "File loading failed. Please check the errors above.\n";
        dataLoaded = false;
    }
}

// ──────────────────────────────────────────────────────────────
// listSubmissions
// ──────────────────────────────────────────────────────────────

void Menu::listSubmissions() const {
    if (!dataLoaded) { std::cout << "No data loaded.\n"; return; }
    std::cout << "\n--- Submissions (" << submissions.size() << ") ---\n";
    for (const auto& s : submissions) {
        std::cout << "  [" << s.id << "] \"" << s.title << "\""
                  << " | Author: " << s.authors
                  << " | Primary: " << s.primaryTopic;
        if (s.secondaryTopic) std::cout << " | Secondary: " << s.secondaryTopic;
        std::cout << "\n";
    }
}

// ──────────────────────────────────────────────────────────────
// listReviewers
// ──────────────────────────────────────────────────────────────

void Menu::listReviewers() const {
    if (!dataLoaded) { std::cout << "No data loaded.\n"; return; }
    std::cout << "\n--- Reviewers (" << reviewers.size() << ") ---\n";
    for (const auto& r : reviewers) {
        std::cout << "  [" << r.id << "] " << r.name
                  << " | Primary expertise: " << r.primaryExpertise;
        if (r.secondaryExpertise) std::cout << " | Secondary: " << r.secondaryExpertise;
        std::cout << "\n";
    }
}

// ──────────────────────────────────────────────────────────────
// listParameters
// ──────────────────────────────────────────────────────────────

void Menu::listParameters() const {
    if (!dataLoaded) { std::cout << "No data loaded.\n"; return; }
    std::cout << "\n--- Parameters ---\n";
    std::cout << "  MinReviewsPerSubmission:    " << params.minReviewsPerSubmission  << "\n";
    std::cout << "  MaxReviewsPerReviewer:      " << params.maxReviewsPerReviewer    << "\n";
    std::cout << "  PrimaryReviewerExpertise:   " << params.primaryReviewerExpertise << "\n";
    std::cout << "  SecondaryReviewerExpertise: " << params.secondaryReviewerExpertise << "\n";
    std::cout << "  PrimarySubmissionDomain:    " << params.primarySubmissionDomain  << "\n";
    std::cout << "  SecondarySubmissionDomain:  " << params.secondarySubmissionDomain << "\n";
    std::cout << "\n--- Control ---\n";
    std::cout << "  GenerateAssignments: " << control.generateAssignments << "\n";
    std::cout << "  RiskAnalysis:        " << control.riskAnalysis        << "\n";
    std::cout << "  OutputFileName:      " << control.outputFileName      << "\n";
}

// ──────────────────────────────────────────────────────────────
// runAssignment
// ──────────────────────────────────────────────────────────────

void Menu::runAssignment() {
    if (!dataLoaded) { std::cout << "No data loaded.\n"; return; }

    int mode = control.generateAssignments;
    if (mode == 0) {
        std::cout << "GenerateAssignments=0: running assignment without reporting.\n";
        mode = 1; // still run to check feasibility
    }

    Solver solver(submissions, reviewers, params, control);
    auto assignments = solver.solve(mode);

    solver.printAssignments(assignments);

    if (control.generateAssignments != 0) {
        saveResults(assignments, {});
    }
}

// ──────────────────────────────────────────────────────────────
// runRiskAnalysis
// ──────────────────────────────────────────────────────────────

void Menu::runRiskAnalysis() {
    if (!dataLoaded) { std::cout << "No data loaded.\n"; return; }
    if (control.riskAnalysis == 0) {
        std::cout << "RiskAnalysis=0: no risk analysis configured.\n";
        return;
    }

    int mode = (control.generateAssignments == 0) ? 1 : control.generateAssignments;
    Solver solver(submissions, reviewers, params, control);
    auto assignments = solver.solve(mode);
    int K = control.riskAnalysis;

    std::cout << "Running risk analysis with K=" << K << "...\n";
    auto riskGroups = solver.riskAnalysis(K, assignments);

    if (riskGroups.empty()) {
        std::cout << "No risky reviewer group found. Assignment is robust.\n";
    } else {
        std::cout << "#Risk Analysis: " << K << "\n";
        for (const auto& group : riskGroups) {
            for (int id : group) std::cout << id << " ";
            std::cout << "\n";
        }
    }

    saveResults(assignments, riskGroups);
}

// ──────────────────────────────────────────────────────────────
// saveResults
// ──────────────────────────────────────────────────────────────

void Menu::saveResults(const std::vector<Assignment>& assignments,
                       const std::vector<std::vector<int>>& riskGroups) {
    Solver solver(submissions, reviewers, params, control);
    solver.writeOutput(control.outputFileName, assignments, {}, riskGroups, control.riskAnalysis);
}

// ──────────────────────────────────────────────────────────────
// Utility helpers
// ──────────────────────────────────────────────────────────────

void Menu::printHeader() const {
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║   Scientific Conference Assignment Tool  (DA)    ║\n";
    std::cout << "║   FEUP — L.EIC016 — Spring 2026                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";
}

void Menu::printSeparator() const {
    std::cout << "──────────────────────────────────────────────────\n";
}

void Menu::waitForEnter() const {
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int Menu::readInt(const std::string& prompt) const {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a valid integer.\n";
    }
}

std::string Menu::readString(const std::string& prompt) const {
    std::string s;
    std::cout << prompt;
    std::getline(std::cin, s);
    return s;
}
