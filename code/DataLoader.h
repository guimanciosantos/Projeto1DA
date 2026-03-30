#ifndef DATALOADER_H
#define DATALOADER_H

#include <string>
#include <vector>

// Structs to hold our parsed CSV data
struct Submission {
    int id;
    std::string title;
    std::string authors;
    std::string email;
    int primaryDomain;
    int secondaryDomain;
};

struct Reviewer {
    int id;
    std::string name;
    std::string email;
    int primaryExpertise;
    int secondaryExpertise;
};

struct AppConfig {
    int minReviewsPerSubmission = 0;
    int maxReviewsPerReviewer = 0;
    int generateAssignments = 0;
    int riskAnalysis = 0;
    std::string outputFileName = "output.csv";
};

// The class that actually loads the data
class DataLoader {
public:
    std::vector<Submission> submissions;
    std::vector<Reviewer> reviewers;
    AppConfig config;

    // Main function to load the file
    bool loadData(const std::string& filename);
};

#endif