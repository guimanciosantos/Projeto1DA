#include <iostream>
#include <string>
#include "DataLoader.h"
#include "NetworkManager.h"

using namespace std;

// Function declarations - Notice we now pass the DataLoader by reference (&)
void runInteractiveMode(DataLoader& loader);
void runBatchMode(const string& inputFile, const string& outputFile, DataLoader& loader);

int main(int argc, char* argv[]) {
    // We declare the loader here so BOTH modes can use the exact same data!
    DataLoader loader;

    // 1. Check for Batch Mode Execution
    if (argc == 4 && string(argv[1]) == "-b") {
        runBatchMode(argv[2], argv[3], loader);
    }
    // 2. Check for Interactive Mode Execution
    else if (argc == 1) {
        runInteractiveMode(loader);
    }
    // 3. Handle Invalid Arguments
    else {
        cerr << "Error: Invalid command line arguments." << endl;
        cerr << "Usage for Interactive Mode: ./myProg" << endl;
        cerr << "Usage for Batch Mode: ./myProg -b input.csv output.csv" << endl;
        return 1;
    }

    return 0;
}

void runInteractiveMode(DataLoader& loader) {
    int choice = -1;

    while (choice != 0) {
        cout << "\n=====================================================\n";
        cout << "   Scientific Conference Organization Tool (DA 2026)\n";
        cout << "=====================================================\n";
        cout << "1. Read Data and Build Graph (T1.2)\n";
        cout << "2. Generate Review Assignments (T2.1)\n";
        cout << "3. Run Risk Analysis\n";
        cout << "0. Exit\n";
        cout << "=====================================================\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice)) {
            cerr << "Error: Invalid input. Please enter a number." << endl;
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {
            case 1: { // The curly braces are required here in C++!
                string filename;
                cout << "Enter the dataset filename (e.g., input.csv): ";
                cin >> filename;

                if (loader.loadData(filename)) {
                    cout << "\nData successfully loaded!\n";
                    cout << "Submissions: " << loader.submissions.size() << "\n";
                    cout << "Reviewers: " << loader.reviewers.size() << "\n";
                    cout << "Building Network Flow Graph...\n";

                    // Build the graph using the function from NetworkManager
                    Graph<string> myGraph = buildGraph(loader.submissions, loader.reviewers, loader.config);

                    cout << "Success! Graph built with " << myGraph.getNumVertex() << " vertices.\n";
                }
                break;
            }
            case 2:
                if (loader.submissions.empty()) {
                    cerr << "Error: You must Read Data (Option 1) before generating assignments!\n";
                } else {
                    cout << "Generating Review Assignments using Edmonds-Karp Max Flow...\n";

                    // 1. Rebuild base graph
                    Graph<string> myGraph = buildGraph(loader.submissions, loader.reviewers, loader.config);

                    // 2. Run Max Flow
                    edmondsKarp(myGraph, "SOURCE", "SINK");

                    // 3. Check for Risk Analysis (Task 2.2)
                    vector<int> riskyReviewers;
                    if (loader.config.riskAnalysis == 1) {
                        cout << "Risk Analysis = 1 is enabled. Simulating reviewer dropouts...\n";
                        riskyReviewers = runRiskAnalysis1(loader);
                    }

                    // 4. Extract and Output Results
                    generateAssignmentsOutput(myGraph, loader, riskyReviewers);
                }
                break;
            case 3:
                cout << "Feature coming soon: Risk Analysis...\n";
                break;
            case 0:
                cout << "Exiting the tool. Goodbye!\n";
                break;
            default:
                cerr << "Error: Invalid choice. Please select a valid option.\n";
        }
    }
}

void runBatchMode(const string& inputFile, const string& outputFile, DataLoader& loader) {
    cout << "Running in Batch Mode..." << endl;

    // In batch mode, we load the file automatically without asking the user
    if (loader.loadData(inputFile)) {
        Graph<string> myGraph = buildGraph(loader.submissions, loader.reviewers, loader.config);
        cout << "Graph built successfully in batch mode.\n";
        // Later, we will redirect the algorithm outputs to outputFile here! [cite: 121]
    } else {
        cerr << "Batch Mode Error: Failed to load " << inputFile << endl;
    }
}