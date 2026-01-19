// include/Board.hpp

#pragma once

#include <vector>

#define BOARD_SIZE 8

/**
 * @brief Représente l'état d'une cellule du plateau.
 */
enum Cell {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
};

/**
 * @brief Représente un coup possible sur le plateau.
 */
struct Move {
    int row, col;
    bool operator==(const Move& other) const { return row==other.row && col==other.col; }
    bool operator!=(const Move& other) const { return !(*this==other); }
};

/**
 * @brief Classe représentant un plateau de jeu pour Othello
 */
class Board {
  public:
    /**
     * @brief Types de disques possibles.
     */
    enum class Disk   : int { Empty = Cell::EMPTY, Black = Cell::BLACK, White = Cell::WHITE };

    /**
     * @brief Joueurs possibles.
     */
    enum class Player : int { Black = Cell::BLACK, White = Cell::WHITE };

    /**
     * @brief Constructeur par défaut: Crée un plateau de taille standard (8x8).
     */
    Board()         : size_(BOARD_SIZE) { init(); }

    /**
     * @brief      Constructeur avec taille personnalisée.
     * @param size Taille du plateau.
     */
    Board(int size) : size_(size)       { init(); }

    /**
     * @brief Destructeur.
     */
    ~Board() = default;

    /**
     * @brief  Retourne la taille du plateau.
     * @return Nombre de cases sur un côté.
     */
    int getSize() const { return size_; }

    /**
     * @brief   Retourne le disque présent à une position donnée.
     * @param r Ligne.
     * @param c Colonne.
     * @return  Type de disque à cette position.
     */
    Disk getDisk(int r,int c) const { return static_cast<Disk>(cells[r][c]); }

    /**
     * @brief Vide entièrement le plateau.
     */
    void clear() {
        for (int i = 0; i < BOARD_SIZE; ++i)
          for (int j = 0; j < BOARD_SIZE; ++j)
            cells[i][j] = EMPTY;
    }

    /**
     * @brief       Définit une cellule du plateau.
     * @param row   Ligne.
     * @param col   Colonne.
     * @param color Couleur (EMPTY, BLACK, WHITE).
     */
    void setCell(int row, int col, int color);

    /**
     * @brief   Place un disque à une position donnée.
     * @param m Coup à jouer.
     * @param d Type de disque à placer.
     */
    void setDisk(const Move& m, Disk d) {
      cells[m.row][m.col] = static_cast<int>(d);
    }

    /**
     * @brief   Retourne la liste des coups valides pour un joueur.
     * @param p Joueur (Black ou White)
     * @return  Liste des coups valides.
     */
    std::vector<Move> getValidMoves(Player p) const {
      return getValidMoves(static_cast<int>(p));
    }
  
    /**
     * @brief   Applique un coup pour un joueur donné.
     * @param m Coup à jouer.
     * @param p Joueur (Black ou White).
     */
    void applyMove(const Move& m, Player p) {
      applyMove(m, static_cast<int>(p));
    }

    /**
     * @brief Affiche le plateau dans la console.
     */
    void display() const;

    /**
     * @brief   Vérifie si une position est sur le plateau.
     * @param r Ligne.
     * @param c Colonne.
     * @return  true si la position est valide, false sinon.
     */
    bool onBoard(int r,int c) const;

    /**
     * @brief       Vérifie si un coup est valide pour un joueur donné.
     * @param r     Ligne.
     * @param c     Colonne.
     * @param color Couleur du joueur (BLACK ou WHITE).
     * @return      true si le coup est valide, false sinon.
     */
    bool isValidMove(int r,int c,int color) const;

    /**
     * @brief       Applique un coup (et retourne les pions adverses capturés) pour une couleur donnée.
     * @param move  Coup à jouer.
     * @param color Couleur du joueur.
     */
    void applyMove(const Move& move,int color);

    /**
     * @brief       Retourne la liste des coups valides pour une couleur donnée.
     * @param color Couleur du joueur (BLACK ou WHITE).
     * @return      Liste des coups valides.
     */
    std::vector<Move> getValidMoves(int color) const;

    /**
     * @brief       Vérifie si le joueur a au moins un coup valide.
     * @param color Couleur du joueur.
     * @return      true si au moins un coup valide existe, false sinon.
     */
    bool hasAnyValidMove(int color) const;

    /**
     * @brief  Vérifie si la partie est terminée.
     * @return true si aucun joueur ne peut jouer, false sinon.
     */
    bool isGameOver() const;

    /**
     * @brief       Compte le nombre de disques d'une couleur donnée
     * @param color Couleur (BLACK ou WHITE).
     * @return       Nombre de disques de cette couleur.
     */
    int count(int color) const;

    /**
     * @brief     Retourne la valeur brute de la cellule.
     * @param row Ligne.
     * @param col Colonne.
     * @return    Valeur brute de la cellule (EMPTY, BLACK, WHITE)
     */
    int cellAt(int row,int col) const { return cells[row][col]; }

  private:
    /**
     * @brief Initialise le plateau avec les 4 pions centraux.
     */
    void init();

    int size_;
    int cells[BOARD_SIZE][BOARD_SIZE];
};

using Disk   = Board::Disk;
using Player = Board::Player;
