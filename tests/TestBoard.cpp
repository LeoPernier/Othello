// tests/BoardTests.cpp

#include "../include/Board.hpp"
#include "TestUtils.hpp"

#include <vector>

/**
 * \brief  Vérifie les compteurs initiaux : 2 noirs, 2 blancs et le reste de cases vides.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testInitialCounts() {
    Board b;
    int blacks  = b.count(BLACK);
    int whites  = b.count(WHITE);
    int empties = b.count(EMPTY);
    return blacks == 2 && whites == 2 && empties == (BOARD_SIZE * BOARD_SIZE - 4);
}

/**
 * \brief  Vérifie la liste des coups valides au plateau initial.
 * \return true si le test/résultat est conforme, false sinon
 */
bool testInitialValidMoves() {
    Board b;
    return b.getValidMoves(BLACK).size() == 4 && b.getValidMoves(WHITE).size() == 4;
}

/**
 * \brief  Vérifie la capture (flip) horizontale des pions adverses après un coup valide.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testHorizontalFlip() {
    Board b;
    b.clear();
    b.setCell(3, 2, BLACK);
    b.setCell(3, 3, BLACK);
    b.setCell(3, 4, WHITE);
    auto mv = Move{3, 5};
    auto moves = b.getValidMoves(BLACK);
    bool found = false;
    for (auto m : moves)
        if (m.row == 3 && m.col == 5)
        found = true;
    if (!found)
        return false;
    b.applyMove(mv, BLACK);
    return b.cellAt(3, 2) == BLACK && b.cellAt(3, 3) == BLACK;
}

/**
 * \brief  Vérifie la capture (flip) verticale des pions adverses après un coup valide.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testVerticalFlip() {
    Board b;
    b.clear();
    b.setCell(2, 4, WHITE);
    b.setCell(3, 4, WHITE);
    b.setCell(4, 4, BLACK);
    Move mv{5, 4};
    auto moves = b.getValidMoves(WHITE);
    bool found = false;
    for (auto m : moves)
        if(m.row == 5 && m.col == 4)
            found = true;
    if (!found)
        return false;
    b.applyMove(mv, WHITE);
    return b.cellAt(2, 4) == WHITE && b.cellAt(3, 4) == WHITE;
}

/**
 * \brief  Vérifie la capture (flip) diagonale Nord-Ouest -> Sud-Est.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testDiagonalFlipNWSE() {
    Board b;
    b.clear();
    b.setCell(3, 3, BLACK);
    b.setCell(4, 4, BLACK);
    b.setCell(5, 5, WHITE);
    Move mv{2, 2};
    auto moves = b.getValidMoves(WHITE);
    bool found = false;
    for (auto m : moves)
        if (m.row == 2 && m.col == 2)
            found = true;
    if (!found)
        return false;
    b.applyMove(mv, WHITE);
    return b.cellAt(3, 3) == WHITE && b.cellAt(4, 4) == WHITE;
}

/**
 * \brief  Vérifie la capture (flip) diagonale Nord-Est -> Sud-Ouest
 * \return true si le test/résultat est conforme, false sinon
 */
bool testDiagonalFlipNESW() {
    Board b;
    b.clear();
    b.setCell(3, 4, BLACK);
    b.setCell(4, 3, BLACK);
    b.setCell(5, 2, WHITE);
    Move mv{2, 5};
    auto moves = b.getValidMoves(WHITE);
    bool found = false;
    for (auto m : moves)
        if (m.row == 2 && m.col == 5)
            found = true;
    if (!found)
        return false;
    b.applyMove(mv, WHITE);
    return b.cellAt(3, 4) == WHITE && b.cellAt(4, 3) == WHITE;
}

/**
 * \brief  Vérifie qu’un coup peut retourner des pions sur plusieurs directions simultanément.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testMultiDirectionFlip() {
    Board b;
    b.clear();
    b.setCell(3, 3, BLACK);
    b.setCell(3, 4, WHITE);
    b.setCell(4, 2, BLACK);
    b.setCell(5, 2, WHITE);
    Move mv{3, 2};
    auto moves = b.getValidMoves(WHITE);
    bool found = false;
    for (auto m : moves)
        if (m.row == 3 && m.col == 2)
            found = true;
    if (!found)
        return false;
    b.applyMove(mv, WHITE);
    return b.cellAt(3, 3) == WHITE && b.cellAt(4, 2) == WHITE;
}

/**
 * \brief  Vérifie qu’un coup illégal n’apparaît pas dans la liste des coups valides
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testIllegalMoveNotInList() {
    Board b;
    b.clear();
    auto moves = b.getValidMoves(BLACK);
    return moves.empty();
}

/**
 * \brief  Vérifie que la liste des coups valides est vide dans une position vide totale.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testValidMovesOnEmptyOnly() {
    Board b;
    for (auto m : b.getValidMoves(WHITE))
        if (b.cellAt(m.row, m.col) != EMPTY)
            return false;
    return true;
}

/**
 * \brief  Vérifie qu’un coup explicitement illégal est correctement exclu.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testKnownIllegalExcluded() {
    Board b;
    for(auto m : b.getValidMoves(BLACK))
        if(m.row == 0 && m.col == 0)
            return false;
    return true;
}

/**
 * \brief  Vérifie la détection du « pass » quand aucun coup n’est possible pour le joueur courant.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testPassDetection() {
    Board b;
    b.clear();
    b.setCell(1, 1, BLACK);
    b.setCell(2, 2, WHITE);
    b.setCell(3, 3, BLACK);
    return  b.hasAnyValidMove(WHITE) && !b.hasAnyValidMove(BLACK);
}

/**
 * \brief  Vérifie la transition vers fin de partie lorsqu’aucun joueur n’a de coup (plateau vide).
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testNoMovesEmptyBoard() {
    Board b;
    b.clear();
    return !b.hasAnyValidMove(BLACK) && !b.hasAnyValidMove(WHITE);
}

/**
 * \brief  Vérifie que le constructeur de copie du plateau duplique l’état sans aliasing.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testCopyConstructor() {
    Board a;
    a.clear();
    a.setCell(0, 1, BLACK);
    a.setCell(0, 2, WHITE);
    Board b(a);
    b.applyMove(Move{0, 0}, WHITE);
    return a.cellAt(0, 0) == EMPTY && b.cellAt(0, 0) == WHITE;
}

/**
 * \brief  Vérifie que l’opérateur d’affectation copie l’état sans lien partagé.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testAssignmentOperator() {
    Board a;
    a.clear();
    a.setCell(1, 1, BLACK);
    Board b = a;
    b.applyMove(Move{1, 0}, WHITE);
    return a.cellAt(1, 0) == EMPTY && b.cellAt(1, 0) == WHITE;
}

/**
 * \brief  Vérifie l’invariant: nbre(Noirs) + nbre(Blancs) + nbre(Vides) = BOARD_SIZE^2.
 * \return true si le test/résultat est conforme, false sinon.
 */
bool testCountSumInvariant() {
    Board b;
    int total = b.count(BLACK) + b.count(WHITE) + b.count(EMPTY);
    return total == BOARD_SIZE*BOARD_SIZE;
}

int main() {
    startSection("Board – full exhaustive tests");
    runTest("Initial Counts",                testInitialCounts);
    runTest("Initial Valid Moves",           testInitialValidMoves);
    runTest("Horizontal Flip",               testHorizontalFlip);
    runTest("Vertical Flip",                 testVerticalFlip);
    runTest("Diag Flip NW->SE",              testDiagonalFlipNWSE);
    runTest("Diag Flip NE->SW",              testDiagonalFlipNESW);
    runTest("Multi-Dir Flip",                testMultiDirectionFlip);
    runTest("Illegal Move Not Listed",       testIllegalMoveNotInList);
    runTest("Valid Moves Are Empty",         testValidMovesOnEmptyOnly);
    runTest("Known Illegal Excluded",        testKnownIllegalExcluded);
    runTest("Pass Detection",                testPassDetection);
    runTest("No-Moves Empty->GameOver",      testNoMovesEmptyBoard);
    runTest("Copy Constructor Isolation",    testCopyConstructor);
    runTest("Assignment Operator Isolation", testAssignmentOperator);
    runTest("Count Sum Invariant",           testCountSumInvariant);
    return printSummaryAndReturn();
}
