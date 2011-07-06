/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QtDeclarative/QDeclarativeItem>
#include <QtScript/QScriptValue>

namespace QmlProfiler {
namespace Internal {

class TimelineView : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(qint64 startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(qint64 endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    Q_PROPERTY(qreal startX READ startX WRITE setStartX NOTIFY startXChanged)
    Q_PROPERTY(qreal totalWidth READ totalWidth NOTIFY totalWidthChanged)

public:
    explicit TimelineView(QDeclarativeItem *parent = 0);

    QDeclarativeComponent * delegate() const
    {
        return m_delegate;
    }

    qint64 startTime() const
    {
        return m_startTime;
    }

    qint64 endTime() const
    {
        return m_endTime;
    }

    qreal startX() const
    {
        return m_startX;
    }

    qreal totalWidth() const
    {
        return m_totalWidth;
    }

signals:
    void delegateChanged(QDeclarativeComponent * arg);
    void startTimeChanged(qint64 arg);
    void endTimeChanged(qint64 arg);
    void startXChanged(qreal arg);
    void totalWidthChanged(qreal arg);

public slots:
    void clearData();
    void setRanges(const QScriptValue &value);
    void updateTimeline(bool updateStartX = true);

    void setDelegate(QDeclarativeComponent * arg)
    {
        if (m_delegate != arg) {
            m_delegate = arg;
            emit delegateChanged(arg);
        }
    }

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

    void setStartX(qreal arg);

protected:
    void componentComplete();

private:
    QDeclarativeComponent * m_delegate;
    QScriptValue m_ranges;
    typedef QList<QScriptValue> ValueList;
    QList<ValueList> m_rangeList;
    QHash<int,QDeclarativeItem*> m_items;
    qint64 m_startTime;
    qint64 m_endTime;
    qreal m_startX;
    int prevMin;
    int prevMax;
    QList<qreal> m_starts;
    QList<qreal> m_ends;

    struct PrevLimits {
        PrevLimits(int _min, int _max) : min(_min), max(_max) {}
        int min;
        int max;
    };

    QList<PrevLimits> m_prevLimits;
    qreal m_totalWidth;
};

} // namespace Internal
} // namespace QmlProfiler

QML_DECLARE_TYPE(QmlProfiler::Internal::TimelineView)

#endif // TIMELINEVIEW_H
