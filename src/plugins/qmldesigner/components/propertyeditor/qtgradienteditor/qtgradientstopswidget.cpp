/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtgradientstopswidget.h"
#include "qtgradientstopsmodel.h"

#include <QtCore/QMap>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QMouseEvent>
#include <QtGui/QRubberBand>
#include <QtGui/QMenu>

QT_BEGIN_NAMESPACE

class QtGradientStopsWidgetPrivate
{
    QtGradientStopsWidget *q_ptr;
    Q_DECLARE_PUBLIC(QtGradientStopsWidget)
public:
    typedef QMap<qreal, QColor> PositionColorMap;
    typedef QMap<QtGradientStop *, qreal> StopPositionMap;

    void slotStopAdded(QtGradientStop *stop);
    void slotStopRemoved(QtGradientStop *stop);
    void slotStopMoved(QtGradientStop *stop, qreal newPos);
    void slotStopsSwapped(QtGradientStop *stop1, QtGradientStop *stop2);
    void slotStopChanged(QtGradientStop *stop, const QColor &newColor);
    void slotStopSelected(QtGradientStop *stop, bool selected);
    void slotCurrentStopChanged(QtGradientStop *stop);
    void slotNewStop();
    void slotDelete();
    void slotFlipAll();
    void slotSelectAll();
    void slotZoomIn();
    void slotZoomOut();
    void slotResetZoom();

    double fromViewport(int x) const;
    double toViewport(double x) const;
    QtGradientStop *stopAt(const QPoint &viewportPos) const;
    QList<QtGradientStop *> stopsAt(const QPoint &viewportPos) const;
    void setupMove(QtGradientStop *stop, int x);
    void ensureVisible(double x); // x = stop position
    void ensureVisible(QtGradientStop *stop);
    QtGradientStop *newStop(const QPoint &viewportPos);

    bool m_backgroundCheckered;
    QtGradientStopsModel *m_model;
    double m_handleSize;
    int m_scaleFactor;
    double m_zoom;

#ifndef QT_NO_DRAGANDDROP
    QtGradientStop *m_dragStop;
    QtGradientStop *m_changedStop;
    QtGradientStop *m_clonedStop;
    QtGradientStopsModel *m_dragModel;
    QColor m_dragColor;
    void clearDrag();
    void removeClonedStop();
    void restoreChangedStop();
    void changeStop(qreal pos);
    void cloneStop(qreal pos);
#endif

    QRubberBand *m_rubber;
    QPoint m_clickPos;

    QList<QtGradientStop *> m_stops;

    bool m_moving;
    int m_moveOffset;
    StopPositionMap m_moveStops;

    PositionColorMap m_moveOriginal;
};

double QtGradientStopsWidgetPrivate::fromViewport(int x) const
{
    QSize size = q_ptr->viewport()->size();
    int w = size.width();
    int max = q_ptr->horizontalScrollBar()->maximum();
    int val = q_ptr->horizontalScrollBar()->value();
    return ((double)x * m_scaleFactor + w * val) / (w * (m_scaleFactor + max));
}

double QtGradientStopsWidgetPrivate::toViewport(double x) const
{
    QSize size = q_ptr->viewport()->size();
    int w = size.width();
    int max = q_ptr->horizontalScrollBar()->maximum();
    int val = q_ptr->horizontalScrollBar()->value();
    return w * (x * (m_scaleFactor + max) - val) / m_scaleFactor;
}

QtGradientStop *QtGradientStopsWidgetPrivate::stopAt(const QPoint &viewportPos) const
{
    double posY = m_handleSize / 2;
    QListIterator<QtGradientStop *> itStop(m_stops);
    while (itStop.hasNext()) {
        QtGradientStop *stop = itStop.next();

        double posX = toViewport(stop->position());

        double x = viewportPos.x() - posX;
        double y = viewportPos.y() - posY;

        if ((m_handleSize * m_handleSize / 4) > (x * x + y * y))
            return stop;
    }
    return 0;
}

QList<QtGradientStop *> QtGradientStopsWidgetPrivate::stopsAt(const QPoint &viewportPos) const
{
    QList<QtGradientStop *> stops;
    double posY = m_handleSize / 2;
    QListIterator<QtGradientStop *> itStop(m_stops);
    while (itStop.hasNext()) {
        QtGradientStop *stop = itStop.next();

        double posX = toViewport(stop->position());

        double x = viewportPos.x() - posX;
        double y = viewportPos.y() - posY;

        if ((m_handleSize * m_handleSize / 4) > (x * x + y * y))
            stops.append(stop);
    }
    return stops;
}

void QtGradientStopsWidgetPrivate::setupMove(QtGradientStop *stop, int x)
{
    m_model->setCurrentStop(stop);

    int viewportX = qRound(toViewport(stop->position()));
    m_moveOffset = x - viewportX;

    QList<QtGradientStop *> stops = m_stops;
    m_stops.clear();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (m_model->isSelected(s) || s == stop) {
            m_moveStops[s] = s->position() - stop->position();
            m_stops.append(s);
        } else {
            m_moveOriginal[s->position()] = s->color();
        }
    }
    itStop.toFront();
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (!m_model->isSelected(s))
            m_stops.append(s);
    }
    m_stops.removeAll(stop);
    m_stops.prepend(stop);
}

void QtGradientStopsWidgetPrivate::ensureVisible(double x)
{
    double viewX = toViewport(x);
    if (viewX < 0 || viewX > q_ptr->viewport()->size().width()) {
        int max = q_ptr->horizontalScrollBar()->maximum();
        int newVal = qRound(x * (max + m_scaleFactor) - m_scaleFactor / 2);
        q_ptr->horizontalScrollBar()->setValue(newVal);
    }
}

void QtGradientStopsWidgetPrivate::ensureVisible(QtGradientStop *stop)
{
    if (!stop)
        return;
    ensureVisible(stop->position());
}

QtGradientStop *QtGradientStopsWidgetPrivate::newStop(const QPoint &viewportPos)
{
    QtGradientStop *copyStop = stopAt(viewportPos);
    double posX = fromViewport(viewportPos.x());
    QtGradientStop *stop = m_model->at(posX);
    if (!stop) {
        QColor newColor;
        if (copyStop)
            newColor = copyStop->color();
        else
            newColor = m_model->color(posX);
        if (!newColor.isValid())
            newColor = Qt::white;
        stop = m_model->addStop(posX, newColor);
    }
    return stop;
}

void QtGradientStopsWidgetPrivate::slotStopAdded(QtGradientStop *stop)
{
    m_stops.append(stop);
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopRemoved(QtGradientStop *stop)
{
    m_stops.removeAll(stop);
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopMoved(QtGradientStop *stop, qreal newPos)
{
    Q_UNUSED(stop)
    Q_UNUSED(newPos)
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopsSwapped(QtGradientStop *stop1, QtGradientStop *stop2)
{
    Q_UNUSED(stop1)
    Q_UNUSED(stop2)
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopChanged(QtGradientStop *stop, const QColor &newColor)
{
    Q_UNUSED(stop)
    Q_UNUSED(newColor)
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopSelected(QtGradientStop *stop, bool selected)
{
    Q_UNUSED(stop)
    Q_UNUSED(selected)
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotCurrentStopChanged(QtGradientStop *stop)
{
    Q_UNUSED(stop)

    if (!m_model)
        return;
    q_ptr->viewport()->update();
    if (stop) {
        m_stops.removeAll(stop);
        m_stops.prepend(stop);
    }
}

void QtGradientStopsWidgetPrivate::slotNewStop()
{
    if (!m_model)
        return;

    QtGradientStop *stop = newStop(m_clickPos);

    if (!stop)
        return;

    m_model->clearSelection();
    m_model->selectStop(stop, true);
    m_model->setCurrentStop(stop);
}

void QtGradientStopsWidgetPrivate::slotDelete()
{
    if (!m_model)
        return;

    m_model->deleteStops();
}

void QtGradientStopsWidgetPrivate::slotFlipAll()
{
    if (!m_model)
        return;

    m_model->flipAll();
}

void QtGradientStopsWidgetPrivate::slotSelectAll()
{
    if (!m_model)
        return;

    m_model->selectAll();
}

void QtGradientStopsWidgetPrivate::slotZoomIn()
{
    double newZoom = q_ptr->zoom() * 2;
    if (newZoom > 100)
        newZoom = 100;
    if (newZoom == q_ptr->zoom())
        return;

    q_ptr->setZoom(newZoom);
    emit q_ptr->zoomChanged(q_ptr->zoom());
}

void QtGradientStopsWidgetPrivate::slotZoomOut()
{
    double newZoom = q_ptr->zoom() / 2;
    if (newZoom < 1)
        newZoom = 1;
    if (newZoom == q_ptr->zoom())
        return;

    q_ptr->setZoom(newZoom);
    emit q_ptr->zoomChanged(q_ptr->zoom());
}

void QtGradientStopsWidgetPrivate::slotResetZoom()
{
    if (1 == q_ptr->zoom())
        return;

    q_ptr->setZoom(1);
    emit q_ptr->zoomChanged(1);
}

QtGradientStopsWidget::QtGradientStopsWidget(QWidget *parent)
    : QAbstractScrollArea(parent), d_ptr(new QtGradientStopsWidgetPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->m_backgroundCheckered = true;
    d_ptr->m_model = 0;
    d_ptr->m_handleSize = 25.0;
    d_ptr->m_scaleFactor = 1000;
    d_ptr->m_moving = false;
    d_ptr->m_zoom = 1;
    d_ptr->m_rubber = new QRubberBand(QRubberBand::Rectangle, this);
#ifndef QT_NO_DRAGANDDROP
    d_ptr->m_dragStop = 0;
    d_ptr->m_changedStop = 0;
    d_ptr->m_clonedStop = 0;
    d_ptr->m_dragModel = 0;
#endif
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    horizontalScrollBar()->setRange(0, (int)(d_ptr->m_scaleFactor * (d_ptr->m_zoom - 1) + 0.5));
    horizontalScrollBar()->setPageStep(d_ptr->m_scaleFactor);
    horizontalScrollBar()->setSingleStep(4);
    viewport()->setAutoFillBackground(false);

    setAcceptDrops(true);

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
}

QtGradientStopsWidget::~QtGradientStopsWidget()
{
}

QSize QtGradientStopsWidget::sizeHint() const
{
    return QSize(qRound(2 * d_ptr->m_handleSize), qRound(3 * d_ptr->m_handleSize) + horizontalScrollBar()->sizeHint().height());
}

QSize QtGradientStopsWidget::minimumSizeHint() const
{
    return QSize(qRound(2 * d_ptr->m_handleSize), qRound(3 * d_ptr->m_handleSize) + horizontalScrollBar()->minimumSizeHint().height());
}

void QtGradientStopsWidget::setBackgroundCheckered(bool checkered)
{
    if (d_ptr->m_backgroundCheckered == checkered)
        return;
    d_ptr->m_backgroundCheckered = checkered;
    update();
}

bool QtGradientStopsWidget::isBackgroundCheckered() const
{
    return d_ptr->m_backgroundCheckered;
}

void QtGradientStopsWidget::setGradientStopsModel(QtGradientStopsModel *model)
{
    if (d_ptr->m_model == model)
        return;

    if (d_ptr->m_model) {
        disconnect(d_ptr->m_model, SIGNAL(stopAdded(QtGradientStop*)),
                    this, SLOT(slotStopAdded(QtGradientStop*)));
        disconnect(d_ptr->m_model, SIGNAL(stopRemoved(QtGradientStop*)),
                    this, SLOT(slotStopRemoved(QtGradientStop*)));
        disconnect(d_ptr->m_model, SIGNAL(stopMoved(QtGradientStop*,qreal)),
                    this, SLOT(slotStopMoved(QtGradientStop*,qreal)));
        disconnect(d_ptr->m_model, SIGNAL(stopsSwapped(QtGradientStop*,QtGradientStop*)),
                    this, SLOT(slotStopsSwapped(QtGradientStop*,QtGradientStop*)));
        disconnect(d_ptr->m_model, SIGNAL(stopChanged(QtGradientStop*,QColor)),
                    this, SLOT(slotStopChanged(QtGradientStop*,QColor)));
        disconnect(d_ptr->m_model, SIGNAL(stopSelected(QtGradientStop*,bool)),
                    this, SLOT(slotStopSelected(QtGradientStop*,bool)));
        disconnect(d_ptr->m_model, SIGNAL(currentStopChanged(QtGradientStop*)),
                    this, SLOT(slotCurrentStopChanged(QtGradientStop*)));

        d_ptr->m_stops.clear();
    }

    d_ptr->m_model = model;

    if (d_ptr->m_model) {
        connect(d_ptr->m_model, SIGNAL(stopAdded(QtGradientStop*)),
                    this, SLOT(slotStopAdded(QtGradientStop*)));
        connect(d_ptr->m_model, SIGNAL(stopRemoved(QtGradientStop*)),
                    this, SLOT(slotStopRemoved(QtGradientStop*)));
        connect(d_ptr->m_model, SIGNAL(stopMoved(QtGradientStop*,qreal)),
                    this, SLOT(slotStopMoved(QtGradientStop*,qreal)));
        connect(d_ptr->m_model, SIGNAL(stopsSwapped(QtGradientStop*,QtGradientStop*)),
                    this, SLOT(slotStopsSwapped(QtGradientStop*,QtGradientStop*)));
        connect(d_ptr->m_model, SIGNAL(stopChanged(QtGradientStop*,QColor)),
                    this, SLOT(slotStopChanged(QtGradientStop*,QColor)));
        connect(d_ptr->m_model, SIGNAL(stopSelected(QtGradientStop*,bool)),
                    this, SLOT(slotStopSelected(QtGradientStop*,bool)));
        connect(d_ptr->m_model, SIGNAL(currentStopChanged(QtGradientStop*)),
                    this, SLOT(slotCurrentStopChanged(QtGradientStop*)));

        QList<QtGradientStop *> stops = d_ptr->m_model->stops().values();
        QListIterator<QtGradientStop *> itStop(stops);
        while (itStop.hasNext())
            d_ptr->slotStopAdded(itStop.next());

        QList<QtGradientStop *> selected = d_ptr->m_model->selectedStops();
        QListIterator<QtGradientStop *> itSelect(selected);
        while (itSelect.hasNext())
            d_ptr->slotStopSelected(itSelect.next(), true);

        d_ptr->slotCurrentStopChanged(d_ptr->m_model->currentStop());
    }
}

void QtGradientStopsWidget::mousePressEvent(QMouseEvent *e)
{
    typedef QtGradientStopsModel::PositionStopMap PositionStopMap;
    if (!d_ptr->m_model)
        return;

    if (e->button() != Qt::LeftButton)
        return;

    d_ptr->m_moving = true;

    d_ptr->m_moveStops.clear();
    d_ptr->m_moveOriginal.clear();
    d_ptr->m_clickPos = e->pos();
    QtGradientStop *stop = d_ptr->stopAt(e->pos());
    if (stop) {
        if (e->modifiers() & Qt::ControlModifier) {
            d_ptr->m_model->selectStop(stop, !d_ptr->m_model->isSelected(stop));
        } else if (e->modifiers() & Qt::ShiftModifier) {
            QtGradientStop *oldCurrent = d_ptr->m_model->currentStop();
            if (oldCurrent) {
                PositionStopMap stops = d_ptr->m_model->stops();
                PositionStopMap::ConstIterator itSt = stops.constFind(oldCurrent->position());
                if (itSt != stops.constEnd()) {
                    while (itSt != stops.constFind(stop->position())) {
                        d_ptr->m_model->selectStop(itSt.value(), true);
                        if (oldCurrent->position() < stop->position())
                            ++itSt;
                        else
                            --itSt;
                    }
                }
            }
            d_ptr->m_model->selectStop(stop, true);
        } else {
            if (!d_ptr->m_model->isSelected(stop)) {
                d_ptr->m_model->clearSelection();
                d_ptr->m_model->selectStop(stop, true);
            }
        }
        d_ptr->setupMove(stop, e->pos().x());
    } else {
        d_ptr->m_model->clearSelection();
        d_ptr->m_rubber->setGeometry(QRect(d_ptr->m_clickPos, QSize()));
        d_ptr->m_rubber->show();
    }
    viewport()->update();
}

void QtGradientStopsWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (!d_ptr->m_model)
        return;

    if (e->button() != Qt::LeftButton)
        return;

    d_ptr->m_moving = false;
    d_ptr->m_rubber->hide();
    d_ptr->m_moveStops.clear();
    d_ptr->m_moveOriginal.clear();
}

void QtGradientStopsWidget::mouseMoveEvent(QMouseEvent *e)
{
    typedef QtGradientStopsWidgetPrivate::PositionColorMap PositionColorMap;
    typedef QtGradientStopsModel::PositionStopMap PositionStopMap;
    typedef QtGradientStopsWidgetPrivate::StopPositionMap StopPositionMap;
    if (!d_ptr->m_model)
        return;

    if (!(e->buttons() & Qt::LeftButton))
        return;

    if (!d_ptr->m_moving)
        return;

    if (!d_ptr->m_moveStops.isEmpty()) {
        double maxOffset = 0.0;
        double minOffset = 0.0;
        bool first = true;
        StopPositionMap::ConstIterator itStop = d_ptr->m_moveStops.constBegin();
        while (itStop != d_ptr->m_moveStops.constEnd()) {
            double offset = itStop.value();

            if (first) {
                maxOffset = offset;
                minOffset = offset;
                first = false;
            } else {
                if (maxOffset < offset)
                    maxOffset = offset;
                else if (minOffset > offset)
                    minOffset = offset;
            }
            ++itStop;
        }

        double viewportMin = d_ptr->toViewport(-minOffset);
        double viewportMax = d_ptr->toViewport(1.0 - maxOffset);

        PositionStopMap newPositions;

        int viewportX = e->pos().x() - d_ptr->m_moveOffset;

        if (viewportX > viewport()->size().width())
            viewportX = viewport()->size().width();
        else if (viewportX < 0)
            viewportX = 0;

        double posX = d_ptr->fromViewport(viewportX);

        if (viewportX > viewportMax)
            posX = 1.0 - maxOffset;
        else if (viewportX < viewportMin)
            posX = -minOffset;

        itStop = d_ptr->m_moveStops.constBegin();
        while (itStop != d_ptr->m_moveStops.constEnd()) {
            QtGradientStop *stop = itStop.key();

            newPositions[posX + itStop.value()] = stop;

            ++itStop;
        }

        bool forward = true;
        PositionStopMap::ConstIterator itNewPos = newPositions.constBegin();
        if (itNewPos.value()->position() < itNewPos.key())
            forward = false;

        itNewPos = forward ? newPositions.constBegin() : newPositions.constEnd();
        while (itNewPos != (forward ? newPositions.constEnd() : newPositions.constBegin())) {
            if (!forward)
                --itNewPos;
            QtGradientStop *stop = itNewPos.value();
            double newPos = itNewPos.key();
            if (newPos > 1)
                newPos = 1;
            else if (newPos < 0)
                newPos = 0;

            QtGradientStop *existingStop = d_ptr->m_model->at(newPos);
            if (existingStop && !d_ptr->m_moveStops.contains(existingStop))
                    d_ptr->m_model->removeStop(existingStop);
            d_ptr->m_model->moveStop(stop, newPos);

            if (forward)
                ++itNewPos;
        }

        PositionColorMap::ConstIterator itOld = d_ptr->m_moveOriginal.constBegin();
        while (itOld != d_ptr->m_moveOriginal.constEnd()) {
            double position = itOld.key();
            if (!d_ptr->m_model->at(position))
                d_ptr->m_model->addStop(position, itOld.value());

            ++itOld;
        }

    } else {
        QRect r(QRect(d_ptr->m_clickPos, e->pos()).normalized());
        r.translate(1, 0);
        d_ptr->m_rubber->setGeometry(r);
        //d_ptr->m_model->clearSelection();

        int xv1 = d_ptr->m_clickPos.x();
        int xv2 = e->pos().x();
        if (xv1 > xv2) {
            int temp = xv1;
            xv1 = xv2;
            xv2 = temp;
        }
        int yv1 = d_ptr->m_clickPos.y();
        int yv2 = e->pos().y();
        if (yv1 > yv2) {
            int temp = yv1;
            yv1 = yv2;
            yv2 = temp;
        }

        QPoint p1, p2;

        if (yv2 < d_ptr->m_handleSize / 2) {
            p1 = QPoint(xv1, yv2);
            p2 = QPoint(xv2, yv2);
        } else if (yv1 > d_ptr->m_handleSize / 2) {
            p1 = QPoint(xv1, yv1);
            p2 = QPoint(xv2, yv1);
        } else {
            p1 = QPoint(xv1, qRound(d_ptr->m_handleSize / 2));
            p2 = QPoint(xv2, qRound(d_ptr->m_handleSize / 2));
        }

        QList<QtGradientStop *> beginList = d_ptr->stopsAt(p1);
        QList<QtGradientStop *> endList = d_ptr->stopsAt(p2);

        double x1 = d_ptr->fromViewport(xv1);
        double x2 = d_ptr->fromViewport(xv2);

        QListIterator<QtGradientStop *> itStop(d_ptr->m_stops);
        while (itStop.hasNext()) {
            QtGradientStop *stop = itStop.next();
            if ((stop->position() >= x1 && stop->position() <= x2) ||
                        beginList.contains(stop) || endList.contains(stop))
                d_ptr->m_model->selectStop(stop, true);
            else
                d_ptr->m_model->selectStop(stop, false);
        }
    }
}

void QtGradientStopsWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (!d_ptr->m_model)
        return;

    if (e->button() != Qt::LeftButton)
        return;

    if (d_ptr->m_clickPos != e->pos()) {
        mousePressEvent(e);
        return;
    }
    d_ptr->m_moving = true;
    d_ptr->m_moveStops.clear();
    d_ptr->m_moveOriginal.clear();

    QtGradientStop *stop = d_ptr->newStop(e->pos());

    if (!stop)
        return;

    d_ptr->m_model->clearSelection();
    d_ptr->m_model->selectStop(stop, true);

    d_ptr->setupMove(stop, e->pos().x());

    viewport()->update();
}

void QtGradientStopsWidget::keyPressEvent(QKeyEvent *e)
{
    typedef QtGradientStopsModel::PositionStopMap PositionStopMap;
    if (!d_ptr->m_model)
        return;

    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
        d_ptr->m_model->deleteStops();
    } else if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right ||
                e->key() == Qt::Key_Home || e->key() == Qt::Key_End) {
        PositionStopMap stops = d_ptr->m_model->stops();
        if (stops.isEmpty())
            return;
        QtGradientStop *newCurrent = 0;
        QtGradientStop *current = d_ptr->m_model->currentStop();
        if (!current || e->key() == Qt::Key_Home || e->key() == Qt::Key_End) {
            if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Home)
                newCurrent = stops.constBegin().value();
            else if (e->key() == Qt::Key_Right || e->key() == Qt::Key_End)
                newCurrent = (--stops.constEnd()).value();
        } else {
            PositionStopMap::ConstIterator itStop = stops.constBegin();
            while (itStop.value() != current)
                ++itStop;
            if (e->key() == Qt::Key_Left && itStop != stops.constBegin())
                --itStop;
            else if (e->key() == Qt::Key_Right && itStop != --stops.constEnd())
                ++itStop;
            newCurrent = itStop.value();
        }
        d_ptr->m_model->clearSelection();
        d_ptr->m_model->selectStop(newCurrent, true);
        d_ptr->m_model->setCurrentStop(newCurrent);
        d_ptr->ensureVisible(newCurrent);
    } else if (e->key() == Qt::Key_A) {
        if (e->modifiers() & Qt::ControlModifier)
            d_ptr->m_model->selectAll();
    }
}

void QtGradientStopsWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    if (!d_ptr->m_model)
        return;

    QtGradientStopsModel *model = d_ptr->m_model;
#ifndef QT_NO_DRAGANDDROP
    if (d_ptr->m_dragModel)
        model = d_ptr->m_dragModel;
#endif

    QSize size = viewport()->size();
    int w = size.width();
    double h = size.height() - d_ptr->m_handleSize;
    if (w <= 0)
        return;

    QPixmap pix(size);
    QPainter p;

    if (d_ptr->m_backgroundCheckered) {
        int pixSize = 20;
        QPixmap pm(2 * pixSize, 2 * pixSize);
        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::white);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::white);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::black);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::black);

        p.begin(&pix);
        p.setBrushOrigin((size.width() % pixSize + pixSize) / 2, (size.height() % pixSize + pixSize) / 2);
        p.fillRect(viewport()->rect(), pm);
        p.setBrushOrigin(0, 0);
    } else {
        p.begin(viewport());
    }

    double viewBegin = (double)w * horizontalScrollBar()->value() / d_ptr->m_scaleFactor;

    int val = horizontalScrollBar()->value();
    int max = horizontalScrollBar()->maximum();

    double begin = (double)val / (d_ptr->m_scaleFactor + max);
    double end = (double)(val + d_ptr->m_scaleFactor) / (d_ptr->m_scaleFactor + max);
    double width = end - begin;

    if (h > 0) {
        QLinearGradient lg(0, 0, w, 0);
        QMap<qreal, QtGradientStop *> stops = model->stops();
        QMapIterator<qreal, QtGradientStop *> itStop(stops);
        while (itStop.hasNext()) {
            QtGradientStop *stop = itStop.next().value();
            double pos = stop->position();
            if (pos >= begin && pos <= end) {
                double gradPos = (pos - begin) / width;
                QColor c = stop->color();
                lg.setColorAt(gradPos, c);
            }
            //lg.setColorAt(stop->position(), stop->color());
        }
        lg.setColorAt(0, model->color(begin));
        lg.setColorAt(1, model->color(end));
        QImage img(w, 1, QImage::Format_ARGB32_Premultiplied);
        QPainter p1(&img);
        p1.setCompositionMode(QPainter::CompositionMode_Source);

        /*
        if (viewBegin != 0)
            p1.translate(-viewBegin, 0);
        if (d_ptr->m_zoom != 1)
            p1.scale(d_ptr->m_zoom, 1);
            */
        p1.fillRect(0, 0, w, 1, lg);

        p.fillRect(QRectF(0, d_ptr->m_handleSize, w, h), QPixmap::fromImage(img));
    }


    double handleWidth = d_ptr->m_handleSize * d_ptr->m_scaleFactor / (w * (d_ptr->m_scaleFactor + max));

    QColor insideColor = QColor::fromRgb(0x20, 0x20, 0x20, 0xFF);
    QColor borderColor = QColor(Qt::white);
    QColor drawColor;
    QColor back1 = QColor(Qt::lightGray);
    QColor back2 = QColor(Qt::darkGray);
    QColor back = QColor::fromRgb((back1.red() + back2.red()) / 2,
            (back1.green() + back2.green()) / 2,
            (back1.blue() + back2.blue()) / 2);

    QPen pen;
    p.setRenderHint(QPainter::Antialiasing);
    QListIterator<QtGradientStop *> itStop(d_ptr->m_stops);
    itStop.toBack();
    while (itStop.hasPrevious()) {
        QtGradientStop *stop = itStop.previous();
        double x = stop->position();
        if (x >= begin - handleWidth / 2 && x <= end + handleWidth / 2) {
            double viewX = x * w * (d_ptr->m_scaleFactor + max) / d_ptr->m_scaleFactor - viewBegin;
            p.save();
            QColor c = stop->color();
#ifndef QT_NO_DRAGANDDROP
            if (stop == d_ptr->m_dragStop)
                c = d_ptr->m_dragColor;
#endif
            if ((0.3 * c.redF() + 0.59 * c.greenF() + 0.11 * c.blueF()) * c.alphaF() +
                (0.3 * back.redF() + 0.59 * back.greenF() + 0.11 * back.blueF()) * (1.0 - c.alphaF()) < 0.5) {
                drawColor = QColor::fromRgb(0xC0, 0xC0, 0xC0, 0xB0);
            } else {
                drawColor = QColor::fromRgb(0x40, 0x40, 0x40, 0x80);
            }
            QRectF rect(viewX - d_ptr->m_handleSize / 2, 0, d_ptr->m_handleSize, d_ptr->m_handleSize);
            rect.adjust(0.5, 0.5, -0.5, -0.5);
            if (h > 0) {
                pen.setWidthF(1);
                QLinearGradient lg(0, d_ptr->m_handleSize, 0, d_ptr->m_handleSize + h / 2);
                lg.setColorAt(0, drawColor);
                QColor alphaZero = drawColor;
                alphaZero.setAlpha(0);
                lg.setColorAt(1, alphaZero);
                pen.setBrush(lg);
                p.setPen(pen);
                p.drawLine(QPointF(viewX, d_ptr->m_handleSize), QPointF(viewX, d_ptr->m_handleSize + h / 2));

                pen.setWidthF(1);
                pen.setBrush(drawColor);
                p.setPen(pen);
                QRectF r1 = rect.adjusted(0.5, 0.5, -0.5, -0.5);
                QRectF r2 = rect.adjusted(1.5, 1.5, -1.5, -1.5);
                QColor inColor = QColor::fromRgb(0x80, 0x80, 0x80, 0x80);
                if (!d_ptr->m_model->isSelected(stop)) {
                    p.setBrush(c);
                    p.drawEllipse(rect);
                } else {
                    pen.setBrush(insideColor);
                    pen.setWidthF(2);
                    p.setPen(pen);
                    p.setBrush(Qt::NoBrush);
                    p.drawEllipse(r1);

                    pen.setBrush(inColor);
                    pen.setWidthF(1);
                    p.setPen(pen);
                    p.setBrush(c);
                    p.drawEllipse(r2);
                }

                if (d_ptr->m_model->currentStop() == stop) {
                    p.setBrush(Qt::NoBrush);
                    pen.setWidthF(5);
                    pen.setBrush(drawColor);
                    int corr = 4;
                    if (!d_ptr->m_model->isSelected(stop)) {
                        corr = 3;
                        pen.setWidthF(7);
                    }
                    p.setPen(pen);
                    p.drawEllipse(rect.adjusted(corr, corr, -corr, -corr));
                }

            }
            p.restore();
        }
    }
    if (d_ptr->m_backgroundCheckered) {
        p.end();
        p.begin(viewport());
        p.drawPixmap(0, 0, pix);
    }
    p.end();
}

void QtGradientStopsWidget::focusInEvent(QFocusEvent *e)
{
    Q_UNUSED(e)
    viewport()->update();
}

void QtGradientStopsWidget::focusOutEvent(QFocusEvent *e)
{
    Q_UNUSED(e)
    viewport()->update();
}

void QtGradientStopsWidget::contextMenuEvent(QContextMenuEvent *e)
{
    if (!d_ptr->m_model)
        return;

    d_ptr->m_clickPos = e->pos();

    QMenu menu(this);
    QAction *newStopAction = new QAction(tr("New Stop"), &menu);
    QAction *deleteAction = new QAction(tr("Delete"), &menu);
    QAction *flipAllAction = new QAction(tr("Flip All"), &menu);
    QAction *selectAllAction = new QAction(tr("Select All"), &menu);
    QAction *zoomInAction = new QAction(tr("Zoom In"), &menu);
    QAction *zoomOutAction = new QAction(tr("Zoom Out"), &menu);
    QAction *zoomAllAction = new QAction(tr("Reset Zoom"), &menu);
    if (d_ptr->m_model->selectedStops().isEmpty() && !d_ptr->m_model->currentStop())
        deleteAction->setEnabled(false);
    if (zoom() <= 1) {
        zoomOutAction->setEnabled(false);
        zoomAllAction->setEnabled(false);
    } else if (zoom() >= 100) {
        zoomInAction->setEnabled(false);
    }
    connect(newStopAction, SIGNAL(triggered()), this, SLOT(slotNewStop()));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(slotDelete()));
    connect(flipAllAction, SIGNAL(triggered()), this, SLOT(slotFlipAll()));
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(slotSelectAll()));
    connect(zoomInAction, SIGNAL(triggered()), this, SLOT(slotZoomIn()));
    connect(zoomOutAction, SIGNAL(triggered()), this, SLOT(slotZoomOut()));
    connect(zoomAllAction, SIGNAL(triggered()), this, SLOT(slotResetZoom()));
    menu.addAction(newStopAction);
    menu.addAction(deleteAction);
    menu.addAction(flipAllAction);
    menu.addAction(selectAllAction);
    menu.addSeparator();
    menu.addAction(zoomInAction);
    menu.addAction(zoomOutAction);
    menu.addAction(zoomAllAction);
    menu.exec(e->globalPos());
}

void QtGradientStopsWidget::wheelEvent(QWheelEvent *e)
{
    int numDegrees = e->delta() / 8;
    int numSteps = numDegrees / 15;

    int shift = numSteps;
    if (shift < 0)
        shift = -shift;
    int pow = 1 << shift;
    //const double c = 0.7071067; // 2 steps per doubled value
    const double c = 0.5946036; // 4 steps pre doubled value
    // in general c = pow(2, 1 / n) / 2; where n is the step
    double factor = pow * c;

    double newZoom = zoom();
    if (numSteps < 0)
        newZoom /= factor;
    else
        newZoom *= factor;
    if (newZoom > 100)
        newZoom = 100;
    if (newZoom < 1)
        newZoom = 1;

    if (newZoom == zoom())
        return;

    setZoom(newZoom);
    emit zoomChanged(zoom());
}

#ifndef QT_NO_DRAGANDDROP
void QtGradientStopsWidget::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (!mime->hasColor())
        return;
    event->accept();
    d_ptr->m_dragModel = d_ptr->m_model->clone();

    d_ptr->m_dragColor = qvariant_cast<QColor>(mime->colorData());
    update();
}

void QtGradientStopsWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QRectF rect = viewport()->rect();
    rect.adjust(0, d_ptr->m_handleSize, 0, 0);
    double x = d_ptr->fromViewport(event->pos().x());
    QtGradientStop *dragStop = d_ptr->stopAt(event->pos());
    if (dragStop) {
        event->accept();
        d_ptr->removeClonedStop();
        d_ptr->changeStop(dragStop->position());
    } else if (rect.contains(event->pos())) {
        event->accept();
        if (d_ptr->m_model->at(x)) {
            d_ptr->removeClonedStop();
            d_ptr->changeStop(x);
        } else {
            d_ptr->restoreChangedStop();
            d_ptr->cloneStop(x);
        }
    } else {
        event->ignore();
        d_ptr->removeClonedStop();
        d_ptr->restoreChangedStop();
    }

    update();
}

void QtGradientStopsWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
    d_ptr->clearDrag();
    update();
}

void QtGradientStopsWidget::dropEvent(QDropEvent *event)
{
    event->accept();
    if (!d_ptr->m_dragModel)
        return;

    if (d_ptr->m_changedStop)
        d_ptr->m_model->changeStop(d_ptr->m_model->at(d_ptr->m_changedStop->position()), d_ptr->m_dragColor);
    else if (d_ptr->m_clonedStop)
        d_ptr->m_model->addStop(d_ptr->m_clonedStop->position(), d_ptr->m_dragColor);

    d_ptr->clearDrag();
    update();
}

void QtGradientStopsWidgetPrivate::clearDrag()
{
    removeClonedStop();
    restoreChangedStop();
    delete m_dragModel;
    m_dragModel = 0;
}

void QtGradientStopsWidgetPrivate::removeClonedStop()
{
    if (!m_clonedStop)
        return;
    m_dragModel->removeStop(m_clonedStop);
    m_clonedStop = 0;
}

void QtGradientStopsWidgetPrivate::restoreChangedStop()
{
    if (!m_changedStop)
        return;
    m_dragModel->changeStop(m_changedStop, m_model->at(m_changedStop->position())->color());
    m_changedStop = 0;
    m_dragStop = 0;
}

void QtGradientStopsWidgetPrivate::changeStop(qreal pos)
{
    QtGradientStop *stop = m_dragModel->at(pos);
    if (!stop)
        return;

    m_dragModel->changeStop(stop, m_dragColor);
    m_changedStop = stop;
    m_dragStop = m_model->at(stop->position());
}

void QtGradientStopsWidgetPrivate::cloneStop(qreal pos)
{
    if (m_clonedStop) {
        m_dragModel->moveStop(m_clonedStop, pos);
        return;
    }
    QtGradientStop *stop = m_dragModel->at(pos);
    if (stop)
        return;

    m_clonedStop = m_dragModel->addStop(pos, m_dragColor);
}

#endif

void QtGradientStopsWidget::setZoom(double zoom)
{
    double z = zoom;
    if (z < 1)
        z = 1;
    else if (z > 100)
        z = 100;

    if (d_ptr->m_zoom == z)
        return;

    d_ptr->m_zoom = z;
    int oldMax = horizontalScrollBar()->maximum();
    int oldVal = horizontalScrollBar()->value();
    horizontalScrollBar()->setRange(0, qRound(d_ptr->m_scaleFactor * (d_ptr->m_zoom - 1)));
    int newMax = horizontalScrollBar()->maximum();
    double newVal = (oldVal + (double)d_ptr->m_scaleFactor / 2) * (newMax + d_ptr->m_scaleFactor)
                / (oldMax + d_ptr->m_scaleFactor) - (double)d_ptr->m_scaleFactor / 2;
    horizontalScrollBar()->setValue(qRound(newVal));
    viewport()->update();
}

double QtGradientStopsWidget::zoom() const
{
    return d_ptr->m_zoom;
}

QT_END_NAMESPACE

#include "moc_qtgradientstopswidget.cpp"
