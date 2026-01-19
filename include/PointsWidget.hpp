// include/PointsWidget.hpp

#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QPen>
#include <QColor>

class OthelloWidget;

/**
 * @brief Widget graphique affichant le score des joueurs (pions noirs et blancs).
 *
 * Ce widget est conçu pour être affiché dans un panneau de l'interface principale (`OthelloWidget`).
 * Il dessine deux indicateurs (souvent sous forme de cercles colorés) représentant le nombre
 * de pions noirs et blancs sur le plateau.
 */
class PointsWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief        Le constructeur de PointsWidget.
     * @param parent Widget parent (optionnel).
     */
    explicit PointsWidget(QWidget *parent = nullptr);

    /**
     * Cette méthode met à jour les compteurs internes et déclenche un repaint du widget.
     * 
     * @brief            Définit les nombres de pions noirs et blancs à afficher.
     * @param blackCount Nombre de pions noirs.
     * @param whiteCount Nombre de pions blancs
     */
    void setCounts(int blackCount, int whiteCount);

protected:
    /**
     * Cette méthode dessine les deux compteurs de pions.
     *
     * @brief       Événement de peinture du widget.
     * @param event Événement Qt contenant la zone à redessiner.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    int blackCount;
    int whiteCount;
};
