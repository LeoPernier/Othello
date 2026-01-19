// src/AutoCompare.cpp

#include "../include/Board.hpp"
#include "../include/Solver.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>

/**
 * @brief Représente les statistiques d’un coup pour le joueur et le solveur.
 *
 * Cette structure stocke :
 *  - L’indice du coup joué par le joueur (Noir) et par le solveur (Blanc),
 *  - Le type de coup (e.g. “Joueur”, “Solveur (Série)”, “Solveur (Parallèle)”,
 *    “Passe Joueur”, “Passe Solveur”),
 *  - Les coordonnées du coup sous forme d’objet Move,
 *  - La durée d’exécution en nanosecondes,
 *  - Le speedup (Ratio série / parallèle).
 *
 * @param coupJoueur   Index du coup effectué par le joueur (Noir).
 * @param coupSolveur  Index du coup effectué par le solveur (Blanc).
 * @param moveType     Description du type de coup.
 * @param move         Objet Move décrivant la position du coup.
 * @param elapsedNs    Durée d’exécution du coup en nanosecondes.
 * @param speedup      Ratio entre le temps série et le temps parallèle.
 */
struct MoveStats {
    int         coupJoueur;
    int         coupSolveur;
    std::string moveType;
    Move        move;
    long long   elapsedNs;
    double      speedup;
};

/**
 * @brief Convertit un objet Move en une représentation texte lisible.
 *
 * Si le coup est un passage (row ou col < 0), retourne "PASSE".
 * Sinon, la colonne est représentée par une lettre (A, B, C, ...) et la ligne par un index à partir de 1.
 *
 * @param  mv  Le Move à convertir.
 * @return     Chaîne représentant le coup (e.g. "A1", "PASSE").
 */
std::string moveToString(const Move& mv) {
    if (mv.row < 0 || mv.col < 0) return "PASSE";
    char col = 'A'    + mv.col;
    int  row = mv.row + 1;
    return std::string(1, col) + std::to_string(row);
}

/**
 * @brief Formate une durée en nanosecondes en une chaîne lisible.
 *
 * En fonction de l'ordre de grandeur, choisit secondes, millisecondes,
 * microsecondes ou nanosecondes.
 *
 * @param  ns  Durée en nanosecondes.
 * @return     Chaîne formatée (e.g. "1.234 s", "5.678 ms").
 */
std::string formatTime(long long ns) {
    std::ostringstream oss;
    if (ns >= 10'000'000'000LL)
        oss << std::fixed << std::setprecision(3) << (ns / 1e9) << " s";
    else if (ns >= 10'000'000LL)
        oss << (ns / 1'000'000) << " ms";
    else if (ns >= 10'000LL)
        oss << (ns / 1'000) << " μs";
    else
        oss << ns << " ns";
    return oss.str();
}

/**
 * @brief Calcule les statistiques de base (moyenne, écart-type, min, max) sur un vecteur de valeurs.
 *
 * @param v       Vecteur de valeurs (e.g. durées en nanosecondes).
 * @param moyenne Résultat : moyenne arithmétique.
 * @param stdev   Résultat : écart-type échantilonnal.
 * @param minv    Résultat : valeur minimale.
 * @param maxv    Résultat : valeur maximale.
 */
void calculeStats(const std::vector<long long>& v, double& moyenne,
    double& stdev, long long& minv, long long& maxv) {
    if (v.empty()) {
        moyenne = stdev = minv = maxv = 0;
        return;
    }
    long long somme = std::accumulate(v.begin(), v.end(), 0LL);
    moyenne         = (double)somme / v.size();
    minv            = *std::min_element(v.begin(), v.end());
    maxv            = *std::max_element(v.begin(), v.end());
    if (v.size() == 1) {
        stdev = 0.0;
        return;
    }
    double var = 0.0;
    for (auto x : v)
        var += (x - moyenne) * (x - moyenne);
    stdev = std::sqrt(var / (v.size() - 1));
}


/**
 * @brief Calcule les statistiques de speedup d'un ensemble de ratios.
 *
 * Détermine la moyenne globale, la moyenne des ratios > 1,
 * les nombres de ratios supérieurs et inférieurs à 1,
 * ainsi que les valeurs minimale et maximale.
 *
 * @param v            Vecteur de ratios (Temps série / Temps parallèle).
 * @param meanAll      Résultat : Moyenne de tous les ratios.
 * @param meanSup1     Résultat : Moyenne des ratios > 1.
 * @param countSup1    Résultat : Nombre de ratios > 1.
 * @param minv         Résultat : Ratio minimal.
 * @param maxv         Résultat : Ratio maximal.
 * @param countInf1    Résultat : Nombre de ratios ≤ 1.
 * @param total        Résultat : Nombre total de ratios.
 */
void calculeSpeedupStats(const std::vector<double>& v,
    double& meanAll,   double& meanSup1,
    int&    countSup1, double& minv,
    double& maxv,      int&    countInf1,
    int& total) {
    meanAll   = meanSup1  = minv  = maxv = 0;
    countSup1 = countInf1 = total = 0;
    if (v.empty()) return;
    minv = *std::min_element(v.begin(), v.end());
    maxv = *std::max_element(v.begin(), v.end());
    double sumAll = 0, sumSup1 = 0;
    for (auto x : v) {
        sumAll += x;
        if (x > 1.0) {
            sumSup1 += x;
            countSup1++;
        } else
            countInf1++;
    }
    total   = v.size();
    meanAll = sumAll / v.size();
    if (countSup1 > 0)
        meanSup1 = sumSup1 / countSup1;
}

/**
 * @brief Formate une ligne de statistiques dans un tableau à largeur fixe.
 *
 * Aligne le titre à gauche et les temps formatés à droite.
 *
 * @param titre Label de la ligne (e.g. "Temps Solveur Série").
 * @param avg   Moyenne en nanosecondes.
 * @param stdev Écart-type en nanosecondes.
 * @param minv  Valeur minimale en nanosecondes.
 * @param maxv  Valeur maximale en nanosecondes.
 * @return      Chaîne représentant une ligne du tableau.
 */
std::string printStatsTable(const char* titre, double avg, double stdev, long long minv, long long maxv) {
    std::ostringstream oss;
    oss << std::left     << std::setw(30) << titre
        << std::right    << std::setw(14) << formatTime((long long)avg)
        << std::setw(14) << formatTime((long long)stdev)
        << std::setw(14) << formatTime(minv)
        << std::setw(14) << formatTime(maxv);
    return oss.str();
}

/**
 * @brief Génère un bloc de texte des statistiques de speedup.
 *
 * Affiche min, max, comptes et moyennes sous forme de lignes lisibles.
 *
 * @param meanAll   Moyenne de tous les speedups.
 * @param meanSup1  Moyenne des speedups > 1.
 * @param countSup1 Nombre de speedups > 1.
 * @param minv      Speedup minimal.
 * @param maxv      Speedup maximal.
 * @param countInf1 Nombre de speedups ≤ 1.
 * @param total     Nombre total de speedups.
 * @return          Chaîne multi-lignes des statistiques.
 */
std::string printSpeedupStats(double meanAll, double meanSup1, int countSup1, double minv, double maxv, int countInf1, int total) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "  Speedup minimum:                x" << minv      << "\n";
    oss << "  Speedup maximum:                x" << maxv      << "\n";
    oss << "  Nombre total de speedups:       "  << total     << "\n";
    oss << "  Nombre speedup < 1:             "  << countInf1 << " (" << ((total > 0) ? (100 * countInf1 / total) : 0) << "%)\n";
    oss << "  Speedup moyen (tous coups):     x" << meanAll   << "\n";
    oss << "  Speedup moyen (speedup>1):      x" << (countSup1 > 0 ? meanSup1 : 0.0) << " (" << countSup1 << " coups)\n";
    return oss.str();
}

/**
 * @brief Formate les statistiques de la partie en CSV.
 * 
 * Produit les données par coup suivies du résumé et des résultats finaux.
 * 
 * Très cringe comme docstring, je me demande si
 * quelqu’un prends même le temps à le lire.
 *
 * @param movesStats     Vecteur des statistiques par coup.
 * @param blackFinal     Nombre final de pions noirs.
 * @param whiteFinal     Nombre final de pions blancs.
 * @param coupsJoueur    Nombre de coups du joueur (Noir).
 * @param coupsSolveur   Nombre de coups du solveur (Blanc).
 * @param passesJoueur   Nombre de passages du joueur.
 * @param passesSolveur  Nombre de passages du solveur.
 * @param pctPassesJ     Pourcentage de passages du joueur.
 * @param pctPassesS     Pourcentage de passages du solveur.
 * @param avgSerie       Moyenne du temps série.
 * @param stdevSerie     Écart-type du temps série.
 * @param minSerie       Temps série minimal.
 * @param maxSerie       Temps série maximal.
 * @param avgPar         Moyenne du temps parallèle.
 * @param stdevPar       Écart-type du temps parallèle.
 * @param minPar         Temps parallèle minimal.
 * @param maxPar         Temps parallèle maximal.
 * @param avgSpeedup     Moyenne des speedups.
 * @param avgSpeedupSup1 Moyenne des speedups > 1.
 * @param minSpeedup     Speedup minimal.
 * @param maxSpeedup     Speedup maximal.
 * @param countSup1      Nombre de speedups > 1.
 * @param countInf1      Nombre de speedups ≤ 1.
 * @param totalSpeedup   Nombre total de speedups.
 * @return               Chaîne CSV complète du rapport.
 */
std::string formatStatsCSV(
    const std::vector<MoveStats>& movesStats,
    int       blackFinal,   int       whiteFinal,
    int       coupsJoueur,  int       coupsSolveur,
    int       passesJoueur, int       passesSolveur,
    double    pctPassesJ,   double    pctPassesS,
    double    avgSerie,     double    stdevSerie,
    long long minSerie,     long long maxSerie,
    double    avgPar,       double    stdevPar,
    long long minPar,       long long maxPar,
    double    avgSpeedup,   double    avgSpeedupSup1,
    double    minSpeedup,   double    maxSpeedup,
    int       countSup1,    int       countInf1,
    int       totalSpeedup) {
    std::ostringstream oss;
    oss << "Coup_Joueur, Coup_Solveur, Type, Coup, Nanosecondes, Speedup\n";
    for (const auto& stat : movesStats) {
        oss << stat.coupJoueur         << ", "
            << stat.coupSolveur        << ", "
            << stat.moveType           << ", "
            << moveToString(stat.move) << ", "
            << stat.elapsedNs          << ", ";
        if (stat.speedup > 0.01)
            oss << std::fixed << std::setprecision(3) << stat.speedup;
        oss << "\n";
    }

    oss << "\nFinal Noir," << blackFinal << "\nFinal Blanc," << whiteFinal << "\n";
    if (blackFinal > whiteFinal)
        oss << "Gagnant, Noir\n";
    else if (whiteFinal > blackFinal)
        oss << "Gagnant, Blanc\n";
    else
        oss << "Gagnant, Nul\n";

    oss << "\nStats de la partie\n";
    oss << "Coups Joueur   (NOIR),"    << coupsJoueur   << "\n";
    oss << "Coups Solveur  (BLANC),"   << coupsSolveur  << "\n";
    oss << "Passes Joueur  (NOIR),"    << passesJoueur  << " (" << pctPassesJ << "%)\n";
    oss << "Passes Solveur (BLANC),"   << passesSolveur << " (" << pctPassesS << "%)\n";
    oss << "Temps Solveur Série     (moyenne [ns], stdev, min, max),"
        << avgSerie << ", " << stdevSerie   << ", " << minSerie << ", "  << maxSerie  << "\n";
    oss << "Temps Solveur Parallèle (moyenne [ns], stdev, min, max),"
        << avgPar << ", "   << stdevPar     << ", " << minPar   << ", "  << maxPar    << "\n";
    oss << "Speedup moyen (tous coups), x"  << avgSpeedup       << "\n";
    oss << "Speedup moyen (speedup>1),  x"  << avgSpeedupSup1   << " (" << countSup1 << " coups)\n";
    oss << "Speedup minimum, x" << minSpeedup   << "\n";
    oss << "Speedup maximum, x" << maxSpeedup   << "\n";
    oss << "Speedup < 1 (parallèle plus lent)," << countInf1  << " ("
        << ((totalSpeedup > 0)? (100 * countInf1 / totalSpeedup) : 0) << "%)\n";
    return oss.str();
}

/**
 * @brief Lit un entier depuis l'entrée standard avec validation.
 *
 * Affiche le prompt tant qu'un entier valide n'est pas saisi.
 *
 * @param prompt Texte affiché avant la saisie.
 * @return       L'entier saisi.
 */
int lireEntier(const std::string& prompt) {
    int val;
    while (true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss(line);
        if (ss >> val && !(ss >> line))
            return val;
        std::cout << "[ERREUR] Entrée invalide!\n";
    }
}

int main(int argc, char* argv[]) {
    int N {10}, profondeur{-1}, temps{-1};

    // Argument 1: Nombre de parties
    if (argc > 1) {
        try {
            int n = std::stoi(argv[1]);
            if (n > 0)
                N = n;
        }
        catch (...) { std::cout << "[INFO] Argument N invalide. 10 parties par défaut.\n"; }
    }

    // Argument 2: Profondeur, 3: Temps
    if (argc > 2) {
        try {
            profondeur = std::stoi(argv[2]);
        }
        catch (...) {
            profondeur = -1;
        }
    }
    if (argc > 3) {
        try {
            temps = std::stoi(argv[3]);
        }
        catch (...) {
            temps = -1;
        }
    }

    if (profondeur <= 0 || temps <= 0) {
        std::cout << "\nChoisir le niveau du Solveur :\n";
        std::cout << "  1) Pour les debutants          : Profondeur 3,  Temps 100   ms\n";
        std::cout << "  2) Pour les joueurs quotidiens : Profondeur 5,  Temps 500   ms\n";
        std::cout << "  3) Poor les tryhards           : Profondeur 8,  Temps 2000  ms\n";
        std::cout << "  4) Pour les pros (lent)        : Profondeur 10, Temps 10000 ms\n";
        std::cout << "  5) Choix personnalisé\n";
        std::cout << "Fais ton choix (1-5) : ";

        int choix = lireEntier("");
        switch (choix) {
            case 1: profondeur = 3;  temps = 100;   break;
            case 2: profondeur = 5;  temps = 500;   break;
            case 3: profondeur = 8;  temps = 2000;  break;
            case 4: profondeur = 10; temps = 10000; break;
            case 5:
            default:
                profondeur = lireEntier("  Choisir la profondeur (e.g.: 5) : ");
                temps      = lireEntier("  Choisir le temps max (En ms, e.g.: 500) : ");
                break;
        }
        std::cout << "\n[INFO] Paramètres solveur : Profondeur = " << profondeur
                  << ", Temps max = " << temps << " ms.\n";
    } else
        std::cout << "[INFO] Utilisation des paramètres solveur en ligne de commande: Profondeur = "
                  << profondeur << ", Temps max = " << temps << " ms.\n";

    std::random_device rd;
    std::mt19937 rng(rd());

    // Stats globales
    int totalJoueur = 0,   totalSolveur = 0, totalPassJoueur = 0, totalPassSolveur = 0;
    std::vector<long long> serieTimesGlobal, parallelTimesGlobal;
    std::vector<double>    speedupsGlobal;

    for (int g = 1; g <= N; ++g) {
        Board board;
        Solver solver(profondeur, temps);
        int moveIdxPlayer = 0, moveIdxSolver = 0, passesPlayer = 0, passesSolver = 0;
        std::vector<MoveStats> movesStats;
        std::vector<long long> serieTimes, parallelTimes;
        std::vector<double> speedups;
        int currentTurn = BLACK;

        std::cout << "\n========== [PARTIE #" << g << "] ==========\n";
        while (!board.isGameOver()) {
            if (currentTurn == BLACK) {
                auto valid = board.getValidMoves(BLACK);
                if (!valid.empty()) {
                    moveIdxPlayer++;
                    std::uniform_int_distribution<int> dist(0, valid.size()-1);
                    Move mv = valid[dist(rng)];
                    std::cout << "[TOUR] Joueur  (NOIR)  | Coup #" << moveIdxPlayer << ": " << moveToString(mv) << "\n";
                    board.applyMove(mv, BLACK);
                    movesStats.push_back({moveIdxPlayer, 0, "Joueur", mv, 0, 0.0});
                    totalJoueur++;
                } else {
                    passesPlayer++;
                    std::cout << "[TOUR] Joueur  (NOIR)  | PASSE (Aucun coup valide).\n";
                    movesStats.push_back({0, 0, "Passe Joueur", {-1,-1}, 0, 0.0});
                    totalPassJoueur++;
                }
                currentTurn = WHITE;
            } else {
                auto valid = board.getValidMoves(WHITE);
                if (!valid.empty()) {
                    moveIdxSolver++;
                    solver.resetTT();
                    std::cout << "[TOUR] Solveur (BLANC) | Coup #" << moveIdxSolver << "\n";
                    std::cout << "    [SOLVEUR-SÉRIE]     Calcul...\n"; // On met ce message pour les gros tests
                    auto start_serial = std::chrono::high_resolution_clock::now();
                    solver.setParallel(false);
                    Move serialMove   = solver.findBestMove(board, WHITE);
                    auto end_serial   = std::chrono::high_resolution_clock::now();
                    auto serial_ns    = std::chrono::duration_cast<std::chrono::nanoseconds>(end_serial - start_serial).count();
                    std::cout << "    [SOLVEUR-SÉRIE]     Coup: " << moveToString(serialMove)
                              << ", temps: " << formatTime(serial_ns) << "\n";
                    solver.resetTT();
                    std::cout << "    [SOLVEUR-PARALLÈLE] Calcul...\n"; // On met ce message pour les gros tests
                    auto start_parallel = std::chrono::high_resolution_clock::now();
                    solver.setParallel(true);
                    Move parallelMove   = solver.findBestMove(board, WHITE);
                    auto end_parallel   = std::chrono::high_resolution_clock::now();
                    auto parallel_ns    = std::chrono::duration_cast<std::chrono::nanoseconds>(end_parallel - start_parallel).count();
                    std::cout << "    [SOLVEUR-PARALLÈLE] Coup: " << moveToString(parallelMove)
                              << ", temps: " << formatTime(parallel_ns) << "\n";
                    if (serialMove == parallelMove)
                        std::cout << "    [INFO]          Les deux solveurs sont d'accord: " << moveToString(serialMove) << "\n";
                    else
                        std::cout << "    [AVERTISSEMENT] Désaccord entre les solveurs!\n";
                    board.applyMove(serialMove, WHITE);
                    movesStats.push_back({0, moveIdxSolver, "Solveur (Série)",     serialMove,   serial_ns, 0.0});
                    movesStats.push_back({0, moveIdxSolver, "Solveur (Parallèle)", parallelMove, parallel_ns,
                                         (parallel_ns > 0 ? (double)serial_ns / parallel_ns : 0.0)});
                    totalSolveur++;
                    serieTimes.push_back(serial_ns);
                    parallelTimes.push_back(parallel_ns);
                    serieTimesGlobal.push_back(serial_ns);
                    parallelTimesGlobal.push_back(parallel_ns);
                    if (parallel_ns > 0)
                        speedups.push_back((double)serial_ns / parallel_ns), speedupsGlobal.push_back((double)serial_ns / parallel_ns);
                } else {
                    passesSolver++;
                    std::cout << "[TOUR] Solveur (BLANC) | PASSE (Aucun coup valide).\n";
                    movesStats.push_back({0, 0, "Passe Solveur", {-1,-1}, 0, 0.0});
                    totalPassSolveur++;
                }
                currentTurn = BLACK;
            }
        }
        int blackFinal = board.count(BLACK);
        int whiteFinal = board.count(WHITE);

        // --- Calcul stats ---
        double    avgSerie = 0, stdevSerie = 0, avgPar = 0, stdevPar = 0;
        long long minSerie = 0, maxSerie   = 0, minPar = 0, maxPar   = 0;
        calculeStats(serieTimes,    avgSerie, stdevSerie, minSerie, maxSerie);
        calculeStats(parallelTimes, avgPar,   stdevPar,   minPar,   maxPar);

        double avgSpeedup = 0, avgSpeedupSup1 = 0, minSpeedup   = 0, maxSpeedup=0;
        int    countSup1  = 0, countInf1      = 0, totalSpeedup = 0;
        calculeSpeedupStats(speedups,   avgSpeedup, avgSpeedupSup1, countSup1,
                            minSpeedup, maxSpeedup, countInf1,      totalSpeedup);

        double pctPassJ = (moveIdxPlayer + passesPlayer > 0) ? 100.0 * passesPlayer / (moveIdxPlayer + passesPlayer) : 0;
        double pctPassS = (moveIdxSolver + passesSolver > 0) ? 100.0 * passesSolver / (moveIdxSolver + passesSolver) : 0;

        // --- On affiche les stats ---
        std::cout << "\n------------------- Stats détaillées de la partie -------------------\n";
        std::cout << "Coups Joueur (NOIR):    "        << moveIdxPlayer
                  << "      Passes: " << passesPlayer  << " (" << pctPassJ << "%)\n";
        std::cout << "Coups Solveur (BLANC):  "        << moveIdxSolver
                  << "      Passes: " << passesSolver  << " (" << pctPassS << "%)\n";
        std::cout << "\n   "          << std::left     << std::setw(32)    << ""
                  << std::setw(14)    << "Moyenne"     << std::setw(14)    << "Stdev" << std::setw(14)
                  << "Min"            << std::setw(14) << "Max" << "\n";
        std::cout << printStatsTable("Temps Solveur Série",     avgSerie, stdevSerie, minSerie, maxSerie) << "\n";
        std::cout << printStatsTable("Temps Solveur Parallèle", avgPar,   stdevPar,   minPar,   maxPar)   << "\n";
        std::cout << "\n--- Stats Speedup ---\n"
                  << printSpeedupStats(avgSpeedup, avgSpeedupSup1, countSup1, minSpeedup,
                                       maxSpeedup, countInf1,      totalSpeedup) << "\n";
        std::cout << "---------------------------------------------------------------------\n";
        std::cout << "Score final: Noir = " << blackFinal << " | Blanc = " << whiteFinal << "\n";
        if (blackFinal > whiteFinal)
            std::cout << "Gagnant: Noir\n";
        else if (whiteFinal > blackFinal)
            std::cout << "Gagnant: Blanc\n";
        else
            std::cout << "Match nul!\n";

        // --- On fait le CSV ---
        std::string filename = "Partie_" + std::to_string(g) + ".csv";
        std::ofstream out(filename);
        if (!out) {
            std::cerr << "[ERREUR] Impossible d'ouvrir " << filename << " pour l'écriture.\n";
            continue;
        }
        out << formatStatsCSV(
            movesStats,     blackFinal,
            whiteFinal,     moveIdxPlayer,
            moveIdxSolver,  passesPlayer,
            passesSolver,   pctPassJ,
            pctPassS,       avgSerie,
            stdevSerie,     minSerie,
            maxSerie,       avgPar,
            stdevPar,       minPar,
            maxPar,         avgSpeedup,
            avgSpeedupSup1, minSpeedup,
            maxSpeedup,     countSup1,
            countInf1,      totalSpeedup
        );
        out.close();
        std::cout << "[INFO] Partie " << g << " terminée. Stats écrites dans " << filename << ".\n";
    }

    // -------- Stats globales ---------
    double    gAvgSerie = 0, gStdevSerie = 0, gAvgPar = 0, gStdevPar = 0;
    long long gMinSerie = 0, gMaxSerie   = 0, gMinPar = 0, gMaxPar   = 0;
    calculeStats(serieTimesGlobal,    gAvgSerie, gStdevSerie, gMinSerie, gMaxSerie);
    calculeStats(parallelTimesGlobal, gAvgPar,   gStdevPar,   gMinPar,   gMaxPar);
    double gAvgSpeedup = 0, gAvgSpeedupSup1 = 0, gMinSpeedup   = 0, gMaxSpeedup = 0;
    int    gCountSup1  = 0, gCountInf1      = 0, gTotalSpeedup = 0;
    calculeSpeedupStats(speedupsGlobal, gAvgSpeedup, gAvgSpeedupSup1, gCountSup1,
                        gMinSpeedup,    gMaxSpeedup, gCountInf1,      gTotalSpeedup);

    std::cout << "\n========== [STATISTIQUES GLOBALES] ==========\n";
    std::cout << "Coups  Joueur  total  (NOIR): " << totalJoueur     << "\n";
    std::cout << "Coups  Solveur total (BLANC): " << totalSolveur     << "\n";
    std::cout << "Passes Joueur         (NOIR): " << totalPassJoueur << "\n";
    std::cout << "Passes Solveur       (BLANC): " << totalPassSolveur << "\n";
    std::cout << "\n   "       << std::left << std::setw(32) << ""
              << std::setw(14) << "Moyenne" << std::setw(14) << "Stdev"
              << std::setw(14) << "Min"     << std::setw(14) << "Max"   << "\n";
    std::cout << printStatsTable("Temps Solveur Série",     gAvgSerie, gStdevSerie, gMinSerie, gMaxSerie) << "\n";
    std::cout << printStatsTable("Temps Solveur Parallèle", gAvgPar,   gStdevPar,   gMinPar,   gMaxPar)   << "\n";
    std::cout << "\n--- Stats Speedup ---\n"
              << printSpeedupStats(gAvgSpeedup, gAvgSpeedupSup1, gCountSup1, gMinSpeedup,
                                   gMaxSpeedup, gCountInf1,      gTotalSpeedup) << "\n";
    std::cout << "[INFO] Toutes les parties sont terminées !\n";
    return 0;
}
