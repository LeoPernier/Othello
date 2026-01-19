// src/Solver.cpp

#include "../include/Solver.hpp"

#include <random>
#include <algorithm>
#include <limits>
#include <chrono>

#ifdef _OPENMP
    #include <omp.h>
#endif

static constexpr int TT_SIZE = 1 << 20;

Solver::Solver(int depth, int time, bool useParallel)
    : depth(depth), time(time), useParallel(useParallel),
    tt(TT_SIZE), zobristTable(BOARD_SIZE * BOARD_SIZE * 2) {

    std::mt19937_64 rng(std::random_device{}());
    for (auto& z : zobristTable)
        z = rng();
}

Solver::Solver(const SolverSettings& s) : Solver(s.searchDepth, s.time, s.useParallel) {}

Move Solver::findBestMove(const Board& board, int color) {
    auto results = scoreMoves(board, color);
    auto it      = std::max_element(
        results.begin(), results.end(),
        [](auto &a, auto &b){ return a.second < b.second; }
    );
    return (it != results.end() ? it->first : Move{-1, -1});
}

std::vector<std::pair<Move, int>>
Solver::scoreMoves(const Board& board, int color) {
    auto moves = board.getValidMoves(color);
    std::vector<std::pair<Move,int>> results(moves.size());

    #pragma omp parallel for if(useParallel) schedule(dynamic)
    for (int i = 0; i < (int)moves.size(); ++i) {

        Board child = board;
        child.applyMove(moves[i], color);

        int sc = -negamax(
            child,
            3 - color,
            depth - 1,
            std::numeric_limits<int>::min() + 1,
            std::numeric_limits<int>::max()
        );
        results[i] = {moves[i], sc};
    }
    return results;
}

uint64_t Solver::zobristHash(const Board& board) const {
    uint64_t h = 0;

    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            int c = board.cellAt(i, j);
            if (c == BLACK)      h ^= zobristTable[(i * BOARD_SIZE + j) * 2 + 0];
            else if (c == WHITE) h ^= zobristTable[(i * BOARD_SIZE + j) * 2 + 1];
        }
    }
    return h;
}

int Solver::negamax(const Board& board, int color, int depth, int alpha, int beta) {
    if (depth == 0 || board.isGameOver()) {
        if (board.isGameOver()) {
            int finalScore = board.count(BLACK) - board.count(WHITE);
            return (color == BLACK ? finalScore : -finalScore);
        }
        return evaluate(board, color);
    }
    uint64_t key   = zobristHash(board) ^ (uint64_t)color;
    auto&    entry = tt[key % TT_SIZE];
    if (entry.key == key && entry.depth >= depth)
        return entry.score;

    auto moves = board.getValidMoves(color);
    if (moves.empty()) {
        int score = -negamax(board, 3 - color, depth - 1, -beta, -alpha);
        entry     = {key, depth, score, {-1, -1}};
        return score;
    }

    moves         = orderMoves(board, color);
    int bestScore = std::numeric_limits<int>::min();
    Move bestLocal{-1, -1};
    for (auto& m : moves) {
        Board child = board;
        child.applyMove(m, color);
        int score = -negamax(child, 3 - color, depth - 1, -beta, -alpha);
        if (score > bestScore) {
            bestScore = score;
            bestLocal = m;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) break;
    }

    entry.key      = key;
    entry.depth    = depth;
    entry.score    = bestScore;
    entry.bestMove = bestLocal;
    return bestScore;
}

int Solver::evaluate(const Board& board, int color) const {
    int myCount  = board.count(color);
    int oppCount = board.count(3 - color);
    int discDiff = 100 * (myCount - oppCount) / (myCount + oppCount + 1);

    int myMoves  = (int)board.getValidMoves(color).size();
    int oppMoves = (int)board.getValidMoves(3 - color).size();
    int mobility = 100 * (myMoves - oppMoves) / (myMoves + oppMoves + 1);

    int cornerMy = 0, cornerOpp = 0;
    const std::vector<std::pair<int,int>> corners = { {0, 0}, {0, 7}, {7, 0}, {7, 7} };
    for (auto& c : corners) {
        int cell = board.cellAt(c.first, c.second);
        if (cell == color)
            cornerMy++;
        else if (cell == 3-color)
            cornerOpp++;
    }
    int cornerScore = 500 * (cornerMy - cornerOpp);

    return discDiff + mobility + cornerScore;
}

std::vector<Move> Solver::orderMoves(const Board& board, int color) const {
    auto moves = board.getValidMoves(color);
    std::sort(moves.begin(), moves.end(),
        [&](const Move& a, const Move& b) {
            bool aCorner = ((a.row == 0 || a.row == 7) && (a.col == 0 || a.col == 7));
            bool bCorner = ((b.row == 0 || b.row == 7) && (b.col == 0 || b.col == 7));
            return (int)aCorner > (int)bCorner;
        });
    return moves;
}

void Solver::resetTT() {
    for (auto& entry : tt) {
        entry.key      = 0;
        entry.depth    = 0;
        entry.score    = 0;
        entry.bestMove = Move{-1, -1};
    }
}
