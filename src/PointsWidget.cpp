// include/PointsWidget.hpp

#include "../include/PointsWidget.hpp"
#include "../include/OthelloWidget.hpp"

#include <QPainter>
#include <QRadialGradient>
#include <QPen>
#include <QColor>
#include <QString>

PointsWidget::PointsWidget(QWidget *parent) : QWidget(parent), blackCount(0), whiteCount(0) {}

void PointsWidget::setCounts(int b, int w) {
    blackCount = b;
    whiteCount = w;
    update();
}

void PointsWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto* ow   = qobject_cast<OthelloWidget*>(parent());
    int   disc = ow ? ow->getCellSize() - 4 : height()*2/3;
    int   y    = (height() - disc) / 2;
    QFontMetrics fm(p.font());

    QRect blackRect(10, y, disc, disc);
    QRadialGradient blackGrad(
        blackRect.center() - QPoint(disc/10, disc/10),
        disc/2
    );
    blackGrad.setColorAt(0, QColor(80,80,80));
    blackGrad.setColorAt(1, QColor(20,20,20));
    p.setBrush(blackGrad);
    p.setPen(QPen(Qt::black, 1));
    p.drawEllipse(blackRect);

    QString bTxt = QString::number(blackCount);
    int     bx   = blackRect.right() + 5;
    int     by   = blackRect.center().y() + fm.ascent()/2;
    p.drawText(bx, by, bTxt);

    QRect whiteRect(width() - 10 - disc, y, disc, disc);
    QRadialGradient whiteGrad(
        whiteRect.center() - QPoint(disc/10, disc/10),
        disc/2
    );

    whiteGrad.setColorAt(0, QColor(245,245,255));
    whiteGrad.setColorAt(1, QColor(180,180,200));
    p.setBrush(whiteGrad);
    p.setPen(QPen(Qt::black, 1));
    p.drawEllipse(whiteRect);

    QString wTxt = QString::number(whiteCount);
    int     wx   = whiteRect.left() - fm.horizontalAdvance(wTxt) - 5;
    int     wy   = whiteRect.center().y() + fm.ascent()/2;
    p.drawText(wx, wy, wTxt);
}
