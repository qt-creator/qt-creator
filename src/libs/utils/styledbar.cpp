#include "styledbar.h"

#include "stylehelper.h"

#include <QtCore/QVariant>
#include <QtGui/QPainter>
#include <QtGui/QPixmapCache>

using namespace Core::Utils;

StyledBar::StyledBar(QWidget *parent)
    : QWidget(parent)
{
    setProperty("panelwidget", true);
}

void StyledBar::paintEvent(QPaintEvent *event)
{
    // Currently from the style
    // Goal should be to migrate that into a Utils::StyledWidget class
    Q_UNUSED(event)
    QPainter painter(this);

    QRect selfRect = rect();
    QString key;
    key.sprintf("mh_toolbar %d %d %d", selfRect.width(), selfRect.height(), StyleHelper::baseColor().rgb());;

    QPixmap pixmap;
    QPainter *p = &painter;
    if (StyleHelper::usePixmapCache() && !QPixmapCache::find(key, pixmap)) {
        pixmap = QPixmap(selfRect.size());
        p = new QPainter(&pixmap);
        selfRect = QRect(0, 0, selfRect.width(), selfRect.height());
    }

    // Map offset for global window gradient
    QPoint offset = window()->mapToGlobal(selfRect.topLeft()) -
                                                  mapToGlobal(selfRect.topLeft());
    QRect gradientSpan;
    gradientSpan = QRect(offset, window()->size());
    StyleHelper::horizontalGradient(p, gradientSpan, selfRect);

    p->setPen(StyleHelper::borderColor());

    // Note: This is a hack to determine if the
    // toolbar should draw the top or bottom outline
    // (needed for the find toolbar for instance)
    QColor lighter(255, 255, 255, 40);
    if (property("topBorder").toBool()) {
        p->drawLine(selfRect.topLeft(), selfRect.topRight());
        p->setPen(lighter);
        p->drawLine(selfRect.topLeft() + QPoint(0, 1), selfRect.topRight() + QPoint(0, 1));
    } else {
        p->drawLine(selfRect.bottomLeft(), selfRect.bottomRight());
        p->setPen(lighter);
        p->drawLine(selfRect.topLeft(), selfRect.topRight());
    }

    if (StyleHelper::usePixmapCache() && !QPixmapCache::find(key, pixmap)) {
        painter.drawPixmap(selfRect.topLeft(), pixmap);
        p->end();
        delete p;
        QPixmapCache::insert(key, pixmap);
    }
}
