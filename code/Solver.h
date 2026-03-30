#ifndef SOLVER_H
#define SOLVER_H

#include <string>
#include <vector>
#include "Graph.h"
#include "Parser.h"

using namespace std;

/**
 * @brief Constrói o grafo de fluxo para atribuição de revisões
 *
 * @param subms Vetor de submissões
 * @param revs Vetor de revisores
 * @param config Configuração com limites de revisões
 * @return Grafo direcionado com capacidades para fluxo máximo
 */
Graph<string> buildGraph(const vector<Submission>& subms, const vector<Reviewer>& revs, const AppConfig& config);

/**
 * @brief Encontra um caminho aumentador da origem para o destino usando BFS
 *
 * @param g Referência ao grafo de fluxo
 * @param s source
 * @param t sink
 * @return true se um caminho aumentador foi encontrado, false caso contrário.
 */
bool findAugmentingPath(Graph<string>& g, string s, string t);

/**
 * @brief Implementa o algoritmo Edmonds-Karp para fluxo máximo
 *
 * @param g Referência ao grafo de fluxo (modificado com fluxos)
 * @param s source
 * @param t sink
 */
void edmondsKarp(Graph<string>& g, string s, string t);

/**
 * @brief Identifica revisores críticos 
 *
 * @param data Dados carregados (submissões, revisores, configuração).
 * @return Vetor de IDs de revisores críticos (ordenado).
 */
vector<int> runRiskAnalysis1(const DataLoader& data);

/**
 * @brief Gera ficheiro de saída com atribuições e análises.
 *
 * @param g Grafo de fluxo com fluxos calculados
 * @param data Estrutura com dados de entrada e configuração
 * @param riskyReviewers Lista de IDs de revisores críticos
 */
void generateAssignmentsOutput(Graph<string>& g, const DataLoader& data, const vector<int>& riskyReviewers);

#endif