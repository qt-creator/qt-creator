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

#include "timelineview.h"

#include <qdeclarativecontext.h>
#include <qdeclarativeproperty.h>
#include <QtCore/QTimer>

using namespace QmlProfiler::Internal;

#define CACHE_ENABLED true
#define CACHE_UPDATEDELAY 10
#define CACHE_STEP 200

TimelineView::TimelineView(QDeclarativeItem *parent) :
    QDeclarativeItem(parent), m_delegate(0), m_itemCount(0), m_startTime(0), m_endTime(0), m_startX(0), m_spacing(0),
    prevMin(0), prevMax(0), m_eventList(0), m_totalWidth(0), m_lastCachedIndex(0), m_creatingCache(false), m_oldCacheSize(0)
{
}

void TimelineView::componentComplete()
{
    QDeclarativeItem::componentComplete();
}

void TimelineView::clearData()
{
    if (CACHE_ENABLED)
        foreach (QDeclarativeItem *item, m_items.values())
            item->setVisible(false);
    else
        foreach (QDeclarativeItem *item, m_items.values())
            delete m_items.take(m_items.key(item));

    m_startTime = 0;
    m_endTime = 0;
    m_startX = 0;
    prevMin = 0;
    prevMax = 0;
    m_totalWidth = 0;
    m_lastCachedIndex = 0;
}

void TimelineView::setStartX(qreal arg)
{
    if (arg == m_startX)
        return;

    qreal window = m_endTime - m_startTime;
    if (window == 0)    //###
        return;
    qreal spacing = width() / window;
    qreal oldStart = m_startTime;
    m_startTime = arg / spacing;
    m_endTime += m_startTime - oldStart;

    updateTimeline(false);
    m_startX = arg;
    emit startXChanged(m_startX);
}

void TimelineView::updateTimeline(bool updateStartX)
{
    if (!m_delegate)
        return;

    if (!m_eventList)
        return;

    qreal totalRange = m_eventList->lastTimeMark() - m_eventList->firstTimeMark();
    qreal window = m_endTime - m_startTime;

    if (window == 0)    //###
        return;

    qreal newSpacing = width() / window;
    bool spacingChanged = (newSpacing != m_spacing);
    m_spacing = newSpacing;

    qreal oldtw = m_totalWidth;
    m_totalWidth = totalRange * m_spacing;


    int minsample = m_eventList->findFirstIndex(m_startTime + m_eventList->firstTimeMark());
    int maxsample = m_eventList->findLastIndex(m_endTime + m_eventList->firstTimeMark());

    if (updateStartX) {
        qreal oldStartX = m_startX;
        m_startX = qRound(m_startTime * m_spacing);
        if (m_startX != oldStartX) {
            emit startXChanged(m_startX);
        }
    }

    //### emitting this before startXChanged was causing issues
    if (m_totalWidth != oldtw)
        emit totalWidthChanged(m_totalWidth);


    // the next loops have to be modified with the new implementation of the cache

    // hide items that are not visible any more
    if (maxsample < prevMin || minsample > prevMax) {
        for (int i = prevMin; i <= prevMax; ++i)
            if (m_items.contains(i)) {
                if (CACHE_ENABLED)
                    m_items.value(i)->setVisible(false);
                else
                    delete m_items.take(i);
            }
    } else {
        if (minsample > prevMin && minsample <= prevMax)
            for (int i = prevMin; i < minsample; ++i)
                if (m_items.contains(i)) {
                    if (CACHE_ENABLED)
                        m_items.value(i)->setVisible(false);
                    else
                        delete m_items.take(i);
                }

        if (maxsample >= prevMin && maxsample < prevMax)
            for (int i = maxsample + 1; i <= prevMax; ++i)
                if (m_items.contains(i)) {
                    if (CACHE_ENABLED)
                        m_items.value(i)->setVisible(false);
                    else
                        delete m_items.take(i);
                }
    }

    // Update visible items
    for (int i = minsample; i <= maxsample; ++i) {
        if (!m_items.contains(i)) {
            createItem(i);
            m_items.value(i)->setVisible(true);
        }
        else
        if (spacingChanged || !m_items.value(i)->isVisible()) {
            m_items.value(i)->setVisible(true);
            updateItemPosition(i);
        }
    }

    prevMin = minsample;
    prevMax = maxsample;

}

void TimelineView::createItem(int itemIndex)
{
    QDeclarativeContext *ctxt = new QDeclarativeContext(qmlContext(this));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(m_delegate->beginCreate(ctxt));
    m_items.insert(itemIndex, item);

    ctxt->setParent(item); //### QDeclarative_setParent_noEvent(ctxt, item); instead?
    ctxt->setContextProperty("index", itemIndex);
    ctxt->setContextProperty("type", m_eventList->getType(itemIndex));
    ctxt->setContextProperty("nestingLevel", m_eventList->getNestingLevel(itemIndex));
    ctxt->setContextProperty("nestingDepth", m_eventList->getNestingDepth(itemIndex));

    updateItemPosition(itemIndex);

    item->setVisible(false);

    item->setParentItem(this);
    m_delegate->completeCreate();
    m_itemCount++;
}

void TimelineView::updateItemPosition(int itemIndex)
{
    QDeclarativeItem *item = m_items.value(itemIndex);
    if (item) {
        qreal itemStartPos = (m_eventList->getStartTime(itemIndex) - m_eventList->firstTimeMark()) * m_spacing;
        item->setX(itemStartPos);
        qreal width = (m_eventList->getEndTime(itemIndex) - m_eventList->getStartTime(itemIndex)) * m_spacing;
        item->setWidth(width > 1 ? width : 1);
    }
}

void TimelineView::rebuildCache()
{
    if (CACHE_ENABLED) {
        m_lastCachedIndex = 0;
        m_creatingCache = false;
        m_oldCacheSize = m_items.count();
        emit cachedProgressChanged();
        QTimer::singleShot(CACHE_UPDATEDELAY, this, SLOT(purgeCache()));
    } else {
        m_creatingCache = true;
        m_lastCachedIndex = m_eventList->count();
        emit cacheReady();
    }
}

qreal TimelineView::cachedProgress() const
{
    qreal progress;
    if (!m_creatingCache) {
        if (m_oldCacheSize == 0)
            progress = 0.5;
       else
            progress = (m_lastCachedIndex * 0.5) / m_oldCacheSize;
    }
    else
        progress = 0.5 + (m_lastCachedIndex * 0.5) / m_eventList->count();

    return progress;
}

void TimelineView::increaseCache()
{
    int totalCount = m_eventList->count();
    if (m_lastCachedIndex >= totalCount) {
        emit cacheReady();
        return;
    }

    for (int i = 0; i < CACHE_STEP; i++) {
        createItem(m_lastCachedIndex);
        m_lastCachedIndex++;
        if (m_lastCachedIndex >= totalCount)
            break;
    }

    emit cachedProgressChanged();

    QTimer::singleShot(CACHE_UPDATEDELAY, this, SLOT(increaseCache()));
}

void TimelineView::purgeCache()
{
    if (m_items.isEmpty()) {
        m_creatingCache = true;
        m_lastCachedIndex = 0;
        QTimer::singleShot(CACHE_UPDATEDELAY, this, SLOT(increaseCache()));
        return;
    }

    for (int i=0; i < CACHE_STEP; i++)
    {
        if (m_items.contains(m_lastCachedIndex))
            delete m_items.take(m_lastCachedIndex);

        m_lastCachedIndex++;
        if (m_items.isEmpty())
            break;
    }

    emit cachedProgressChanged();
    QTimer::singleShot(CACHE_UPDATEDELAY, this, SLOT(purgeCache()));
}

qint64 TimelineView::getDuration(int index) const
{
    Q_ASSERT(m_eventList);
    return m_eventList->getEndTime(index) - m_eventList->getStartTime(index);
}

QString TimelineView::getFilename(int index) const
{
    Q_ASSERT(m_eventList);
    return m_eventList->getFilename(index);
}

int TimelineView::getLine(int index) const
{
    Q_ASSERT(m_eventList);
    return m_eventList->getLine(index);
}

QString TimelineView::getDetails(int index) const
{
    Q_ASSERT(m_eventList);
    return m_eventList->getDetails(index);
}
