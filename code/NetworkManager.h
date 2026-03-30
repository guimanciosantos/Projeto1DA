#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "DataLoader.h"

using namespace std;

// Builds the initial graph
Graph<string> buildGraph(const vector<Submission>& subs, const vector<Reviewer>& revs, const AppConfig& config);

// Edmonds-Karp Max Flow
bool findAugmentingPath(Graph<string>& g, string s, string t);
void edmondsKarp(Graph<string>& g, string s, string t);

// --- NEW TASK 2.2: Risk Analysis = 1 ---
vector<int> runRiskAnalysis1(const DataLoader& data);

// Output Formatter
void generateAssignmentsOutput(Graph<string>& g, const DataLoader& data, const vector<int>& riskyReviewers);

#endif