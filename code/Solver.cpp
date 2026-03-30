#include "Solver.h"
#include <iostream>
#include <queue>
#include <algorithm>
#include <fstream>

using namespace std;

/**
 * @brief Constrói o grafo de fluxo para atribuição de revisões
 *
 * Adiciona:
 * - Vértices SOURCE e SINK
 * - Vértices de submissões com arestas do SOURCE (capacidade = minReviewsPerSubmission)
 * - Vértices de revisores com arestas para SINK (capacidade = maxReviewsPerReviewer)
 * - Arestas entre submissões e revisores se os domínios coincidem (capacidade = 1)
 *
 * @param subms Vetor de submissões
 * @param revs Vetor de revisores
 * @param config Configuração com limites de revisões.
 * @return Grafo direcionado com capacidades para fluxo máximo
 */
Graph<string> buildGraph(const vector<Submission>& subms, const vector<Reviewer>& revs, const AppConfig& config) {
    Graph<string> g;
    
    //  nó sink e nó source
    g.addVertex("SOURCE");
    g.addVertex("SINK");

    // 2. Adiciona Submissions e conecta a partir do Source
    for (const auto& sub : subms) {
        string subNode = "SUB_" + to_string(sub.id);
        g.addVertex(subNode);
        
        // define a capacidade como minReviewsPerSubmission
        g.addDirectedFlowEdge("SOURCE", subNode, config.minReviewsPerSubmission);
    }

    // adiciona os revisores e conecta ao sink
    for (const auto& rev : revs) {
        string revNode = "REV_" + to_string(rev.id);
        g.addVertex(revNode);
        
        // aresta do revisor para o sink com capacidade maxReviewsPerReviewer
        g.addDirectedFlowEdge(revNode, "SINK", config.maxReviewsPerReviewer);
    }

    // conecta as submissões aos revisores com base nos seus domínios 
    for (const auto& sub : subms) {
        string subNode = "SUB_" + to_string(sub.id);
        for (const auto& rev : revs) {
            string revNode = "REV_" + to_string(rev.id);
            
            // se  domínio da submissão = especialidade do revisor, conecta com capacidade 1
            if (sub.primaryDomain == rev.primaryExpertise) {
                // capacidade = 1 (um revisor só pode ver um paper específico uma vez)
                g.addDirectedFlowEdge(subNode, revNode, 1); 
            }
        }
    }
    
    return g;
}

// ---------------------------------------------------------
// Implementação do Algoritmo Edmonds-Karp
// ---------------------------------------------------------

/**
 * @brief Encontra um caminho aumentador de origem para destino usando BFS
 *
 * Realiza uma busca em largura (BFS) para encontrar um caminho do SOURCE para SINK
 * onde ainda há capacidade disponível nas arestas.
 *
 * @param g Referência ao grafo de fluxo
 * @param s source
 * @param t sink
 * @return true se um caminho aumentador foi encontrado; false caso contrário.
 */
bool findAugmentingPath(Graph<string>& g, string s, string t) {
    // todos os vértices para não visitados e sem caminho
    for (auto v : g.getVertexSet()) {
        v->setVisited(false);
        v->setPath(nullptr);
    }

    auto sVertex = g.findVertex(s);
    sVertex->setVisited(true);
    queue<Vertex<string>*> q;
    q.push(sVertex);

    // BFS para encontrar caminho para sink
    while (!q.empty() && !g.findVertex(t)->isVisited()) {
        auto v = q.front();
        q.pop();

        for (auto e : v->getAdj()) {
            auto w = e->getDest();
            // se w não foi visitado e ainda há capacidade residual na aresta (peso - fluxo > 0)
            if (!w->isVisited() && (e->getWeight() - e->getFlow()) > 0) {
                w->setVisited(true);
                w->setPath(e); // lembra como chegamos aqui para refazer depois
                q.push(w);
            }
        }
    }
    // retorna true quamdo chegamos ao sink
    return g.findVertex(t)->isVisited();
}

/**
 * @brief Implementa o algoritmo Edmonds-Karp para fluxo máximo
 *
 * Calcula o fluxo máximo de origem para destino iterativamente:
 * 1. Redefine todos os fluxos para 0
 * 2. Encontra caminhos aumentadores e chega ao máximo fluxo possível
 * 3. Atualiza as arestas residuais (reversa)
 * 4. Repete até não haver mais caminhos aumentadores
 *
 * @param g Referência ao grafo de fluxo 
 * @param s source
 * @param t sink
 */
void edmondsKarp(Graph<string>& g, string s, string t) {
    // redefine todos os fluxos para 0
    for (auto v : g.getVertexSet()) {
        for (auto e : v->getAdj()) {
            e->setFlow(0);
        }
    }

    // Repete até não haver mais caminhos aumentadores
    while (findAugmentingPath(g, s, t)) {
        double bottleneck = INF;

        // encontra o bottleneck
        auto curr = g.findVertex(t);
        while (curr->getInfo() != s) {
            auto e = curr->getPath();
            bottleneck = min(bottleneck, e->getWeight() - e->getFlow());
            curr = e->getOrig();
        }

        // puxa o fluxo pelo caminho aumentador e atualiza as arestas residuais
        curr = g.findVertex(t);
        while (curr->getInfo() != s) {
            auto e = curr->getPath();
            e->setFlow(e->getFlow() + bottleneck);
            e->getReverse()->setFlow(e->getReverse()->getFlow() - bottleneck);
            curr = e->getOrig();
        }
    }
}


// estrutura para armazenar as atribuições extraídas do grafo
struct Assignment {
    int subId;
    int revId;
    int matchDomain;
};

/**
 * @brief Compara para ordenar atribuições pelo ID da submissão.
 *
 * Ordena primeiro pelo ID da submissão, depois pelo ID do revisor (desempate).
 *
 * @param a Primeira atribuição.
 * @param b Segunda atribuição.
 * @return true se a < b segundo este critério.
 */
bool compareSubmissions(const Assignment& a, const Assignment& b) {
    if (a.subId != b.subId) return a.subId < b.subId;
    return a.revId < b.revId;
}

/**
 * @brief Compara para ordenar atribuições pelo ID do revisor.
 *
 * Ordena primeiro pelo ID do revisor, depois pelo ID da submissão (desempate).
 *
 * @param a Primeira atribuição.
 * @param b Segunda atribuição.
 * @return true se a < b segundo este critério.
 */
bool compareReviewers(const Assignment& a, const Assignment& b) {
    if (a.revId != b.revId) return a.revId < b.revId;
    return a.subId < b.subId;
}


/**
 * @brief Identifica revisores críticos (ausencia faz com que a atribuiçao falhe)
 *
 * Para cada revisor, simula a sua remoção do grafo e faz Edmonds-Karp
 * Se o fluxo máximo ficar abaixo do alvo (submissões × minReviewsPerSubmission), classifica o revisor como crítico/arriscado
 *
 * @param data Dados carregados (submissões, revisores, configuração)
 * @return Vetor de IDs de revisores críticos (ordenado)
 */
vector<int> runRiskAnalysis1(const DataLoader& data) {
    vector<int> riskyReviewers;

    // numero de revisões necessárias para todas as submissões
    int targetFlow = data.submissions.size() * data.config.minReviewsPerSubmission;

    // para cada revisor, simula a sua remoção do grafo e faz Edmonds-Karp
    for (const auto& rev : data.reviewers) {
        // constroi o grafo do inicio para cada simulação 
        Graph<string> g = buildGraph(data.submissions, data.reviewers, data.config);

        // remove o revisor do grafo
        g.removeVertex("REV_" + to_string(rev.id));

        // calcula o fluxo máximo sem este revisor
        edmondsKarp(g, "SOURCE", "SINK");

        // calcula o fluxo atual sem este revisor
        int currentFlow = 0;
        auto sourceVertex = g.findVertex("SOURCE");
        if (sourceVertex) {
            for (auto e : sourceVertex->getAdj()) {
                currentFlow += e->getFlow();
            }
        }

        // se  fluxo atual < fluxo alvo, revisor crítico
        if (currentFlow < targetFlow) {
            riskyReviewers.push_back(rev.id);
        }
    }

    sort(riskyReviewers.begin(), riskyReviewers.end());
    return riskyReviewers;
}

/**
 * @brief Gera ficheiro de saída com atribuições e análises
 *
 * Extrai fluxo > 0 do grafo para montar a tabela de atribuições
 * 1. Atribuições da submissão
 * 2. Atribuições do revisor
 * 3. Submissões com revisões em falta (se houver)
 * 4. Revisores críticos (se riskAnalysis = 1)
 *
 * @param g Grafo de fluxo com fluxos calculados
 * @param data Estrutura com dados de entrada e configuração
 * @param riskyReviewers Lista de IDs de revisores criticos
 */
void generateAssignmentsOutput(Graph<string>& g, const DataLoader& data, const vector<int>& riskyReviewers) {
    vector<Assignment> assignments;

    for (const auto& sub : data.submissions) {
        string subNode = "SUB_" + to_string(sub.id);
        auto v = g.findVertex(subNode);
        for (auto e : v->getAdj()) {
            if (e->getFlow() > 0 && e->getDest()->getInfo().find("REV_") == 0) {
                int revId = stoi(e->getDest()->getInfo().substr(4));
                assignments.push_back({sub.id, revId, sub.primaryDomain});
            }
        }
    }

    string outName = data.config.outputFileName.empty() ? "output.csv" : data.config.outputFileName;
    ofstream outFile(outName);

    // espaçamento exato: #SubmissionId,ReviewerId,Match
    vector<Assignment> subSorted = assignments;
    sort(subSorted.begin(), subSorted.end(), compareSubmissions);

    outFile << "#SubmissionId,ReviewerId,Match\n";
    for (const auto& a : subSorted) {
        outFile << a.subId << ", " << a.revId << ", " << a.matchDomain << "\n";
    }

    // espaçamento exato: #ReviewerId,SubmissionId,Match
    vector<Assignment> revSorted = assignments;
    sort(revSorted.begin(), revSorted.end(), compareReviewers);

    outFile << "#ReviewerId,SubmissionId,Match\n";
    for (const auto& a : revSorted) {
        outFile << a.revId << ", " << a.subId << ", " << a.matchDomain << "\n";
    }

    outFile << "#Total: " << assignments.size() << "\n";

    // submissões com revisões em falta (fluxo < capacidade)
    struct Missing { int subId; int domain; int missingReviews; };
    vector<Missing> missingList;
    auto sourceVertex = g.findVertex("SOURCE");
    for (auto e : sourceVertex->getAdj()) {
        if (e->getFlow() < e->getWeight()) {
            int missing = e->getWeight() - e->getFlow();
            int subId = stoi(e->getDest()->getInfo().substr(4));
            int domain = 0;
            for(const auto& s : data.submissions) {
                if (s.id == subId) { domain = s.primaryDomain; break; }
            }
            missingList.push_back({subId, domain, missing});
        }
    }

    if (!missingList.empty()) {
        outFile << "#SubmissionId,Domain,MissingReviews\n";
        sort(missingList.begin(), missingList.end(), [](const Missing& a, const Missing& b){ return a.subId < b.subId; });
        for(const auto& m : missingList) {
            outFile << m.subId << ", " << m.domain << ", " << m.missingReviews << "\n";
        }
    }

    // análise de risco (revisores críticos)
    if (data.config.riskAnalysis == 1) {
        outFile << "#Risk Analysis: 1\n";
        if (!riskyReviewers.empty()) {
            for (size_t i = 0; i < riskyReviewers.size(); ++i) {
                outFile << riskyReviewers[i] << (i + 1 == riskyReviewers.size() ? "\n" : ", ");
            }
        } else {
            outFile << "\n"; 
        }
    }

    cout << "Resultados guardados em " << outName << endl;
    outFile.close();
}