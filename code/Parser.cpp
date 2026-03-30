#include "Parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>

using namespace std;

/**
 * @brief Remove espaços em branco e aspas no início e no fim 
 *
 * @param str String de entrada possivelmente com espaços e aspas.
 * @return string sem espaços ou aspas em ambas extremidades.
 */
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\"");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\"");
    return str.substr(first, (last - first + 1));
}

/**
 * @brief Carrega os dados e estrutura em seções
 *
 * Para cada linha válida, preenche 'submissions', 'reviewers' e 'config'.
 * Ignora comentários (tudo após '#')
 *
 * @param filename Caminho do ficheiro a carregar.
 * @return true se o ficheiro foi carregado com sucesso; false em erro.
 */
bool DataLoader::loadData(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro: Não foi possível abrir o ficheiro " << filename << endl;
        return false;
    }

    submissions.clear();
    reviewers.clear();
    config = AppConfig();

    string line, currentSection = "";
    unordered_set<int> seenSubIds, seenRevIds;

    while (getline(file, line)) {
        if (line.find("#Submissions") == 0) { currentSection = "Submissions"; continue; }
        if (line.find("#Reviewers") == 0) { currentSection = "Reviewers"; continue; }
        if (line.find("#Parameters") == 0) { currentSection = "Parameters"; continue; }
        if (line.find("#Control") == 0) { currentSection = "Control"; continue; }

        size_t hashPos = line.find('#');
        if (hashPos != string::npos) {
            line = line.substr(0, hashPos);
        }

        line = trim(line);
        if (line.empty()) continue;

        stringstream ss(line);

        
        if (currentSection == "Submissions") {
            Submission sub;
            string idStr, priStr, secStr;
            getline(ss, idStr, ','); getline(ss, sub.title, ','); getline(ss, sub.authors, ',');
            getline(ss, sub.email, ','); getline(ss, priStr, ','); getline(ss, secStr, ',');

            sub.id = stoi(trim(idStr));
            sub.primaryDomain = stoi(trim(priStr));
            sub.secondaryDomain = trim(secStr).empty() ? 0 : stoi(trim(secStr));

            if (!seenSubIds.insert(sub.id).second) {
                cerr << "Error: Duplicate Submission ID " << sub.id << endl;
                return false;
            }
            submissions.push_back(sub);
        }
        else if (currentSection == "Reviewers") {
            Reviewer rev;
            string idStr, priStr, secStr;
            getline(ss, idStr, ','); getline(ss, rev.name, ','); getline(ss, rev.email, ',');
            getline(ss, priStr, ','); getline(ss, secStr, ',');

            rev.id = stoi(trim(idStr));
            rev.primaryExpertise = stoi(trim(priStr));
            if (trim(secStr).empty()) {
                rev.secondaryExpertise = 0;
            } else {
                rev.secondaryExpertise = stoi(trim(secStr));
            }

            if (!seenRevIds.insert(rev.id).second) {
                cerr << "Error: Duplicate Reviewer ID " << rev.id << endl;
                return false;
            }
            reviewers.push_back(rev);
        }
        else if (currentSection == "Parameters" || currentSection == "Control") {
            string key, valStr;
            getline(ss, key, ','); getline(ss, valStr, ',');
            key = trim(key); valStr = trim(valStr);

            if (key == "MinReviewsPerSubmission") config.minReviewsPerSubmission = stoi(valStr);
            else if (key == "MaxReviewsPerReviewer") config.maxReviewsPerReviewer = stoi(valStr);
            else if (key == "GenerateAssignments") config.generateAssignments = stoi(valStr);
            else if (key == "RiskAnalysis") config.riskAnalysis = stoi(valStr);
            else if (key == "OutputFileName") config.outputFileName = valStr;
        }
    }
    return true;
}