#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>

/**
 * @brief Represents a paper submission.
 */
struct Submission {
    int id;                  ///< Unique paper ID
    std::string title;       ///< Paper title
    std::string authors;     ///< Author(s)
    std::string email;       ///< Contact e-mail
    int primaryTopic;        ///< Primary topic identifier
    int secondaryTopic;      ///< Secondary topic identifier (0 = none)
    int graphNode;           ///< Corresponding node index in the flow graph
};

/**
 * @brief Represents a reviewer.
 */
struct Reviewer {
    int id;                  ///< Unique reviewer ID
    std::string name;        ///< Full name
    std::string email;       ///< Contact e-mail
    int primaryExpertise;    ///< Primary domain of expertise
    int secondaryExpertise;  ///< Secondary domain of expertise (0 = none)
    int graphNode;           ///< Corresponding node index in the flow graph
};

/**
 * @brief Parameters read from the #Parameters section of the input file.
 */
struct Parameters {
    int minReviewsPerSubmission  = 1;
    int maxReviewsPerReviewer    = 3;
    int primaryReviewerExpertise = 1; ///< Weight/flag for primary expertise
    int secondaryReviewerExpertise = 0;
    int primarySubmissionDomain  = 1;
    int secondarySubmissionDomain = 0;
};

/**
 * @brief Control options read from the #Control section.
 */
struct ControlOptions {
    int generateAssignments = 1;   ///< 0-3 (see spec)
    int riskAnalysis        = 0;   ///< 0, 1, or K>1
    std::string outputFileName = "output.csv";
};

/**
 * @brief Represents a single review assignment (submission -> reviewer).
 */
struct Assignment {
    int submissionId;
    int reviewerId;
    int matchDomain; ///< The topic domain on which the match was made
};

#endif // TYPES_H
