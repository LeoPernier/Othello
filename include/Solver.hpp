// include/Solver.hpp

#pragma once

#include "Board.hpp"

#include <vector>
#include <cstdint>
#include <utility>
#include <chrono>

/**
 * @brief Paramètres de configuration du solver Othello.
 */
struct SolverSettings {
  int searchDepth  = 5;
  int time         = 500;
  bool useParallel = false;
};

/**
 * @brief Entrée d'une table de transposition (TT).
 *
 * Utilisée pour stocker des résultats intermédiaires afin d'éviter
 * des recalculs lors de l'exploration de l'arbre de recherche.
 */
struct TTEntry {
  uint64_t key;
  int      depth, score;
  Move     bestMove;
};

/**
 * @brief Solver pour le jeu Othello/Reversi, implémentant un algorithme Negamax
 *        avec table de transposition et option de parallélisation.
 */
class Solver {
  public:
    /**
     * @brief             Constructeur avec paramètres explicites
     * @param depth       Profondeur maximale de recherche.
     * @param time        Temps limite en ms pour trouver un coup.
     * @param useParallel Active la recherche parallèle si true.
     */
    Solver(int depth = 5, int time = 500, bool useParallel = false);

    /**
     * @brief   Constructeur depuis une configuration.
     * @param s Structure SolverSettings contenant les paramètres.
     */
    Solver(const SolverSettings& s);

    /**
     * @brief   Trouve le meilleur coup pour un joueur donné.
     * @param b Plateau actuel.
     * @param p Joueur pour lequel chercher le coup.
     * @return  Coup optimal selon l'évaluation du solver.
     */
    Move getBestMove( const Board& b,     Player p) { return findBestMove(b, static_cast<int>(p)); }

    /**
     * @brief       Calcule le meilleur coup pour une couleur donnée.
     * @param board Plateau actuel.
     * @param color Couleur du joueur (BLACK ou WHITE).
     * @return       Coup optimal trouvé.
     */
    Move findBestMove(const Board& board, int    color);

    /**
     * @brief       Évalue et attribue un score à chaque coup possible.
     * @param board Plateau actuel.
     * @param color Couleur du joueur.
     * @return      Liste de paires (coup, score).
     */
    std::vector<std::pair<Move,int>> scoreMoves(const Board& board, int color);

    /**
     * @brief Réinitialise la table de transposition.
     */
    void resetTT();

    /**
     * @brief Retourne la profondeur de recherche actuelle.
     */
    int  getDepth() const    { return depth; }
  
    /**
     * @brief Retourne le temps maximum autorisé pour la recherche.
     */
    int  getTime()  const    { return time; }

    /**
     * @brief Définit la profondeur de recherche.
     */
    void setDepth(   int d)  { depth       = d; }

    /**
     * @brief Définit le temps maximum autorisé (ms).
     */
    void setTime(    int t)  { time        = t; }

    /**
     * @brief Active ou désactive la recherche parallèle.
     */
    void setParallel(int up) { useParallel = up; }

  private:
    int  depth, time;
    bool useParallel;
    std::vector<TTEntry>  tt;
    std::vector<uint64_t> zobristTable;
public:
    /**
     * @brief       Calcule la clé Zobrist pour un plateau donné.
     * @param board Plateau de jeu.
     * @return      Clé unique représentant cet état
     */
    uint64_t          zobristHash(const Board& board) const;

    /**
     * @brief       Algorithme Negamax avec élagage alpha-beta.
     * @param board Plateau courant.
     * @param color Couleur du joueur actuel.
     * @param depth Profondeur restante.
     * @param alpha Borne alpha pour l'élagage.
     * @param beta  Borne beta pour l'élagage.
     * @return      Score évalué pour la position.
     */
    int               negamax(    const Board& board, int color, int depth, int alpha, int beta);

    /**
     * @brief       Trie les coups possibles pour optimiser la recherche.
     * @param board Plateau actuel.
     * @param color Couleur du joueur.
     * @return      Liste des coups triés.
     */
    std::vector<Move> orderMoves( const Board& board, int color) const;

    /**
     * @brief       Évalue la position actuelle du plateau.
     * @param board Plateau actuel.
     * @param color Couleur du joueur.
     * @return      Score numérique de l'évaluation (plus haut = meilleur)
     */
    int               evaluate(   const Board& board, int color) const;
};
