// src/Board.cpp

#include "../include/Board.hpp"

#include <iostream>
#include <vector>
#include <utility>

void Board::display() const {
    std::cout << "  ";
    for (int j = 0; j < BOARD_SIZE; j++)
        std::cout << j << " ";
    std::cout << "\n";
    for (int i = 0; i < BOARD_SIZE; i++) {
        std::cout << i << " ";
        for (int j = 0; j < BOARD_SIZE; j++) {
            char c = '.';
            if (cells[i][j] == BLACK)
                c = 'B';
            else if (cells[i][j] == WHITE)
                c = 'W';
            std::cout << c << " ";
        }
        std::cout << "\n";
    }
}

bool Board::onBoard(int row, int col) const {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

bool Board::isValidMove(int row, int col, int color) const {
    if (!onBoard(row, col) || cells[row][col] != EMPTY)
        return false;
    int opponent = (color == BLACK ? WHITE : BLACK);
    // Directions to check
    const int dr[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    const int dc[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    for (unsigned int d = 0; d < 8; d++) {
        int i = row + dr[d], j = col + dc[d];
        bool seenOpponent = false;

        // First must see at least one opponent disc
        while (onBoard(i, j) && cells[i][j] == opponent) {
            seenOpponent = true;
            i += dr[d];
            j += dc[d];
        }

        // Then must end on own color
        if (seenOpponent && onBoard(i, j) && cells[i][j] == color)
            return true;
    }
    return false;
}

void Board::applyMove(const Move& move, int color) {
    cells[move.row][move.col] = color;
    int opponent = (color == BLACK ? WHITE : BLACK);
    const int dr[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    const int dc[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    for (int d = 0; d < 8; d++) {
        int i = move.row + dr[d], j = move.col + dc[d];
        std::vector<std::pair<int,int>> toFlip;
        while (onBoard(i, j) && cells[i][j] == opponent) {
            toFlip.emplace_back(i, j);
            i += dr[d];
            j += dc[d];
        }
        if (!toFlip.empty() && onBoard(i, j) && cells[i][j] == color)
            for (auto& p : toFlip)
                cells[p.first][p.second] = color;
    }
}

std::vector<Move> Board::getValidMoves(int color) const {
    std::vector<Move> moves;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (isValidMove(i, j, color))
                moves.push_back({ i, j });
    return moves;
}

bool Board::hasAnyValidMove(int color) const {
    return !getValidMoves(color).empty();
}

bool Board::isGameOver() const {
    return !hasAnyValidMove(BLACK) && !hasAnyValidMove(WHITE);
}

int Board::count(int color) const {
    int count = 0;
    for (unsigned int i = 0; i < BOARD_SIZE; i++)
        for (unsigned int j = 0; j < BOARD_SIZE; j++)
            if (cells[i][j] == color)
                count++;
    return count;
}

void Board::init() {
    // On initialise le Board
    for (unsigned int i = 0; i < BOARD_SIZE; i++)
        for (unsigned int j = 0; j < BOARD_SIZE; j++)
            cells[i][j] = EMPTY;
    
    // On initialise les 4 pieces de depart
    cells[3][3] = WHITE;
    cells[3][4] = BLACK;
    cells[4][3] = BLACK;
    cells[4][4] = WHITE;
}


void Board::setCell(int row, int col, int color) {
    if (onBoard(row, col))
        cells[row][col] = color;
}
