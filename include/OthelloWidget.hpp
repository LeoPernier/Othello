// include/OthelloWidget.hpp

#pragma once

#include "Board.hpp"
#include "Solver.hpp"
#include "PointsWidget.hpp"
#include "SolverSettingsDialog.hpp"

#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QScrollBar>
#include <QTimer>
#include <QDockWidget>
#include <QTextEdit>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWidget>
#include <QPainter>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QComboBox>

/**
 * @brief Structure contenant les résultats comparatifs entre l'exécution série et parallèle.
 */
struct CompareResult {
    Move   serieMove, paralleleMove;
    qint64 serieNs,   paralleleNs;
};

/**
 * @brief Fenêtre principale du jeu Othello, gérant l'affichage, les interactions utilisateur et la logique de jeu.
 */
class OthelloWidget : public QMainWindow {
    Q_OBJECT

  public:
    /**
     * @brief        Constructeur de OthelloWidget.
     * @param parent Widget parent (optionnel).
     */
    explicit OthelloWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructeur par défaut.
     */
    ~OthelloWidget() = default;

    /**
     * @brief Retourne la taille en pixels d'une cellule du plateau.
     */
    int getCellSize()         const { return cellSize; }

    /**
     * @brief Retourne la hauteur de la zone d'affichage des points.
     */
    int getPointsPaneHeight() const { return cellSize; }

  protected:
    /**
     * @brief       Gestion du rendu graphique de la fenêtre.
     * @param event Événement de peinture.
     */
    void paintEvent(QPaintEvent      *event) override;

    /**
     * @brief       Gestion des clics souris pour placer des pions.
     * @param event Événement de clic.
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief       Gestion du redimensionnement de la fenêtre.
     * @param event Événement de redimensionnement.
     */
    void resizeEvent(QResizeEvent    *event) override;

  private slots:
    /**
     * @brief Lance une nouvelle partie en mode série.
     */
    void newGameSerial();

    /**
     * @brief Lance une nouvelle partie en mode parallèle.
     */
    void newGameParallel();

    /**
     * @brief Lance une partie en comparant série et parallèle.
     */
    void newGameCompare();

    /**
     * @brief Annule le dernier coup joué.
     */
    void undoLastMove();

    /**
     * @brief Rétablit un coup annulé précédemment.
     */
    void redoLastMove();

    /**
     * @brief Active ou désactive l'affichage des statistiques.
     */
    void toggleStats();

    /**
     * @brief Active ou désactive l'affichage des points.
     */
    void togglePoints();

    /**
     * @brief Demande au solver de jouer un coup.
     */
    void makeSolverMove();

    /**
     * @brief Affiche les informations sur l'application.
     */
    void showInfo();

    /**
     * @brief Slot appelé quand la visibilité du panneau de stats change.
     * @param visible true si le panneau est visible.
     */
    void onStatsVisibilityChanged(bool visible);

    /**
     * @brief Affiche la zone des points.
     */
    void showPoints();

    /**
     * @brief Slot appelé quand le solver a terminé son calcul.
     */
    void onSolverFinished();

  private:
    /**
     * @brief Initialise la barre de menu et ses actions.
     */
    void initMenu();

    enum Mode { SERIE, PARALLELE, COMPARE };
    Mode gameMode = SERIE;

    Board  board;
    Solver solver;
    int    playerColor = BLACK, solverColor = WHITE, currentTurn = BLACK;

    const int cellSize = 60;
    const int padding  = 20;
    const int margin   = 20;

    QSize baseWindowSize;
    int   statsPaneWidth = 350;
    QDockWidget::DockWidgetFeatures defaultDockFeatures;

    QAction* toggleStatsAction  = nullptr;
    QAction* pointsAction       = nullptr;
    QAction* undoAction         = nullptr;
    QAction* redoAction         = nullptr;
    QAction* exportAction       = nullptr;
    QAction* importAction       = nullptr;
    bool     ttEnabled          = true;
    QAction* toggleTTAction     = nullptr;

    bool         statsOn        = false;
    QDockWidget* statsDock      = nullptr;
    QTextEdit*   statsLog       = nullptr;

    QDockWidget*  pointsDock    = nullptr;
    PointsWidget* pointsWidget  = nullptr;
    bool          pointsVisible = false;

    bool  resizingProgrammatically = false;
    bool  passPending              = false;
    int   passStreak               = 0;
    QSize lastProgrammaticSize;

    enum class SolverType { None,     Serie,      Parallele };
    enum class ActionType { UserMove, SolverMove, UserPass, SolverPass };
    QFutureWatcher<QPair<Move, qint64>>* solverWatcher;
    int solverGen = 0;

    /**
     * @brief Structure représentant une action dans l'historique de la partie.
     */
    struct Action {
        ActionType type;
        Move       move;
        int        solverTurnIndex = 0;
        SolverType solverType      = SolverType::None;
        qint64     elapsedNs       = 0;
    };
    QVector<Action>                  actionHistory;
    QVector<Action>                  redoBuffer;
    QFutureWatcher<QVector<Action>>* importWatcher;

    QComboBox* timeUnitCombo = nullptr;

    /**
     * @brief Rejoue toutes les actions de l'historique pour reconstruire l'état du plateau.
     */
    void replayHistory();

    /**
     * @brief Met à jour le log des statistiques.
     */
    void updateStatsLog();

    /**
     * @brief Met à jour l'affichage des points.
     */
    void updatePoints();

    /**
     * @brief Recalcule la taille de la fenêtre en fonction du plateau et des panneaux.
     */
    void recalcWindowSize();

    /**
     * @brief Exporte la partie courante dans un fichier.
     */
    void exportGame();

    /**
     * @brief Importe une partie depuis un fichier.
     */
    void importGame();

    /**
     * @brief Slot appelé quand l'import est terminé.
     */
    void onImportFinished();

    /**
     * @brief Vérifie si un joueur doit passer son tour.
     */
    void checkPass();

    /**
     * @brief   Redimensionne la fenêtre à des dimensions précises
     * @param w Largeur.
     * @param h Hauteur.
     */
    void performResize(int w, int h);

    /**
     * @brief Met à jour le titre de la fenêtre.
     */
    void updateWindowTitle();
};
