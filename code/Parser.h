#ifndef PARSER_H
#define PARSER_H

#include "Types.h"
#include <string>
#include <vector>

/**
 * @brief Parses the project's CSV input file format.
 *
 * Handles all four sections: #Submissions, #Reviewers, #Parameters, #Control.
 * Lines starting with '#' that are not section markers are treated as comments.
 * Fields after a '#' character on a data line are ignored.
 */
class Parser {
public:
    /**
     * @brief Parses the given CSV file.
     * @param filename Path to the input .csv file
     * @param submissions Output vector of parsed submissions
     * @param reviewers   Output vector of parsed reviewers
     * @param params      Output parameters struct
     * @param control     Output control options struct
     * @return true if parsing succeeded without errors; false otherwise
     * Time Complexity: O(N + M) where N = #submissions, M = #reviewers
     */
    static bool parse(const std::string& filename,
                      std::vector<Submission>& submissions,
                      std::vector<Reviewer>& reviewers,
                      Parameters& params,
                      ControlOptions& control);

private:
    /// @brief Strips inline comments (everything from '#' onward) and trims whitespace.
    static std::string stripComment(const std::string& line);

    /// @brief Splits a string by commas, respecting quoted fields.
    static std::vector<std::string> splitCSV(const std::string& line);

    /// @brief Trims leading/trailing whitespace and removes surrounding quotes.
    static std::string trim(const std::string& s);
};

#endif // PARSER_H
