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

using namespace QmlProfiler::Internal;

TimelineView::TimelineView(QDeclarativeItem *parent) :
    QDeclarativeItem(parent), m_delegate(0), m_startTime(0), m_endTime(0), m_startX(0),
    prevMin(0), prevMax(0), m_totalWidth(0)
{
}

void TimelineView::componentComplete()
{
    QDeclarativeItem::componentComplete();
}

void TimelineView::clearData()
{
    m_rangeList.clear();
    m_items.clear();
    m_startTime = 0;
    m_endTime = 0;
    m_startX = 0;
    prevMin = 0;
    prevMax = 0;
    m_prevLimits.clear();
    m_totalWidth = 0;

}

void TimelineView::setRanges(const QScriptValue &value)
{
    //TODO clear old values (always?)
    m_ranges = value;

    //### below code not yet used anywhere
    int length = m_ranges.property("length").toInt32();

    for (int i = 0; i < length; ++i) {
        int type = m_ranges.property(i).property("type").toNumber();
        Q_ASSERT(type >= 0);
        while (m_rangeList.count() <= type)
            m_rangeList.append(ValueList());
        m_rangeList[type] << m_ranges.property(i);
    }

    for (int i = 0; i < m_rangeList.count(); ++i)
        m_prevLimits << PrevLimits(0, 0);

    qreal startValue = m_ranges.property(0).property("start").toNumber();
    m_starts.clear();
    m_starts.reserve(length);
    m_ends.clear();
    m_ends.reserve(length);
    for (int i = 0; i < length; ++i) {
        m_starts.append(m_ranges.property(i).property("start").toNumber() - startValue);
        m_ends.append(m_ranges.property(i).property("start").toNumber() + m_ranges.property(i).property("duration").toNumber() - startValue);
    }
}

void TimelineView::setStartX(qreal arg)
{
    if (arg == m_startX)
        return;

    if (!m_ranges.isArray())
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

    if (!m_ranges.isArray())
        return;

    int length = m_ranges.property("length").toInt32();

    qreal startValue = m_ranges.property(0).property("start").toNumber();
    qreal endValue = m_ranges.property(length-1).property("start").toNumber() + m_ranges.property(length-1).property("duration").toNumber();

    qreal totalRange = endValue - startValue;
    qreal window = m_endTime - m_startTime;

    if (window == 0)    //###
        return;

    qreal spacing = width() / window;
    qreal oldtw = m_totalWidth;
    m_totalWidth = totalRange * spacing;

    // Find region samples
    int minsample = 0;
    int maxsample = 0;

    for (int i = 0; i < length; ++i) {
        if (m_ends.at(i) >= m_startTime)
            break;
        minsample = i;
    }

    for (int i = minsample + 1; i < length; ++i) {
        maxsample = i;
        if (m_starts.at(i) > m_endTime)
            break;
    }

    //### overkill (if we can expose whether or not data is nested)
    for (int i = maxsample + 1; i < length; ++i) {
        if (m_starts.at(i) < m_endTime)
            maxsample = i;
    }

    //qDebug() << maxsample - minsample;

    if (updateStartX) {
        qreal oldStartX = m_startX;
        m_startX = qRound(m_startTime * spacing);
        if (m_startX != oldStartX) {
            emit startXChanged(m_startX);
        }
    }

    //### emitting this before startXChanged was causing issues
    if (m_totalWidth != oldtw)
        emit totalWidthChanged(m_totalWidth);

    //clear items no longer in view
    while (prevMin < minsample) {
        delete m_items.take(prevMin);
        ++prevMin;
    }
    while (prevMax > maxsample) {
        delete m_items.take(prevMax);
        --prevMax;
    }

    // Show items
    int z = 0;
    for (int i = maxsample; i >= minsample; --i) {
        QDeclarativeItem *item = 0;
        item = m_items.value(i);
        bool creating = false;
        if (!item) {
            QDeclarativeContext *ctxt = new QDeclarativeContext(qmlContext(this));
            item = qobject_cast<QDeclarativeItem*>(m_delegate->beginCreate(ctxt));
            m_items.insert(i, item);
            creating = true;

            int type = m_ranges.property(i).property("type").toNumber();

            ctxt->setParent(item); //### QDeclarative_setParent_noEvent(ctxt, item); instead?
            ctxt->setContextProperty("duration", qMax(qRound(m_ranges.property(i).property("duration").toNumber()/qreal(1000)),1));
            ctxt->setContextProperty("fileName", m_ranges.property(i).property("fileName").toString());
            ctxt->setContextProperty("line", m_ranges.property(i).property("line").toNumber());
            ctxt->setContextProperty("index", i);
            ctxt->setContextProperty("nestingLevel", m_ranges.property(i).property("nestingLevel").toNumber());
            ctxt->setContextProperty("nestingDepth", m_ranges.property(i).property("nestingDepth").toNumber());
            QString label;
            QVariantList list = m_ranges.property(i).property("label").toVariant().value<QVariantList>();
            for (int i = 0; i < list.size(); ++i) {
                if (i > 0)
                    label += QLatin1Char('\n');
                QString sub = list.at(i).toString();

                //### only do rewrite for bindings...
                if (type == 3) {
                    //### don't construct in loop
                    QRegExp rewrite("\\(function \\$(\\w+)\\(\\) \\{ return (.+) \\}\\)");
                    bool match = rewrite.exactMatch(sub);
                    if (match)
                        sub = rewrite.cap(1) + ": " + rewrite.cap(2);
                }

                label += sub;
            }
            ctxt->setContextProperty("label", label);
            ctxt->setContextProperty("type", type);
            item->setParentItem(this);
        }
        if (item) {
            item->setX(m_starts.at(i)*spacing);
            qreal width = (m_ends.at(i)-m_starts.at(i)) * spacing;
            item->setWidth(width > 1 ? width : 1);
            item->setZValue(++z);
        }
        if (creating)
            m_delegate->completeCreate();
    }

    prevMin = minsample;
    prevMax = maxsample;
}
