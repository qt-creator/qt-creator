/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TIMELINERENDERER_H
#define TIMELINERENDERER_H

#include <QDeclarativeItem>
#include <QScriptValue>
#include "qmlprofilertimelinemodelproxy.h"
#include "timelinemodelaggregator.h"

namespace QmlProfiler {
namespace Internal {

class TimelineRenderer : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(qint64 startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(qint64 endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    Q_PROPERTY(QObject *profilerModelProxy READ profilerModelProxy WRITE setProfilerModelProxy NOTIFY profilerModelProxyChanged)
    Q_PROPERTY(bool selectionLocked READ selectionLocked WRITE setSelectionLocked NOTIFY selectionLockedChanged)
    Q_PROPERTY(int selectedItem READ selectedItem WRITE setSelectedItem NOTIFY selectedItemChanged)
    Q_PROPERTY(int selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged)
    Q_PROPERTY(int startDragArea READ startDragArea WRITE setStartDragArea NOTIFY startDragAreaChanged)
    Q_PROPERTY(int endDragArea READ endDragArea WRITE setEndDragArea NOTIFY endDragAreaChanged)

public:
    explicit TimelineRenderer(QDeclarativeItem *parent = 0);

    qint64 startTime() const
    {
        return m_startTime;
    }

    qint64 endTime() const
    {
        return m_endTime;
    }

    bool selectionLocked() const
    {
        return m_selectionLocked;
    }

    int selectedItem() const
    {
        return m_selectedItem;
    }

    int selectedModel() const
    {
        return m_selectedModel;
    }

    int startDragArea() const
    {
        return m_startDragArea;
    }

    int endDragArea() const
    {
        return m_endDragArea;
    }

    TimelineModelAggregator *profilerModelProxy() const { return m_profilerModelProxy; }
    void setProfilerModelProxy(QObject *profilerModelProxy)
    {
        if (m_profilerModelProxy) {
            disconnect(m_profilerModelProxy, SIGNAL(expandedChanged()), this, SLOT(requestPaint()));
        }
        m_profilerModelProxy = qobject_cast<TimelineModelAggregator *>(profilerModelProxy);

        if (m_profilerModelProxy) {
            connect(m_profilerModelProxy, SIGNAL(expandedChanged()), this, SLOT(requestPaint()));
        }
        emit profilerModelProxyChanged(m_profilerModelProxy);
    }

    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE QString getFilename(int index) const;
    Q_INVOKABLE int getLine(int index) const;
    Q_INVOKABLE QString getDetails(int index) const;
    Q_INVOKABLE int getYPosition(int modelIndex, int index) const;

//    Q_INVOKABLE void setRowExpanded(int modelIndex, int rowIndex, bool expanded);

    Q_INVOKABLE void selectNext();
    Q_INVOKABLE void selectPrev();
    Q_INVOKABLE int nextItemFromId(int modelIndex, int eventId) const;
    Q_INVOKABLE int prevItemFromId(int modelIndex, int eventId) const;
    Q_INVOKABLE void selectNextFromId(int modelIndex, int eventId);
    Q_INVOKABLE void selectPrevFromId(int modelIndex, int eventId);
    Q_INVOKABLE int modelIndexFromType(int typeIndex) const;

signals:
    void startTimeChanged(qint64 arg);
    void endTimeChanged(qint64 arg);
    void profilerModelProxyChanged(TimelineModelAggregator *list);
    void selectionLockedChanged(bool locked);
    void selectedItemChanged(int modelIndex, int itemIndex);
    void selectedModelChanged(int modelIndex);
    void startDragAreaChanged(int startDragArea);
    void endDragAreaChanged(int endDragArea);
    void itemPressed(int modelIndex, int pressedItem);

public slots:
    void clearData();
    void requestPaint();


    void setStartTime(qint64 arg)
    {
        if (m_startTime != arg) {
            m_startTime = arg;
            emit startTimeChanged(arg);
        }
    }

    void setEndTime(qint64 arg)
    {
        if (m_endTime != arg) {
            m_endTime = arg;
            emit endTimeChanged(arg);
        }
    }

    void setSelectionLocked(bool locked)
    {
        if (m_selectionLocked != locked) {
            m_selectionLocked = locked;
            update();
            emit selectionLockedChanged(locked);
        }
    }

    void setSelectedItem(int itemIndex)
    {
        if (m_selectedItem != itemIndex) {
            m_selectedItem = itemIndex;
            update();
            emit selectedItemChanged(m_selectedModel, itemIndex);
        }
    }

    void setSelectedModel(int modelIndex)
    {
        if (m_selectedModel != modelIndex) {
            m_selectedModel = modelIndex;
            update();
            emit selectedModelChanged(modelIndex);
        }
    }

    void setStartDragArea(int startDragArea)
    {
        if (m_startDragArea != startDragArea) {
            m_startDragArea = startDragArea;
            emit startDragAreaChanged(startDragArea);
        }
    }

    void setEndDragArea(int endDragArea)
    {
        if (m_endDragArea != endDragArea) {
            m_endDragArea = endDragArea;
            emit endDragAreaChanged(endDragArea);
        }
    }

protected:
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    virtual void componentComplete();
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

private:
    void drawItemsToPainter(QPainter *p, int modelIndex, int fromIndex, int toIndex);
    void drawSelectionBoxes(QPainter *p, int modelIndex, int fromIndex, int toIndex);
    void drawBindingLoopMarkers(QPainter *p, int modelIndex, int fromIndex, int toIndex);
    int modelFromPosition(int y);

    void manageClicked();
    void manageHovered(int x, int y);

private:
    qint64 m_startTime;
    qint64 m_endTime;
    qreal m_spacing;
    qint64 m_lastStartTime;
    qint64 m_lastEndTime;

    TimelineModelAggregator *m_profilerModelProxy;
//    BasicTimelineModel *m_profilerModelProxy;

    QList<int> m_rowLastX;
    QList<int> m_rowStarts;
    QList<int> m_rowWidths;
    QList<bool> m_rowsExpanded;
    QList<int> m_modelRowEnds;

    struct {
        qint64 startTime;
        qint64 endTime;
        int row;
        int eventIndex;
        int modelIndex;
    } m_currentSelection;

    int m_selectedItem;
    int m_selectedModel;
    bool m_selectionLocked;
    int m_startDragArea;
    int m_endDragArea;
};

} // namespace Internal
} // namespace QmlProfiler

QML_DECLARE_TYPE(QmlProfiler::Internal::TimelineRenderer)

#endif // TIMELINERENDERER_H
