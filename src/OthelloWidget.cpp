// src/OthelloWidget.cpp

#include "../include/OthelloWidget.hpp"
#include "../include/PointsWidget.hpp"
#include "SolverSettingsDialog.hpp"

#include <QCoreApplication>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QFileDialog>
#include <QDir>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QKeySequence>
#include <QDataStream>
#include <QScrollBar>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QTimer>
#include <algorithm>
#include <QStringList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QJsonObject>
#include <QElapsedTimer>

QString formatBestTime(qint64 ms, qint64 ns) {
    if (ms >= 10000) {
        double sec = ms / 1000.0;
        return QString::number(sec, 'f', 3) + " s";
    } else if (ms >= 10)
        return QString::number(ms) + " ms";
    else if (ns >= 10000) {
        double us = ns / 1e3;
        return QString::number(us, 'f', 0) + " μs";
    } else
        return QString::number(ns) + " ns";
}

OthelloWidget::OthelloWidget(QWidget *parent) : QMainWindow(parent), solver() {
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint & ~Qt::WindowMaximizeButtonHint);

    statsDock = new QDockWidget("Solver Stats", this);
    statsLog  = new QTextEdit();
    statsLog->setReadOnly(true);
    statsDock->setWidget(statsLog);
    statsDock->setFeatures(QDockWidget::DockWidgetClosable);
    statsDock->setAllowedAreas(Qt::RightDockWidgetArea);
    statsDock->setFixedWidth(statsPaneWidth);
    addDockWidget(Qt::RightDockWidgetArea, statsDock);
    connect(statsDock, &QDockWidget::visibilityChanged, this, &OthelloWidget::onStatsVisibilityChanged);
    statsDock->hide();

    QWidget*     statsWidget = new QWidget();
    QVBoxLayout* statsLayout = new QVBoxLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(4);

    timeUnitCombo = new QComboBox();
    timeUnitCombo->addItems({"Best", "s", "ms", "μs", "ns"});
    timeUnitCombo->setCurrentIndex(0);
    statsLayout->addWidget(timeUnitCombo, 0, Qt::AlignRight);

    statsLayout->addWidget(statsLog);
    statsDock->setWidget(statsWidget);

    connect(timeUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OthelloWidget::updateStatsLog);

    pointsWidget = new PointsWidget(this);
    pointsWidget->setFixedHeight(cellSize);
    pointsDock = new QDockWidget(QString(), this);
    pointsDock->setWidget(pointsWidget);
    pointsDock->setAllowedAreas(Qt::TopDockWidgetArea);
    pointsDock->setFeatures(QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::TopDockWidgetArea, pointsDock);
    connect(pointsDock, &QDockWidget::visibilityChanged, this, [this](bool visible){
        pointsVisible = visible;
        recalcWindowSize();
    });
    pointsDock->hide();
    pointsDock->setFixedWidth(BOARD_SIZE * cellSize + 2 * padding);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCentralWidget(centralWidget);

    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(margin, margin, margin, margin);
    centralLayout->setSpacing(0);
    centralWidget->setLayout(centralLayout);

    initMenu();

    solverWatcher = new QFutureWatcher<QPair<Move, qint64>>(this);
    connect(solverWatcher, &QFutureWatcher<QPair<Move, qint64>>::finished, this, &OthelloWidget::onSolverFinished);

    int menuH          = menuBar()->sizeHint().height();
    int gridSize       = BOARD_SIZE * cellSize;
    int totalBoardSize = gridSize + 2 * padding;
    int baseW          = totalBoardSize + 2 * margin;
    int baseH          = menuH + totalBoardSize + 2 * margin;
    setMinimumSize(baseW, baseH);
    baseWindowSize = QSize(baseW, baseH);
    lastProgrammaticSize = baseWindowSize;
    setMaximumSize(baseW, baseH);
    recalcWindowSize();

    importWatcher = new QFutureWatcher<QVector<Action>>(this);
    connect(importWatcher, &QFutureWatcher<QVector<Action>>::finished, this, &OthelloWidget::onImportFinished);
    newGameSerial();
}

void OthelloWidget::initMenu() {
    QMenuBar* mb = menuBar();

    QMenu* fileMenu = mb->addMenu("File");
    fileMenu->addAction("New Game (serie)",     this, &OthelloWidget::newGameSerial);
    fileMenu->addAction("New Game (parallele)", this, &OthelloWidget::newGameParallel);
    fileMenu->addAction("New Game (compare)",   this, &OthelloWidget::newGameCompare);

    fileMenu->addSeparator();
    exportAction = fileMenu->addAction("Export Game…", this, &OthelloWidget::exportGame);
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    importAction = fileMenu->addAction("Import Game…", this, &OthelloWidget::importGame);
    importAction->setShortcut(QKeySequence("Ctrl+I"));

    fileMenu->addSeparator();
    fileMenu->addAction("Exit",                 this, &OthelloWidget::close);

    QMenu* gameMenu = mb->addMenu("Game");
    undoAction = gameMenu->addAction("Undo", this, &OthelloWidget::undoLastMove);
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    redoAction = gameMenu->addAction("Redo", this, &OthelloWidget::redoLastMove);
    redoAction->setShortcut(QKeySequence("Ctrl+Y"));

    gameMenu->addSection("Display Options");
    toggleStatsAction = gameMenu->addAction("Stats", this, &OthelloWidget::toggleStats);
    toggleStatsAction->setShortcut(QKeySequence("Ctrl+T"));
    pointsAction = gameMenu->addAction("Points", this, &OthelloWidget::togglePoints);
    pointsAction->setShortcut(QKeySequence("Ctrl+P"));

    QAction* solverSettingsAction = gameMenu->addAction("Solver Settings…", this, [this]() {

        SolverSettingsDialog solverSettings(solver.getDepth(), solver.getTime(), this);

        if (solverSettings.exec() == QDialog::Accepted) {
            solver.setDepth(solverSettings.getDepth());
            solver.setTime(solverSettings.getTime());

            QMessageBox::information(
                this,
                "Solver Settings Updated",
                QString("Profondeur: %1\nTemps max: %2 ms").arg(solver.getDepth()).arg(solver.getTime())
            );
            updateWindowTitle();
        }
    });
    solverSettingsAction->setShortcut(QKeySequence("Ctrl+L"));

    toggleTTAction = gameMenu->addAction("Transposition Table (ON)");
    toggleTTAction->setCheckable(true);
    toggleTTAction->setChecked(true);
    toggleTTAction->setShortcut(QKeySequence("Ctrl+M"));
    connect(toggleTTAction, &QAction::toggled, this, [this](bool checked) {
        ttEnabled = checked;
        toggleTTAction->setText(QString("Transposition Table (%1)").arg(checked ? "ON" : "OFF"));
        if (!ttEnabled) solver.resetTT();
        updateWindowTitle();
    });

    QMenu* helpMenu = mb->addMenu("Help");
    helpMenu->addAction("Information",  this, &OthelloWidget::showInfo);
}


void OthelloWidget::newGameSerial() {
    solverGen++;

    board  = Board();
    actionHistory.clear();
    redoBuffer.clear();
    passPending = false;
    passStreak  = 0;
    replayHistory();
    updateStatsLog();
    currentTurn = BLACK;
    gameMode    = SERIE;

    statsDock->setFeatures(QDockWidget::DockWidgetClosable);
    toggleStatsAction->setEnabled(true);
    statsLog->clear();

    ttEnabled = true;
    toggleTTAction->setChecked(true);
    toggleTTAction->setEnabled(true);

    updateWindowTitle();

    recalcWindowSize();
    solverGen++;
    update();
    checkPass();
}

void OthelloWidget::newGameParallel() {
    solverGen++;

    board  = Board();
    actionHistory.clear();
    redoBuffer.clear();
    passPending = false;
    passStreak  = 0;
    replayHistory();
    updateStatsLog();
    currentTurn = BLACK;
    gameMode    = PARALLELE;

    statsDock->setFeatures(QDockWidget::DockWidgetClosable);
    toggleStatsAction->setEnabled(true);
    statsLog->clear();

    ttEnabled = true;
    toggleTTAction->setChecked(true);
    toggleTTAction->setEnabled(true);

    updateWindowTitle();

    recalcWindowSize();
    solverGen++;
    update();
    checkPass();
}

void OthelloWidget::newGameCompare() {
    solverGen++;

    board  = Board();
    actionHistory.clear();
    redoBuffer.clear();
    passPending = false;
    passStreak  = 0;
    replayHistory();
    updateStatsLog();
    currentTurn = BLACK;
    gameMode    = COMPARE;

    statsOn     = true;
    statsDock->show();
    statsDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    toggleStatsAction->setEnabled(false);
    statsLog->clear();

    ttEnabled = false;
    toggleTTAction->setChecked(false);
    toggleTTAction->setEnabled(false);
    solver.resetTT();

    updateWindowTitle();

    recalcWindowSize();
    solverGen++;
    update();
    checkPass();
}

void OthelloWidget::undoLastMove() {
    if (actionHistory.isEmpty()) return;
    if (actionHistory.back().type == ActionType::SolverMove) {
        redoBuffer.append(actionHistory.takeLast());
        if (!actionHistory.isEmpty() && actionHistory.back().type == ActionType::UserMove) {
            redoBuffer.append(actionHistory.takeLast());
        }
    }
    else {
        redoBuffer.append(actionHistory.takeLast());
    }
    replayHistory();
    updateStatsLog();
    update();
}

void OthelloWidget::redoLastMove() {
    if (redoBuffer.isEmpty()) return;
    if (redoBuffer.back().type == ActionType::UserMove) {
        Action userAct = redoBuffer.takeLast();
        actionHistory.append(userAct);
        if (!redoBuffer.isEmpty() && redoBuffer.back().type == ActionType::SolverMove) {
            Action solAct = redoBuffer.takeLast();
            actionHistory.append(solAct);
        }
    }
    else {
        Action act = redoBuffer.takeLast();
        actionHistory.append(act);
    }
    replayHistory();
    updateStatsLog();
    update();
}

void OthelloWidget::toggleStats() {
    if (gameMode == COMPARE) return;

    statsOn = !statsOn;
    if (statsOn) {
        statsDock->show();
        recalcWindowSize();
    } else {
        statsDock->hide();
        recalcWindowSize();
    }
}

void OthelloWidget::showInfo() {
    QMessageBox::information(this, "Othello Rules",
        "Othello is played on an 8×8 board. Players take turns placing discs. "
        "Any discs of the opponent's color that are in a straight line and "
        "bounded by the disc just placed and another disc of your color are "
        "flipped. Game ends when neither can move; most discs wins.");
}

void OthelloWidget::makeSolverMove() {
    if (passPending) passPending = false;
    if (board.isGameOver()) return;

    if (solverWatcher->isRunning()) return;

    solverGen++;
    solverWatcher->setProperty("gen", solverGen);

    if (gameMode == COMPARE) {
        disconnect(solverWatcher, nullptr, nullptr, nullptr);

        Board boardCopy = board;

        if (!ttEnabled) solver.resetTT();

        QFuture<QPair<Move, qint64>> serieFuture = QtConcurrent::run([this, boardCopy]() {
            QElapsedTimer timer;
            timer.start();
            solver.setParallel(false);
            Move mv = solver.findBestMove(boardCopy, solverColor);
            qint64 ns = timer.nsecsElapsed();
            return QPair<Move, qint64>(mv, ns);
        });

        connect(solverWatcher, &QFutureWatcher<QPair<Move, qint64>>::finished, this, [this, boardCopy]() {
            int gen = solverWatcher->property("gen").toInt();
            if (gen != solverGen) return;

            QPair<Move, qint64> serieResult = solverWatcher->result();
            Move bestMove = serieResult.first;
            qint64 serieTime = serieResult.second;

            if (!ttEnabled) solver.resetTT();

            QFuture<QPair<Move, qint64>> paralleleFuture = QtConcurrent::run([this, boardCopy]() {
                QElapsedTimer timer;
                timer.start();
                solver.setParallel(true);
                Move mv = solver.findBestMove(boardCopy, solverColor);
                qint64 ns = timer.nsecsElapsed();
                return QPair<Move, qint64>(mv, ns);
            });

            disconnect(solverWatcher, nullptr, nullptr, nullptr);
            connect(solverWatcher, &QFutureWatcher<QPair<Move, qint64>>::finished, this, [this, bestMove, serieTime]() {
                int gen = solverWatcher->property("gen").toInt();
                if (gen != solverGen) return;

                QPair<Move, qint64> paralleleResult = solverWatcher->result();
                Move paralleleMove = paralleleResult.first;
                qint64 paralleleTime = paralleleResult.second;

                if (bestMove != paralleleMove)
                    qWarning() << "[WARNING]: Serie and Parallele different!";

                actionHistory.append({ ActionType::SolverMove, bestMove, 0, SolverType::Serie, serieTime });
                actionHistory.append({ ActionType::SolverMove, bestMove, 0, SolverType::Parallele, paralleleTime });

                redoBuffer.clear();
                passPending = false;
                passStreak = 0;
                replayHistory();
                updateStatsLog();
                currentTurn = playerColor;
                updatePoints();
                update();
                checkPass();
            });
            solverWatcher->setFuture(paralleleFuture);
        });
        solverWatcher->setFuture(serieFuture);
    } else {
        disconnect(solverWatcher, nullptr, nullptr, nullptr);

        if (!ttEnabled) solver.resetTT();

        QFuture<QPair<Move, qint64>> fut = QtConcurrent::run([this]() {
            solver.setParallel(gameMode == PARALLELE);
            QElapsedTimer timer;
            timer.start();
            return QPair<Move, qint64>(solver.findBestMove(board, solverColor), timer.nsecsElapsed());
        });

        connect(solverWatcher, &QFutureWatcher<QPair<Move, qint64>>::finished, this, &OthelloWidget::onSolverFinished);
        solverWatcher->setFuture(fut);
    }
}

void OthelloWidget::onSolverFinished() {
    int gen = solverWatcher->property("gen").toInt();
    if (gen != solverGen) return;

    QPair<Move, qint64> result = solverWatcher->result();
    Move mv = result.first;
    qint64 ns = result.second;

    SolverType type = SolverType::None;
    if      (gameMode == SERIE)     type = SolverType::Serie;
    else if (gameMode == PARALLELE) type = SolverType::Parallele;

    actionHistory.append({ ActionType::SolverMove, mv, 0, type, ns });
    redoBuffer.clear();
    passPending = false;
    replayHistory();
    updateStatsLog();
    currentTurn = playerColor;
    update();
    checkPass();
}

void OthelloWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QFontMetrics fm = painter.fontMetrics();

    int menuH   = menuBar()->height();
    int boardSize = BOARD_SIZE * cellSize;
    int left      = margin;

    int yOffset   = pointsVisible ? cellSize : 0;
    int top       = menuH + yOffset + margin;

    QRect boardRect(left, top, boardSize + 2*padding, boardSize + 2*padding);

    painter.setBrush(QColor(34,139,34));
    painter.setPen(Qt::NoPen);
    painter.drawRect(boardRect);

    painter.setBrush(QColor(34,139,34,120));
    painter.drawRect(boardRect);

    QLinearGradient borderGrad(boardRect.topLeft(), boardRect.bottomRight());
    borderGrad.setColorAt(0, QColor(80,160,80,150));
    borderGrad.setColorAt(1, QColor(0,60,0,200));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(borderGrad, 8));
    painter.drawRect(boardRect);

    int x0 = left + padding;
    int y0 = top + padding;
    painter.setPen(QPen(QColor(10,60,10),2));
    for(int i=0;i<=BOARD_SIZE;i++) {
        painter.drawLine(x0, y0 + i*cellSize, x0 + boardSize, y0 + i*cellSize);
        painter.drawLine(x0 + i*cellSize, y0, x0 + i*cellSize, y0 + boardSize);
    }

    painter.setBrush(QColor(200,180,60));
    painter.setPen(Qt::NoPen);
    constexpr int dr = 5;
    int bc = cellSize*2, fg = cellSize*6;
    QPoint dots[4] = {{x0+bc, y0+bc},{x0+fg, y0+bc},{x0+bc, y0+fg},{x0+fg, y0+fg}};
    for(auto& pt : dots) painter.drawEllipse(pt, dr, dr);

    painter.setPen(QColor(240,240,240));
    for(int c=0;c<BOARD_SIZE;c++) {
        QString lbl = QChar('A'+c);
        int lx = x0 + c*cellSize + cellSize/2 - fm.horizontalAdvance(lbl)/2;
        int ly = top + padding/2 + fm.ascent()/2;
        painter.drawText(lx, ly, lbl);
    }

    for(int r=0;r<BOARD_SIZE;r++) {
        QString lbl = QString::number(r+1);
        int lx = left + padding/2 - fm.horizontalAdvance(lbl)/2;
        int ly = y0 + r*cellSize + cellSize/2 + fm.ascent()/2;
        painter.drawText(lx, ly, lbl);
    }

    if (currentTurn == playerColor && !board.isGameOver()) {
        auto moves = board.getValidMoves(playerColor);
        painter.setBrush(QColor(255, 255, 255, 120));
        painter.setPen(Qt::NoPen);
        int dotRadius = cellSize / 8;
        for (auto& mv : moves) {
            int cx = x0 + mv.col * cellSize + cellSize/2;
            int cy = y0 + mv.row * cellSize + cellSize/2;
            painter.drawEllipse(QPoint(cx, cy), dotRadius, dotRadius);
        }
    }

    for(int r=0;r<BOARD_SIZE;r++) {
        for(int c=0;c<BOARD_SIZE;c++) {
            int cell = board.cellAt(r,c);
            if(cell==BLACK||cell==WHITE) {
                QRect pr(x0+c*cellSize+2, y0+r*cellSize+2, cellSize-4, cellSize-4);
                QRadialGradient gr(pr.center()-QPoint(cellSize/10,cellSize/10), cellSize/2);
                if(cell==BLACK) {
                    gr.setColorAt(0, QColor(80,80,80));
                    gr.setColorAt(1, QColor(20,20,20));
                } else {
                    gr.setColorAt(0, QColor(245,245,255));
                    gr.setColorAt(1, QColor(180,180,200));
                }
                painter.setBrush(gr);
                painter.setPen(QPen(Qt::black,1));
                painter.drawEllipse(pr);
            }
        }
    }

    if (!actionHistory.isEmpty()) {
        for (int idx = actionHistory.size() - 1; idx >= 0; --idx) {
            const auto& act = actionHistory.at(idx);
            if (act.type == ActionType::UserMove || act.type == ActionType::SolverMove) {
                int lr = act.move.row;
                int lc = act.move.col;
                int cx = x0 + lc * cellSize + cellSize/2;
                int cy = y0 + lr * cellSize + cellSize/2;
                int dotRadius = cellSize / 8;

                painter.setBrush(QColor(200, 180, 60, 180));  
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(QPoint(cx, cy), dotRadius, dotRadius);
                break;
            }
        }
    }

    if (passPending && currentTurn == solverColor) {
        painter.setPen(QPen(Qt::red));
        QFont f = painter.font();
        f.setPointSize(cellSize / 2);
        f.setBold(true);
        painter.setFont(f);
        QString msg = "You Pass";
        if (passStreak > 1)
            msg += QString(" x%1").arg(passStreak);
        painter.drawText(boardRect, Qt::AlignCenter, msg);
    } else if (passPending && currentTurn == playerColor) {
        painter.setPen(QPen(Qt::red));
        QFont f = painter.font();
        f.setPointSize(cellSize / 2);
        f.setBold(true);
        painter.setFont(f);
        QString msg = "Solver Pass";
        if (passStreak > 1)
            msg += QString(" x%1").arg(passStreak);
        painter.drawText(boardRect, Qt::AlignCenter, msg);
    }

    if (board.isGameOver()) {
        int blackCount = board.count(BLACK);
        int whiteCount = board.count(WHITE);
        bool win = (playerColor == BLACK ? blackCount > whiteCount : whiteCount > blackCount);
        QColor msgColor = win ? QColor(0,200,0) : QColor(200,0,0);
        painter.setPen(QPen(msgColor));
        QFont f = painter.font();
        f.setPointSize(cellSize);
        f.setBold(true);
        painter.setFont(f);
        painter.drawText(boardRect, Qt::AlignCenter, win ? "You win" : "You Lost");
    }
}

void OthelloWidget::mousePressEvent(QMouseEvent *event) {
    if (solverWatcher->isRunning()) return;
    if (currentTurn != playerColor) return;

    if (board.isGameOver()) return;

    QPointF pos     = event->position();
    int     menuH   = menuBar()->height();
    int     yOffset = pointsVisible ? cellSize : 0;
    int     top     = menuH  + yOffset + margin;
    int     x0      = margin + padding;
    int     y0      = top    + padding;

    int row = int((pos.y() - y0) / cellSize);
    int col = int((pos.x() - x0) / cellSize);
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;

    if (board.isValidMove(row, col, currentTurn)) {
        actionHistory.append({ ActionType::UserMove, {row, col}, 0 });
        redoBuffer.clear();
        passPending = false;
        replayHistory();
        updateStatsLog();
        updatePoints();
        update();
        if (board.hasAnyValidMove(solverColor))
            QTimer::singleShot(200, this, &OthelloWidget::makeSolverMove);
        else
            checkPass();
    }
}

void OthelloWidget::onStatsVisibilityChanged(bool visible) {
    statsOn = visible;
    recalcWindowSize();
}

void OthelloWidget::resizeEvent(QResizeEvent* event) {
    if (!resizingProgrammatically) {
        resize(lastProgrammaticSize);
        return;
    }
    resizingProgrammatically = false;
    QMainWindow::resizeEvent(event);
}

void OthelloWidget::performResize(int w, int h) {
    resizingProgrammatically = true;
    lastProgrammaticSize = QSize(w, h);
    setFixedSize(w, h);
}

void OthelloWidget::replayHistory() {
    board = Board();
    currentTurn = BLACK;
    int solverCount = 0;
    for (auto &act : actionHistory) {
        switch (act.type) {
          case ActionType::UserMove:
            board.applyMove(act.move, playerColor);
            currentTurn = solverColor;
            break;
          case ActionType::SolverMove:
            solverCount++;
            act.solverTurnIndex = solverCount;
            board.applyMove(act.move, solverColor);
            currentTurn = playerColor;
            break;
          case ActionType::UserPass:
            passPending = true;
            currentTurn = solverColor;
            break;
          case ActionType::SolverPass:
            passPending = true;
            currentTurn = playerColor;
            break;
          default: break;
        }
    }
    updatePoints();
}

void OthelloWidget::updateStatsLog() {
    statsLog->clear();
    int userCount = 0, serieCount = 0, paralleleCount = 0, passCount = 0;
    int timeUnitIdx = timeUnitCombo ? timeUnitCombo->currentIndex() : 1;
    double timeFactor[] = { 1e-9, 1e-6, 1e-3, 1.0 }; // s, ms, us, ns
    QString unitStr[] = { "s", "ms", "μs", "ns" };
    bool useBest = (timeUnitIdx == 0);

    qint64 lastSerieNs = 0, lastParalleleNs = 0;

    QString html = R"(
        <body style='margin:0;padding:0;width:100%;'>
        <div style='width:100%;'>
            <style>
                table.stats-table {
                    border-collapse: collapse; width: 100%;
                    font-size: 13px;
                }
                table.stats-table th, table.stats-table td {
                    border-bottom: 1px solid #5e9772;
                    padding: 6px 10px;
                    text-align: center;
                    width: auto; /* Make sure cells flex */
                }
                table.stats-table th {
                    background: #227a41;
                    color: #fff;
                    font-weight: bold;
                }
                table.stats-table tr.user-row td {
                    background: #c3eaff;
                    color: #035fa7;
                }
                table.stats-table tr.serie-row td {
                    background: #bbf3de;
                    color: #106245;
                }
                table.stats-table tr.parallel-row td {
                    background: #fff7b3;
                    color: #b28500;
                }
                table.stats-table tr.pass-row td {
                    background: #ffccd5;
                    color: #b71c1c;
                    font-weight: bold;
                }
                table.stats-table tr.default-row td {
                    background: #e7ede9;
                    color: #227a41;
                }
            </style>
            <table class='stats-table'>
                <tr>
                    <th>#</th>
                    <th>Type</th>
                    <th>Move</th>
                    <th>Time</th>
                    <th>Speedup</th>
                </tr>
    )";

    int moveNum = 1;
    for (int i = 0; i < actionHistory.size(); ++i) {
        const auto& act = actionHistory[i];
        bool validMove = (act.move.row >= 0 && act.move.col >= 0 && act.move.row < BOARD_SIZE && act.move.col < BOARD_SIZE);
        QChar colLabel = validMove ? QChar('A' + act.move.col) : '?';
        int rowLabel = validMove ? (act.move.row + 1) : 0;

        QString timeStr, speedupStr, rowClass, typeStr;

        switch (act.type) {
        case ActionType::UserMove:
            typeStr = "<b style='color:#1565c0;'>You</b>";
            rowClass = "user-row";
            timeStr = "-";
            speedupStr = "-";
            html += QString("<tr class='%1'><td>%2</td><td>%3</td><td><b>%4%5</b></td><td>%6</td><td>%7</td></tr>")
                .arg(rowClass).arg(moveNum++).arg(typeStr).arg(colLabel).arg(rowLabel).arg(timeStr).arg(speedupStr);
            break;
        case ActionType::SolverMove:
            if (act.solverType == SolverType::Serie) {
                typeStr = "<b style='color:#00695c;'>Solver (Serie)</b>";
                rowClass = "serie-row";
                lastSerieNs = act.elapsedNs;
                if (useBest) {
                    qint64 ms = act.elapsedNs / 1000000;
                    timeStr = act.elapsedNs > 0 ? formatBestTime(ms, act.elapsedNs) : "-";
                } else {
                    timeStr = act.elapsedNs > 0 ?
                        QString("%1 %2")
                            .arg(act.elapsedNs * timeFactor[timeUnitIdx-1], 0, 'f', 3)
                            .arg(unitStr[timeUnitIdx-1]) : "-";
                }
                speedupStr = "-";
                html += QString("<tr class='%1'><td>%2</td><td>%3</td><td><b>%4%5</b></td><td>%6</td><td>%7</td></tr>")
                    .arg(rowClass).arg(moveNum++).arg(typeStr).arg(colLabel).arg(rowLabel).arg(timeStr).arg(speedupStr);
            } else if (act.solverType == SolverType::Parallele) {
                typeStr = "<b style='color:#f9a825;'>Solver (Parallele)</b>";
                rowClass = "parallel-row";
                lastParalleleNs = act.elapsedNs;
                if (useBest) {
                    qint64 ms = act.elapsedNs / 1000000;
                    timeStr = act.elapsedNs > 0 ? formatBestTime(ms, act.elapsedNs) : "-";
                } else {
                    timeStr = act.elapsedNs > 0 ?
                        QString("%1 %2")
                            .arg(act.elapsedNs * timeFactor[timeUnitIdx-1], 0, 'f', 3)
                            .arg(unitStr[timeUnitIdx-1]) : "-";
                }
                double speedup = (lastSerieNs > 0 && lastParalleleNs > 0) ?
                        (double)lastSerieNs / lastParalleleNs : 0;
                speedupStr = (speedup > 0.01)
                    ? QString("<span style='color:#e91e63;font-weight:bold;'>×%1</span>").arg(speedup, 0, 'f', 2)
                    : "-";
                html += QString("<tr class='%1'><td>%2</td><td>%3</td><td><b>%4%5</b></td><td>%6</td><td>%7</td></tr>")
                    .arg(rowClass).arg(moveNum++).arg(typeStr).arg(colLabel).arg(rowLabel).arg(timeStr).arg(speedupStr);
            } else {
                typeStr = "<b style='color:#757575;'>Solver</b>";
                rowClass = "default-row";
                timeStr = "-";
                speedupStr = "-";
                html += QString("<tr><td>%1</td><td>%2</td><td><b>%3%4</b></td><td>%5</td><td>%6</td></tr>")
                    .arg(moveNum++).arg(typeStr).arg(colLabel).arg(rowLabel).arg(timeStr).arg(speedupStr);
            }
            break;
        case ActionType::UserPass:
            typeStr = "<b>Forced pass</b>";
            rowClass = "pass-row";
            html += QString("<tr class='%1'><td>%2</td><td colspan='4'>No valid move for player</td></tr>")
                .arg(rowClass).arg(moveNum++);
            break;
        case ActionType::SolverPass:
            typeStr = "<b>Solver pass</b>";
            rowClass = "pass-row";
            html += QString("<tr class='%1'><td>%2</td><td colspan='4'>No valid move for solver</td></tr>")
                .arg(rowClass).arg(moveNum++);
            break;
        default:
            break;
        }
    }

    html += "</table><hr>";

    if (board.isGameOver()) {
        int blackCount = board.count(BLACK);
        int whiteCount = board.count(WHITE);
        html += "<div style='padding:8px 0;'>";
        html += QString("<b>Game Over!</b> Black: <b>%1</b> &nbsp; White: <b>%2</b><br>").arg(blackCount).arg(whiteCount);
        if      (blackCount > whiteCount) html += "<span style='color:#00695c;'><b>Black wins!</b></span><br>";
        else if (whiteCount > blackCount) html += "<span style='color:#1565c0;'><b>White wins!</b></span><br>";
        else                             html += "<span style='color:#777;'><b>Draw!</b></span><br>";
        html += "</div>";
    }

    statsLog->setHtml(html);
}

void OthelloWidget::exportGame() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Game"),
        QDir::currentPath(),
        tr("Othello Save Files (*.othello)")
    );
    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(".othello", Qt::CaseInsensitive)) fileName += ".othello";

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
            this,
            tr("Export Game"),
            tr("Could not save file:\n%1").arg(file.errorString())
        );
        return;
    }
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);
    quint32 n = actionHistory.size();
    out << n;
    for (const auto &act : actionHistory) {
        out << quint8(act.type);
        out << qint32(act.move.row) << qint32(act.move.col);
        out << qint32(act.solverTurnIndex);
    }
}

void OthelloWidget::importGame() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Import Game"),
        QDir::currentPath(),
        tr("Othello Save Files (*.othello)")
    );
    if (fileName.isEmpty())
        return;

    auto future = QtConcurrent::run([fileName]() -> QVector<Action> {
        QVector<Action> hist;
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly)) return hist;
        QDataStream in(&f);
        in.setVersion(QDataStream::Qt_6_0);
        quint32 count; in >> count;
        hist.reserve(count);
        for (quint32 i = 0; i < count; ++i) {
            quint8 t; qint32 r,c,s;
            in >> t >> r >> c >> s;
            Action a; a.type = ActionType(t);
            a.move.row = r; a.move.col = c;
            a.solverTurnIndex = s;
            hist.append(a);
        }
        return hist;
    });
    importWatcher->setFuture(future);
}

void OthelloWidget::onImportFinished() {
    actionHistory = importWatcher->result();
    passPending = false;
    redoBuffer.clear();
    replayHistory();
    updateStatsLog();
    update();
}

void OthelloWidget::checkPass() {
    if (currentTurn == playerColor
        && !board.hasAnyValidMove(playerColor)
        && board.hasAnyValidMove(solverColor))
    {
        actionHistory.append({ ActionType::UserPass, {-1,-1}, 0 });
        redoBuffer.clear();
        passStreak++;
        passPending = true;
        replayHistory();
        updateStatsLog();
        update();

        QTimer::singleShot(1000, this, [this]() {
            passPending = false;
            update();
        });

        QTimer::singleShot(150, this, &OthelloWidget::makeSolverMove);
    } else if (currentTurn == solverColor
        && !board.hasAnyValidMove(solverColor)
        && board.hasAnyValidMove(playerColor))
    {
        actionHistory.append({ ActionType::SolverPass, {-1,-1}, 0 });
        redoBuffer.clear();
        passStreak++;
        passPending = true;
        replayHistory();
        updateStatsLog();
        update();

        QTimer::singleShot(1000, this, [this]() {
            passPending = false;
            update();
        });

        currentTurn = playerColor;
        update();
        checkPass();
    } else {
        QTimer::singleShot(1000, this, [this]() {
            passPending = false;
            update();
        });
    }
}

void OthelloWidget::showPoints() {
    int blackCount = board.count(BLACK);
    int whiteCount = board.count(WHITE);

    QMessageBox::information(
        this,
        "Points",
        QString(" Black: %1\n White: %2").arg(blackCount).arg(whiteCount)
    );
}

void OthelloWidget::updatePoints() {
    int b = board.count(BLACK);
    int w = board.count(WHITE);
    pointsWidget->setCounts(b, w);
}

void OthelloWidget::togglePoints() {
    pointsVisible = !pointsVisible;
    pointsDock->setVisible(pointsVisible);
    recalcWindowSize();
}

void OthelloWidget::recalcWindowSize() {
    int w = baseWindowSize.width()  + (statsOn       ? statsPaneWidth : 0);
    int h = baseWindowSize.height() + (pointsVisible ? cellSize       : 0);
    performResize(w, h);
}

void OthelloWidget::updateWindowTitle() {
    QString modeStr;
    switch (gameMode) {
      case SERIE:
        modeStr = "Serie";
        break;
      case PARALLELE:
        modeStr = "Parallele";
        break;
      case COMPARE:
        modeStr = "Compare";
        break;
      default:
        modeStr = "[ERROR]";
        break;
    }

    QString ttStr = ttEnabled ? "true" : "false";
    int depth  = solver.getDepth();
    int timeMs = solver.getTime();
    QString title = QString("Othello – %1 – TT: %2, (%3, %4)").arg(modeStr).arg(ttStr).arg(depth).arg(timeMs);
    setWindowTitle(title);
}