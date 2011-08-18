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
#include <qmljsdebugclient/qmlprofilereventlist.h>

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
    Q_PROPERTY(QObject* eventList READ eventList WRITE setEventList NOTIFY eventListChanged)
    Q_PROPERTY(qreal cachedProgress READ cachedProgress NOTIFY cachedProgressChanged)

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

    qreal cachedProgress() const;

    QmlJsDebugClient::QmlProfilerEventList *eventList() const { return m_eventList; }
    void setEventList(QObject *eventList)
    {
        m_eventList = qobject_cast<QmlJsDebugClient::QmlProfilerEventList *>(eventList);
        emit eventListChanged(m_eventList);
    }

    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE QString getFilename(int index) const;
    Q_INVOKABLE int getLine(int index) const;
    Q_INVOKABLE QString getDetails(int index) const;
    Q_INVOKABLE void rebuildCache();

signals:
    void delegateChanged(QDeclarativeComponent * arg);
    void startTimeChanged(qint64 arg);
    void endTimeChanged(qint64 arg);
    void startXChanged(qreal arg);
    void totalWidthChanged(qreal arg);
    void eventListChanged(QmlJsDebugClient::QmlProfilerEventList *list);

    void cachedProgressChanged();
    void cacheReady();

public slots:
    void clearData();
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
    void createItem(int itemIndex);
    void updateItemPosition(int itemIndex);

public slots:
    void increaseCache();
    void purgeCache();

private:
    QDeclarativeComponent * m_delegate;
    QHash<int,QDeclarativeItem*> m_items;
    qint64 m_itemCount;
    qint64 m_startTime;
    qint64 m_endTime;
    qreal m_startX;
    qreal m_spacing;
    int prevMin;
    int prevMax;

    QmlJsDebugClient::QmlProfilerEventList *m_eventList;

    qreal m_totalWidth;
    int m_lastCachedIndex;
    bool m_creatingCache;
    int m_oldCacheSize;

};

} // namespace Internal
} // namespace QmlProfiler

QML_DECLARE_TYPE(QmlProfiler::Internal::TimelineView)

#endif // TIMELINEVIEW_H
