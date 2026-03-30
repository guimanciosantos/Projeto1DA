#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
using namespace std;

// Structs to hold our parsed CSV data
struct Submission {
    int id;
    string title;
    string authors;
    string email;
    int primaryDomain;
    int secondaryDomain;
};

struct Reviewer {
    int id;
    string name;
    string email;
    int primaryExpertise;
    int secondaryExpertise;
};

struct AppConfig {
    int minReviewsPerSubmission = 0;
    int maxReviewsPerReviewer = 0;
    int generateAssignments = 0;
    int riskAnalysis = 0;
    string outputFileName = "output.csv";
};

/**
 * @brief  le e mantem os dados do ficheiro de entrada
 *
 * Mantém vetores de submissões e revisores, e a configuração da aplicação.
 */
class DataLoader {
public:
    std::vector<Submission> submissions;
    std::vector<Reviewer> reviewers;
    AppConfig config;

    /**
     * @brief Carrega dados a partir de um ficheiro 
     *
     * @param filename Caminho do ficheiro de input
     * @return true se o ficheiro foi carregado com sucesso, false caso contrário.
     */
    bool loadData(const std::string& filename);
};

#endif