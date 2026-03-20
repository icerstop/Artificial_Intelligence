#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include "tsp.h"
#include "algorithms.h"


static void printStats(const Stats& s) {
    std::cout << std::setw(8) << static_cast<int>(s.avg)
              << " (" << std::setw(7) << s.min
              << " - " << std::setw(7) << s.max << ")";
}

static void printStatsTable(
    const std::vector<std::string>& labels,
    const std::vector<AlgoStats>&   statsA,
    const std::vector<AlgoStats>&   statsB,
    bool usePhaseI)
{
    const int W = 12;
    std::cout << std::left << std::setw(W) << "Metoda"
              << "  " << std::setw(28) << "TSPA"
              << "  " << std::setw(28) << "TSPB" << "\n";
    std::cout << std::string(74, '-') << "\n";

    for (int i = 0; i < (int)labels.size(); ++i) {
        const Stats& sA = usePhaseI ? statsA[i].phaseI  : statsA[i].phaseII;
        const Stats& sB = usePhaseI ? statsB[i].phaseI  : statsB[i].phaseII;
        std::cout << std::left << std::setw(W) << labels[i] << std::right << "  ";
        printStats(sA);
        std::cout << "  ";
        printStats(sB);
        std::cout << "\n";
    }
    std::cout << "\n";
}


static void printInstanceSummary(const TSPInstance& tsp) {
    std::cout << "=== " << tsp.name << " ===\n";
    std::cout << "Liczba wierzcholkow: " << tsp.size() << "\n";
    std::cout << "Przykladowe wierzcholki (x, y, profit):\n";
    for (int i = 0; i < std::min(3, tsp.size()); ++i)
        std::cout << "  [" << i << "] " << tsp.nodes[i].x << " " << tsp.nodes[i].y
                  << " profit=" << tsp.nodes[i].profit << "\n";
    std::cout << "Przykladowe odleglosci:\n";
    int n = std::min(4, tsp.size());
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            std::cout << "  dist[" << i << "][" << j << "] = " << tsp.dist[i][j] << "\n";
    std::cout << "\n";
}


int main() {
    try {
        TSPInstance tspA("TSPA.csv");
        TSPInstance tspB("TSPB.csv");

        printInstanceSummary(tspA);
        printInstanceSummary(tspB);

        auto wRegret = [](int s, const TSPInstance& t, bool u) {
            return weightedRegretGC(s, t, u, 1.0, 1.0);
        };

        std::vector<std::string> labels = {
            "NNa", "NNb",
            "GCa", "GCb",
            "RegretGCa", "RegretGCb",
            "WRegretGCa", "WRegretGCb"
        };
        std::vector<std::pair<AlgoFunc, bool>> algos = {
            { greedyNN, false }, { greedyNN, true  },
            { greedyGC, false }, { greedyGC, true  },
            { regretGC, false }, { regretGC, true  },
            { wRegret,  false }, { wRegret,  true  }
        };

        std::cout << "Trwa obliczanie (200 startow x 8 algorytmow x 2 instancje)...\n\n";
        std::mt19937 rng(42);
        labels.insert(labels.begin(), "Random");
        std::vector<AlgoStats> statsA, statsB;
        statsA.push_back(collectRandomStats(tspA, 200, rng));
        statsB.push_back(collectRandomStats(tspB, 200, rng));
        for (auto& [algo, useProfit] : algos) {
            statsA.push_back(collectStats(algo, tspA, useProfit));
            statsB.push_back(collectStats(algo, tspB, useProfit));
        }

        // faza I
        std::cout << "Wyniki po I fazie\n";
        std::cout << "    srednia (min - max)\n\n";
        printStatsTable(labels, statsA, statsB, true);

        // faza II
        std::cout << "Wyniki po II fazie\n";
        std::cout << "    srednia (min - max)\n\n";
        printStatsTable(labels, statsA, statsB, false);

        // Najlepsze rozwiazania
        std::cout << "Najlepsze rozwiazania\n";
        for (int i = 0; i < (int)labels.size(); ++i) {
            std::cout << "  " << std::left << std::setw(12) << labels[i] << std::right
                      << "  TSPA: wynik=" << std::setw(7) << tspA.evaluate(statsA[i].bestTour)
                      << "  n=" << std::setw(3) << statsA[i].bestTour.size()
                      << "  ||  TSPB: wynik=" << std::setw(7) << tspB.evaluate(statsB[i].bestTour)
                      << "  n=" << std::setw(3) << statsB[i].bestTour.size() << "\n";
        }

        // Eksport najlepszych rozwiązań do plików dla Solution Checkera
        // Format: jedna liczba na linię = ID wierzchołka (0-indexed), wkleić do kolumny F od wiersza 3
        std::cout << "\nEksport rozwiazań do plikow solution_*.txt ...\n";
        for (int i = 0; i < (int)labels.size(); ++i) {
            auto exportTour = [&](const std::string& instance,
                                  const std::vector<int>& tour) {
                std::string fname = "solution_" + instance + "_" + labels[i] + ".txt";
                // zamień spacje na podkreślniki w nazwie pliku
                for (char& c : fname) if (c == ' ') c = '_';
                std::ofstream f(fname);
                for (int node : tour)
                    f << node << "\n";
            };
            exportTour("TSPA", statsA[i].bestTour);
            exportTour("TSPB", statsB[i].bestTour);
        }
        std::cout << "Gotowe. Wklej zawartosc pliku do kolumny F (od wiersza 3) w Solution checker2.xlsx\n";

    } catch (const std::exception& e) {
        std::cerr << "Blad: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
