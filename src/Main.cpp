// src/Main.cpp

#include "../include/OthelloWidget.hpp"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication  app(argc, argv);
    OthelloWidget w;

    const int cellSize       = 60;
    const int gridSize       = BOARD_SIZE * cellSize;
    const int padding        = 20;
    const int margin         = 20;
    const int totalBoardSize = gridSize + 2 * padding;

    int menuHeight   = w.menuBar()->sizeHint().height();
    int windowWidth  = totalBoardSize + 2 * margin;
    int windowHeight = menuHeight + totalBoardSize + 2 * margin;

    w.resize(windowWidth, windowHeight);
    w.show();

    return app.exec();
}
