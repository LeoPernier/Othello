// tests/TestSolver.cpp

#include "../include/Board.hpp"
#include "../include/Solver.hpp"
#include "TestUtils.hpp"

#include <climits>
#include <algorithm>
#include <vector>


/**
 * \brief  Vérifie les paramètres par défaut du solveur (profondeur, budget temps, etc.).
 * \return true si le test/résultat est conforme, false sinon
 */
bool testDefaultSettings() {
    Solver s;
    return s.getDepth() == 5 && s.getTime() == 500;
}

/**
 * \brief  Vérifie les accesseurs/mutateurs de profondeur de recherche et de temps.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testSetAndGetDepthTime() {
    Solver s;
    s.setDepth(3);
    s.setTime(250);
    return s.getDepth() == 3 && s.getTime() == 250;
}

/**
 * \brief  Vérifie que le meilleur coup trouvé est déterministe à paramètres identiques.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testDeterministicBestMove() {
    Board b;
    Solver s(3, 200, false);
    Move m1 = s.findBestMove(b, BLACK);
    Move m2 = s.findBestMove(b, BLACK);
    return m1.row == m2.row && m1.col == m2.col;
}

/**
 * \brief  Vérifie la stabilité du score de coups entre deux évaluations identiques.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testScoreMovesStability() {
    Board b;
    Solver s(3, 200, false);
    auto r1 = s.scoreMoves(b, WHITE);
    auto r2 = s.scoreMoves(b, WHITE);
    if (r1.size() != r2.size())
        return false;
    for (size_t i = 0; i < r1.size(); ++i) {
        if (r1[i].first.row != r2[i].first.row ||
            r1[i].first.col != r2[i].first.col ||
            r1[i].second    != r2[i].second)
            return false;
    }
    return true;
}

/**
 * \brief  Vérifie que le hachage Zobrist est déterministe pour un même état.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testZobristHashDeterminism() {
    Board b;
    Solver s;
    auto h1 = s.zobristHash(b);
    auto h2 = s.zobristHash(b);
    return h1 == h2;
}

/**
 * \brief Vérifie que le hachage Zobrist change dès qu’un coup modifie l’état
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testZobristHashChangesOnMove() {
    Board b;
    Solver s;
    auto moves = b.getValidMoves(BLACK);
    if (moves.empty())
        return false;
    uint64_t hBefore = s.zobristHash(b);
    b.applyMove(moves[0], BLACK);
    uint64_t hAfter  = s.zobristHash(b);
    return hBefore != hAfter;
}

/**
 * \brief  Vérifie l’évaluation lorsque aucun coup n’est disponible.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testScoreMovesNoMoves() {
    Board b;
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            b.setCell(i, j, WHITE);
    Solver s(1, 100, false);
    return s.scoreMoves(b, BLACK).empty();
}

/**
 * \brief  Compare résultats parallèles vs séquentiels pour assurer la cohérence.
 * \return true si le test/résultat est conforme, false sinon
 */
bool testParallelSequentialConsistency() {
    Board b;
    Solver seq(2, 200, false), par(2, 200, true);
    auto rSeq = seq.scoreMoves(b, WHITE);
    auto rPar = par.scoreMoves(b, WHITE);
    if (rSeq.size() != rPar.size())
        return false;
    for (size_t i = 0; i < rSeq.size(); ++i) {
        if (rSeq[i].first.row != rPar[i].first.row ||
            rSeq[i].first.col != rPar[i].first.col ||
            rSeq[i].second    != rPar[i].second)
            return false;
    }
    return true;
}

/**
 * \brief  Vérifie le repli (fallback) lorsque le budget temps est nul ou insuffisant.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testZeroTimeFallback() {
    Board b;
    Solver t0(3, 0, false), tNorm(3, 500, false);
    Move m0 = t0.findBestMove(b, BLACK);
    Move mN = tNorm.findBestMove(b, BLACK);
    return m0.row == mN.row && m0.col == mN.col;
}

/**
 * \brief  Vérifie qu’un coup qui retourne beaucoup de pions reçoit un score élevé (heuristique).
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testHighFlipMove() {
    Board b;
    for (int j = 1; j < 7; ++j)
        b.setCell(0, j, WHITE);
    b.setCell(0, 7, BLACK);
    Solver s(1, 100, false);
    Move m = s.findBestMove(b, BLACK);
    return m.row == 0 && m.col == 0;
}

/**
 * \brief  Vérifie l’heuristique de mobilité (nombre de coups possibles).
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testEvaluateMobility() {
    Board b;
    Solver s;
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            b.setCell(i, j, BLACK);
    b.setCell(7,7, WHITE);
    int scoreB = s.evaluate(b, BLACK);
    int scoreW = s.evaluate(b, WHITE);
    return scoreB > scoreW;
}

/**
 * \brief   Vérifie l’évaluation en position terminale (gain/perte/égalité).
 * \return  true si le test/résultat est conforme, false sinon.
 */
bool testEvaluateTerminalBoard() {
    Board b;
    Solver s;
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            b.setCell(i, j, (i < 4 ? BLACK : WHITE));
    return s.evaluate(b, BLACK) == 0 && s.evaluate(b, WHITE) == 0;
}

/**
 * \brief  Vérifie que le meilleur coup correspond au meilleur score retourné.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testBestMoveMatchesScore() {
    Board b;
    Solver s(2, 200, false);
    auto scores = s.scoreMoves(b, BLACK);
    if (scores.empty())
        return false;
    auto bestIt = std::max_element(
        scores.begin(), scores.end(),
        [](auto &a, auto &b){ return a.second < b.second; });
    Move bestByScore = bestIt->first;
    Move best        = s.findBestMove(b, BLACK);
    return best.row == bestByScore.row && best.col == bestByScore.col;
}

int main() {
    startSection("Solver Tests");
    runTest("Default depth/time settings",               testDefaultSettings);
    runTest("Set & get depth/time",                      testSetAndGetDepthTime);
    runTest("Deterministic best move",                   testDeterministicBestMove);
    runTest("scoreMoves() stability",                    testScoreMovesStability);
    runTest("Zobrist hash determinism",                  testZobristHashDeterminism);
    runTest("Zobrist hash changes on move",              testZobristHashChangesOnMove);
    runTest("Empty scoreMoves() when no moves",          testScoreMovesNoMoves);
    runTest("Parallel vs sequential consistency",        testParallelSequentialConsistency);
    runTest("Zero time = depth-only fallback",           testZeroTimeFallback);
    runTest("Immediate all-flip move detection",         testHighFlipMove);
    runTest("Evaluate() reflects mobility advantage",    testEvaluateMobility);
    runTest("Evaluate() on terminal board == disc diff", testEvaluateTerminalBoard);
    runTest("BestMove matches highest score",            testBestMoveMatchesScore);
    return printSummaryAndReturn();
}
