#include "Parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>

// ──────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────

std::string Parser::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n\"");
    size_t end   = s.find_last_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

std::string Parser::stripComment(const std::string& line) {
    // Find '#' that is NOT at the start of the line (section markers begin with #)
    // Any '#' AFTER position 0 starts an inline comment.
    bool inQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '"') inQuote = !inQuote;
        if (!inQuote && line[i] == '#' && i > 0) {
            return line.substr(0, i);
        }
    }
    return line;
}

std::vector<std::string> Parser::splitCSV(const std::string& line) {
    std::vector<std::string> result;
    std::string field;
    bool inQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuote = !inQuote;
        } else if (c == ',' && !inQuote) {
            result.push_back(trim(field));
            field.clear();
        } else {
            field += c;
        }
    }
    result.push_back(trim(field));
    return result;
}

// ──────────────────────────────────────────────────────────────
// Main parse function
// Time Complexity: O(N + M) where N=#submissions, M=#reviewers
// ──────────────────────────────────────────────────────────────

bool Parser::parse(const std::string& filename,
                   std::vector<Submission>& submissions,
                   std::vector<Reviewer>&   reviewers,
                   Parameters&              params,
                   ControlOptions&          control) {

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open file '" << filename << "'\n";
        return false;
    }

    enum class Section { NONE, SUBMISSIONS, REVIEWERS, PARAMETERS, CONTROL };
    Section current = Section::NONE;

    std::set<int> subIds, revIds;
    bool hasErrors = false;
    int lineNo = 0;
    std::string line;

    while (std::getline(file, line)) {
        ++lineNo;
        line = stripComment(line);
        line = trim(line);
        if (line.empty()) continue;

        // ── Section markers ──────────────────────────────────
        if (line == "#Submissions")  { current = Section::SUBMISSIONS;  continue; }
        if (line == "#Reviewers")    { current = Section::REVIEWERS;    continue; }
        if (line == "#Parameters")   { current = Section::PARAMETERS;   continue; }
        if (line == "#Control")      { current = Section::CONTROL;      continue; }
        if (line == "#")             { current = Section::NONE;         continue; }

        // Lines that start with '#' inside a section are column headers — skip
        if (line[0] == '#') continue;

        auto fields = splitCSV(line);

        // ── Submissions ──────────────────────────────────────
        if (current == Section::SUBMISSIONS) {
            if (fields.size() < 5) {
                std::cerr << "Warning (line " << lineNo << "): submission has fewer than 5 fields — skipped\n";
                continue;
            }
            Submission s;
            try {
                s.id      = std::stoi(fields[0]);
                s.title   = fields[1];
                s.authors = fields[2];
                s.email   = fields[3];
                s.primaryTopic   = std::stoi(fields[4]);
                s.secondaryTopic = (fields.size() > 5 && !fields[5].empty()) ? std::stoi(fields[5]) : 0;
                s.graphNode      = -1;
            } catch (...) {
                std::cerr << "Error (line " << lineNo << "): invalid submission data\n";
                hasErrors = true; continue;
            }
            if (subIds.count(s.id)) {
                std::cerr << "Error: duplicate submission ID " << s.id << "\n";
                hasErrors = true; continue;
            }
            subIds.insert(s.id);
            submissions.push_back(s);
        }

        // ── Reviewers ────────────────────────────────────────
        else if (current == Section::REVIEWERS) {
            if (fields.size() < 4) {
                std::cerr << "Warning (line " << lineNo << "): reviewer has fewer than 4 fields — skipped\n";
                continue;
            }
            Reviewer r;
            try {
                r.id    = std::stoi(fields[0]);
                r.name  = fields[1];
                r.email = fields[2];
                r.primaryExpertise   = std::stoi(fields[3]);
                r.secondaryExpertise = (fields.size() > 4 && !fields[4].empty()) ? std::stoi(fields[4]) : 0;
                r.graphNode          = -1;
            } catch (...) {
                std::cerr << "Error (line " << lineNo << "): invalid reviewer data\n";
                hasErrors = true; continue;
            }
            if (revIds.count(r.id)) {
                std::cerr << "Error: duplicate reviewer ID " << r.id << "\n";
                hasErrors = true; continue;
            }
            revIds.insert(r.id);
            reviewers.push_back(r);
        }

        // ── Parameters ───────────────────────────────────────
        else if (current == Section::PARAMETERS) {
            if (fields.size() < 2) continue;
            std::string key = fields[0];
            std::string val = fields[1];
            try {
                if (key == "MinReviewsPerSubmission")    params.minReviewsPerSubmission  = std::stoi(val);
                else if (key == "MaxReviewsPerReviewer") params.maxReviewsPerReviewer    = std::stoi(val);
                else if (key == "PrimaryReviewerExpertise")   params.primaryReviewerExpertise   = std::stoi(val);
                else if (key == "SecondaryReviewerExpertise") params.secondaryReviewerExpertise = std::stoi(val);
                else if (key == "PrimarySubmissionDomain")    params.primarySubmissionDomain    = std::stoi(val);
                else if (key == "SecondarySubmissionDomain")  params.secondarySubmissionDomain  = std::stoi(val);
                else std::cerr << "Warning (line " << lineNo << "): unknown parameter '" << key << "'\n";
            } catch (...) {
                std::cerr << "Error (line " << lineNo << "): invalid parameter value for '" << key << "'\n";
                hasErrors = true;
            }
        }

        // ── Control ──────────────────────────────────────────
        else if (current == Section::CONTROL) {
            if (fields.size() < 2) continue;
            std::string key = fields[0];
            std::string val = fields[1];
            try {
                if (key == "GenerateAssignments") control.generateAssignments = std::stoi(val);
                else if (key == "RiskAnalysis")   control.riskAnalysis        = std::stoi(val);
                else if (key == "OutputFileName") control.outputFileName       = val;
                else std::cerr << "Warning (line " << lineNo << "): unknown control key '" << key << "'\n";
            } catch (...) {
                std::cerr << "Error (line " << lineNo << "): invalid control value for '" << key << "'\n";
                hasErrors = true;
            }
        }
    }

    if (submissions.empty()) {
        std::cerr << "Error: no submissions found in file.\n";
        hasErrors = true;
    }
    if (reviewers.empty()) {
        std::cerr << "Error: no reviewers found in file.\n";
        hasErrors = true;
    }

    return !hasErrors;
}
