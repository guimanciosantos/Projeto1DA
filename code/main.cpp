#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "Parser.h"
#include "Solver.h"

using namespace std;

/**
 * @brief Declarações de funções para os dois modos de execução
 */
void runInteractiveMode(DataLoader& loader);
void runBatchMode(const string& inputFile, const string& outputFile, DataLoader& loader);

/**
 * @brief Função principal,gere os modos de execução
 *
 * Interpreta argumentos 
 * - Sem argumentos: ativa modo interativo
 * - Com "-b input.csv output.csv": ativa o modo batch
 * - Argumentos inválidos: exibe mensagem de erro
 *
 * @param argc Número de argumentos de linha de comando
 * @param argv Vetor de argumentos de linha de comando
 * @return 0 se execução bem-sucedida; 1 em caso de erro
 */
int main(int argc, char* argv[]) {
    DataLoader loader;

    // verifica se o modo batch foi executado
    if (argc == 4 && string(argv[1]) == "-b") {
        runBatchMode(argv[2], argv[3], loader);
    }
    // modo interativo
    else if (argc == 1) {
        runInteractiveMode(loader);
    }
    // argumentos inválidos
    else {
        cerr << "Erro: Argumentos de linha de comando inválidos." << endl;
        cerr << "Uso para modo interativo: ./myProg" << endl;
        cerr << "Uso para modo batch: ./myProg -b input.csv output.csv" << endl;
        return 1;
    }

    return 0;
}

/**
 * @brief Executa o modo interativo com menu de opções
 *
 * Opções disponíveis:
 * 1. Ler dados e construir grafo 
 * 2. Ver dados carregados e parâmetros 
 * 3. Gerar atribuições de revisão 
 * 0. Sair
 *
 * @param loader Referência ao carregador de dados (DataLoader)
 */
void runInteractiveMode(DataLoader& loader) {
    int choice = -1;

    while (choice != 0) {
        cout << "\n-----------------------------------------------------\n";
        cout << "   Ferramenta de Organização  \n";
        cout << "-----------------------------------------------------\n";
        cout << "1. Ler dados e construir grafo \n";
        cout << "2. Ver dados carregados e parâmetros \n";
        cout << "3. Gerar atribuições de revisão \n";
        cout << "0. Sair\n";
        cout << "-----------------------------------------------------\n";
        cout << "Escolha uma opção: ";

        if (!(cin >> choice)) {
            cerr << "Erro: número inválido. insira um número válido." << endl;
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {
            case 1: {
                string filename;
                //nome se estiver o ficheiro na pasta, senao escrever o caminho completo
                cout << "Insira o nome do ficheiro de dados (ex: input.csv): ";
                cin >> filename;

                if (loader.loadData(filename)) {
                    cout << "\nDados carregados com sucesso\n";
                    cout << "Submissões: " << loader.submissions.size() << "\n";
                    cout << "Revisores: " << loader.reviewers.size() << "\n";
                }
                break;
            }
            case 2: { 
                if (loader.submissions.empty()) {
                    cerr << "Erro: use a Opção 1 primeiro\n";
                    break;
                }

                int subChoice = -1;
                while (subChoice != 0) {
                    cout << "\n--- Ver dados carregados ---\n";
                    cout << "1. Parâmetros atuais de configuração\n";
                    cout << "2. Lista de submissões\n";
                    cout << "3. Lista de revisores\n";
                    cout << "0. Voltar ao menu\n";
                    cout << "------------------------\n";
                    cout << "Selecione opção: ";

                    if (!(cin >> subChoice)) {
                        cerr << "Erro: Entrada inválida.\n";
                        cin.clear(); cin.ignore(10000, '\n');
                        continue;
                    }

                    switch (subChoice) {
                        case 1:
                            cout << "\n[ PARÂMETROS ATUAIS DE CONFIGURAÇÃO ]\n";
                            cout << "- Revisões mínimas por submissão: " << loader.config.minReviewsPerSubmission << "\n";
                            cout << "- Revisões máximas por revisor: " << loader.config.maxReviewsPerReviewer << "\n";
                            cout << "- Análise de risco ativada: ";
                            if (loader.config.riskAnalysis == 1) {
                                cout << "Sim\n";
                            } else {
                                cout << "Não\n";
                            }
                            break;
                        case 2:
                            cout << "\n[ LISTA DE SUBMISSÕES (Total: " << loader.submissions.size() << ") ]\n";
                            for (const auto& sub : loader.submissions) {
                                cout << "ID: " << sub.id << " | Domínio principal: " << sub.primaryDomain
                                     << " | Título: " << sub.title << "\n";
                            }
                            break;
                        case 3:
                            cout << "\n[ LISTA DE REVISORES (Total: " << loader.reviewers.size() << ") ]\n";
                            for (const auto& rev : loader.reviewers) {
                                cout << "ID: " << rev.id << " | Especialidade principal: " << rev.primaryExpertise
                                     << " | Nome: " << rev.name << "\n";
                            }
                            break;
                        case 0:
                            cout << "Regressando ao menu principal...\n";
                            break;
                        default:
                            cerr << "Erro: Escolha inválida.\n";
                    }
                }
                break;
            }
            case 3: {
                if (loader.submissions.empty()) {
                    cerr << "Erro: Deves ler os dados (Opção 1) antes de gerar as atribuições!\n";
                } else {
                    cout << "Construindo grafo...\n";
                    Graph<string> myGraph = buildGraph(loader.submissions, loader.reviewers, loader.config);

                    cout << "gerando revisões usando Edmonds-Karp Max Flow...\n";
                    edmondsKarp(myGraph, "SOURCE", "SINK");

                    // Imprimir os caminhos completos na consola (T1.1)
                    cout << "\n--- Caminhos Completos (Source -> Sink) ---\n";
                    for (const auto& sub : loader.submissions) {
                        string subNode = "SUB_" + to_string(sub.id);
                        auto v = myGraph.findVertex(subNode);
                        if (v) {
                            for (auto e : v->getAdj()) {
                                if (e->getFlow() > 0 && e->getDest()->getInfo().find("REV_") == 0) {
                                    cout << "SOURCE -> " << subNode << " -> " << e->getDest()->getInfo() << " -> SINK\n";
                                }
                            }
                        }
                    }
                    cout << "---------------------------------------------------\n";

                    vector<int> riskyReviewers;
                    if (loader.config.riskAnalysis == 1) {
                        cout << "Análise de risco = 1 ativada. A simular ausências de revisores...\n";
                        riskyReviewers = runRiskAnalysis1(loader);
                    }

                    generateAssignmentsOutput(myGraph, loader, riskyReviewers);
                }
                break;
            }
            case 0:
                cout << "Saindo da ferramenta\n";
                break;
            default:
                cerr << "Erro: número inválido. Por favor selecione um número válido.\n";
        }
    }
}

/**
 * @brief Executa a ferramenta em modo batch 
 *  
 * Lê o ficheiro , contrói o grafo, calcula o fluxo máximo,
 * executa análise de risco (quando pedido), e mete os resultados nos ficheiros de saída
 *
 * @param inputFile Caminho do ficheiro de entrada
 * @param outputFile Caminho do ficheiro de saída
 * @param loader Referência ao carregador de dados (modificado com dados carregados)
 */
void runBatchMode(const string& inputFile, const string& outputFile, DataLoader& loader) {
    if (loader.loadData(inputFile)) {
        loader.config.outputFileName = outputFile;

        Graph<string> myGraph = buildGraph(loader.submissions, loader.reviewers, loader.config);
        edmondsKarp(myGraph, "SOURCE", "SINK");

        vector<int> riskyReviewers;
        if (loader.config.riskAnalysis == 1) {
            riskyReviewers = runRiskAnalysis1(loader);
        }

        generateAssignmentsOutput(myGraph, loader, riskyReviewers);
    } else {
        cerr << "Erro: Falha ao carregar " << inputFile << " em modo batch." << endl;
    }
}
