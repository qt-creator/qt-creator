/****************************************************************************
**
** Copyright (C) 2013 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "pixmapcachemodel.h"
#include "qmldebug/qmlprofilereventtypes.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"

#include <QDebug>

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler::Internal;

class PixmapCacheModel::PixmapCacheModelPrivate {
public:
    PixmapCacheModelPrivate(PixmapCacheModel *qq):q(qq) {}

    ~PixmapCacheModelPrivate();

    PixmapCacheModel *q;

    void computeCacheSizes();
    void resizeUnfinishedLoads();
    void flattenLoads();
    void computeRowCounts();

    QVector < PixmapCacheModel::PixmapCacheEvent > eventList;
    QVector < QString > pixmapUrls;
    QVector < QPair<int, int> > pixmapSizes;
    bool isExpanded;
    int expandedRowCount;
    int collapsedRowCount;
    QString displayTime(double time);
    void addVP(QVariantList &l, QString label, qint64 time);

    qint64 minCacheSize;
    qint64 maxCacheSize;
};

PixmapCacheModel::PixmapCacheModel(QObject *parent)
    : AbstractTimelineModel(parent), d(new PixmapCacheModelPrivate(this))
{
    d->collapsedRowCount = 1;
    d->expandedRowCount = 1;
}

PixmapCacheModel::~PixmapCacheModel()
{
}

int PixmapCacheModel::categories() const
{
    return 1;
}

QStringList PixmapCacheModel::categoryTitles() const
{
    QStringList retString;
    retString << categoryLabel(0);
    return retString;
}

QString PixmapCacheModel::name() const
{
    return QLatin1String("PixmapCacheTimeLineModel");
}

int PixmapCacheModel::count() const
{
    return d->eventList.count();
}

bool PixmapCacheModel::isEmpty() const
{
    return d->eventList.isEmpty();
}

bool PixmapCacheModel::eventAccepted(const QmlProfiler::Internal::QmlProfilerSimpleModel::QmlEventData &event) const
{
    return (event.eventType == QmlDebug::PixmapCacheEvent);
}

qint64 PixmapCacheModel::lastTimeMark() const
{
    return d->eventList.last().startTime;
}

bool PixmapCacheModel::expanded(int ) const
{
    return d->isExpanded;
}

void PixmapCacheModel::setExpanded(int category, bool expanded)
{
    Q_UNUSED(category);
    d->isExpanded = expanded;
}

int PixmapCacheModel::categoryDepth(int categoryIndex) const
{
    Q_UNUSED(categoryIndex);
    if (isEmpty())
        return 1;
    if (d->isExpanded)
        return d->expandedRowCount;
    return d->collapsedRowCount;
}

int PixmapCacheModel::categoryCount() const
{
    return 1;
}

const QString PixmapCacheModel::categoryLabel(int categoryIndex) const
{
    Q_UNUSED(categoryIndex);
    return QLatin1String("Pixmap Cache");
}

int PixmapCacheModel::findFirstIndex(qint64 startTime) const
{
    int candidate = findFirstIndexNoParents(startTime);
    return candidate;
}

int PixmapCacheModel::findFirstIndexNoParents(qint64 startTime) const
{
    if (d->eventList.isEmpty())
        return -1;
    if (d->eventList.count() == 1 || d->eventList.first().startTime+d->eventList.first().duration >= startTime)
        return 0;
    else
        if (d->eventList.last().startTime+d->eventList.last().duration <= startTime)
            return -1;

    int fromIndex = 0;
    int toIndex = d->eventList.count()-1;
    while (toIndex - fromIndex > 1) {
        int midIndex = (fromIndex + toIndex)/2;
        if (d->eventList[midIndex].startTime + d->eventList[midIndex].duration < startTime)
            fromIndex = midIndex;
        else
            toIndex = midIndex;
    }
    return toIndex;
}

int PixmapCacheModel::findLastIndex(qint64 endTime) const
{
    if (d->eventList.isEmpty())
        return -1;
    if (d->eventList.first().startTime >= endTime)
        return -1;
    if (d->eventList.count() == 1)
        return 0;
    if (d->eventList.last().startTime <= endTime)
        return d->eventList.count()-1;

    int fromIndex = 0;
    int toIndex = d->eventList.count()-1;
    while (toIndex - fromIndex > 1) {
        int midIndex = (fromIndex + toIndex)/2;
        if (d->eventList[midIndex].startTime < endTime)
            fromIndex = midIndex;
        else
            toIndex = midIndex;
    }

    return fromIndex;
}

int PixmapCacheModel::getEventType(int index) const
{
    Q_UNUSED(index);
    return QmlDebug::PixmapCacheEvent;
}

int PixmapCacheModel::getEventCategory(int index) const
{
    Q_UNUSED(index);
    return 0;
}

int PixmapCacheModel::getEventRow(int index) const
{
    if (d->isExpanded)
        return d->eventList[index].rowNumberExpanded;
    return d->eventList[index].rowNumberCollapsed;
}

qint64 PixmapCacheModel::getDuration(int index) const
{
    return d->eventList[index].duration;
}

qint64 PixmapCacheModel::getStartTime(int index) const
{
    return d->eventList[index].startTime;
}

qint64 PixmapCacheModel::getEndTime(int index) const
{
    return getStartTime(index)+getDuration(index);
}

int PixmapCacheModel::getEventId(int index) const
{
    return d->eventList[index].eventId;
}

QColor PixmapCacheModel::getColor(int index) const
{
    if (d->eventList[index].pixmapEventType == PixmapCacheCountChanged)
        return QColor::fromHsl(240, 76, 166);

    int ndx = getEventId(index);
    return QColor::fromHsl((ndx*25)%360, 76, 166);
}

float PixmapCacheModel::getHeight(int index) const
{
    if (d->eventList[index].pixmapEventType == PixmapCacheCountChanged) {
        float scale = d->maxCacheSize - d->minCacheSize;
        float fraction = 1.0f;
        if (scale > 1)
            fraction = (float)(d->eventList[index].cacheSize -
                                d->minCacheSize) / scale;

        return fraction * 0.85f + 0.15f;
    }

    return 1.0f;
}

QString getFilenameOnly(QString absUrl)
{
    int characterPos = absUrl.lastIndexOf(QLatin1Char('/'))+1;
    if (characterPos < absUrl.length())
        absUrl = absUrl.mid(characterPos);
    return absUrl;
}

const QVariantList PixmapCacheModel::getLabelsForCategory(int category) const
{
    Q_UNUSED(category);
    QVariantList result;

    if (d->isExpanded && !isEmpty()) {
        {
            // Cache Size
            QVariantMap element;
            element.insert(QLatin1String("displayName"), QVariant(QLatin1String("Cache Size")));
            element.insert(QLatin1String("description"), QVariant(QLatin1String("Cache Size")));

            element.insert(QLatin1String("id"), QVariant(0));
            result << element;
        }

        for (int i=0; i < d->pixmapUrls.count(); i++) {
            // Loading
            QVariantMap element;
            element.insert(QLatin1String("displayName"), QVariant(getFilenameOnly(d->pixmapUrls[i])));
            element.insert(QLatin1String("description"), QVariant(getFilenameOnly(d->pixmapUrls[i])));

            element.insert(QLatin1String("id"), QVariant(i+1));
            result << element;
        }
    }

    return result;
}

QString PixmapCacheModel::PixmapCacheModelPrivate::displayTime(double time)
{
    if (time < 1e6)
        return QString::number(time/1e3,'f',3) + trUtf8(" \xc2\xb5s");
    if (time < 1e9)
        return QString::number(time/1e6,'f',3) + tr(" ms");

    return QString::number(time/1e9,'f',3) + tr(" s");
}

void PixmapCacheModel::PixmapCacheModelPrivate::addVP(QVariantList &l, QString label, qint64 time)
{
    if (time > 0) {
        QVariantMap res;
        res.insert(label, QVariant(displayTime(time)));
        l << res;
    }
}

const QVariantList PixmapCacheModel::getEventDetails(int index) const
{
    QVariantList result;
    PixmapCacheEvent *ev = &d->eventList[index];

    {
        QVariantMap res;
        if (ev->pixmapEventType == PixmapCacheCountChanged)
            res.insert(QLatin1String("title"), QVariant(QLatin1String("Image Cached")));
        else if (ev->pixmapEventType == PixmapLoadingStarted)
            res.insert(QLatin1String("title"), QVariant(QLatin1String("Image Loaded")));
        result << res;
    }

    if (ev->pixmapEventType != PixmapCacheCountChanged) {
        d->addVP(result, tr("Duration"), ev->duration );
    }

    {
        QVariantMap res;
        res.insert(tr("File"), QVariant(getFilenameOnly(d->pixmapUrls[ev->urlIndex])));
        result << res;
    }

    {
        QVariantMap res;
        res.insert(tr("Width"), QVariant(QString::fromLatin1("%1 px").arg(d->pixmapSizes[ev->urlIndex].first)));
        result << res;
        res.clear();
        res.insert(tr("Height"), QVariant(QString::fromLatin1("%1 px").arg(d->pixmapSizes[ev->urlIndex].second)));
        result << res;
    }

    if (ev->pixmapEventType == PixmapLoadingStarted && ev->cacheSize == -1) {
        QVariantMap res;
        res.insert(tr("Result"), QVariant(QLatin1String("Load Error")));
        result << res;
    }

    return result;
}

const QVariantMap PixmapCacheModel::getEventLocation(int /*index*/) const
{
    QVariantMap map;
    return map;
}

int PixmapCacheModel::getEventIdForHash(const QString &/*eventHash*/) const
{
    return -1;
}

int PixmapCacheModel::getEventIdForLocation(const QString &/*filename*/, int /*line*/, int /*column*/) const
{
    return -1;
}

bool compareStartTimes(const PixmapCacheModel::PixmapCacheEvent&t1, const PixmapCacheModel::PixmapCacheEvent &t2)
{
    return t1.startTime < t2.startTime;
}

void PixmapCacheModel::loadData()
{
    clear();
    QmlProfilerSimpleModel *simpleModel = m_modelManager->simpleModel();
    if (simpleModel->isEmpty())
        return;

    int lastCacheSizeEvent = -1;
    int cumulatedCount = 0;
    QVector < int > pixmapStartPoints;

    foreach (const QmlProfilerSimpleModel::QmlEventData &event, simpleModel->getEvents()) {
        if (!eventAccepted(event))
            continue;

        PixmapCacheEvent newEvent;
        newEvent.pixmapEventType = event.bindingType;
        newEvent.startTime = event.startTime;
        newEvent.duration = 0;

        bool isNewEntry = false;
        newEvent.urlIndex = d->pixmapUrls.indexOf(event.location.filename);
        if (newEvent.urlIndex == -1) {
            isNewEntry = true;
            newEvent.urlIndex = d->pixmapUrls.count();
            d->pixmapUrls << event.location.filename;
            d->pixmapSizes << QPair<int, int>(0,0); // default value
            pixmapStartPoints << d->eventList.count(); // index to the starting point
        }

        if (newEvent.pixmapEventType == PixmapSizeKnown) { // pixmap size
            d->pixmapSizes[newEvent.urlIndex] = QPair<int,int>((int)event.numericData1, (int)event.numericData2);
        }

        newEvent.eventId = newEvent.urlIndex + 1;

        // Cache Size Changed Event
        if (newEvent.pixmapEventType == PixmapCacheCountChanged) {
            newEvent.startTime = event.startTime + 1; // delay 1 ns for proper sorting
            newEvent.eventId = 0;
            newEvent.rowNumberExpanded = 1;
            newEvent.rowNumberCollapsed = 1;

            qint64 pixSize = d->pixmapSizes[newEvent.urlIndex].first * d->pixmapSizes[newEvent.urlIndex].second;
            qint64 prevSize = 0;
            if (lastCacheSizeEvent != -1) {
                prevSize = d->eventList[lastCacheSizeEvent].cacheSize;
                if (event.numericData3 < cumulatedCount)
                    pixSize = -pixSize;
                cumulatedCount = event.numericData3;

                d->eventList[lastCacheSizeEvent].duration = newEvent.startTime - d->eventList[lastCacheSizeEvent].startTime;
            }
            newEvent.cacheSize = prevSize + pixSize;
            d->eventList << newEvent;
            lastCacheSizeEvent = d->eventList.count() - 1;
        }

        // Load
        if (newEvent.pixmapEventType == PixmapLoadingStarted) {
            pixmapStartPoints[newEvent.urlIndex] = d->eventList.count();
            newEvent.rowNumberExpanded = newEvent.urlIndex + 2;
            d->eventList << newEvent;
        }

        if (newEvent.pixmapEventType == PixmapLoadingFinished || newEvent.pixmapEventType == PixmapLoadingError) {
            int loadIndex = pixmapStartPoints[newEvent.urlIndex];
            if (!isNewEntry) {
                d->eventList[loadIndex].duration = event.startTime - d->eventList[loadIndex].startTime;
            } else {
                // if it's a new entry it means that we don't have a corresponding start
                newEvent.pixmapEventType = PixmapLoadingStarted;
                newEvent.rowNumberExpanded = newEvent.urlIndex + 2;
                newEvent.startTime = traceStartTime();
                newEvent.duration = event.startTime - traceStartTime();
                d->eventList << newEvent;
            }
            if (event.bindingType == PixmapLoadingFinished)
                d->eventList[loadIndex].cacheSize = 1;  // use count to mark success
            else
                d->eventList[loadIndex].cacheSize = -1; // ... or failure
        }

        m_modelManager->modelProxyCountUpdated(m_modelId, d->eventList.count(), 2*simpleModel->getEvents().count());
    }

    if (lastCacheSizeEvent != -1) {
        d->eventList[lastCacheSizeEvent].duration = traceEndTime() - d->eventList[lastCacheSizeEvent].startTime;
    }

    d->resizeUnfinishedLoads();

    qSort(d->eventList.begin(), d->eventList.end(), compareStartTimes);

    d->computeCacheSizes();
    d->flattenLoads();
    d->computeRowCounts();

    m_modelManager->modelProxyCountUpdated(m_modelId, 1, 1);
}

void PixmapCacheModel::clear()
{
    d->eventList.clear();
    d->pixmapUrls.clear();
    d->pixmapSizes.clear();
    d->collapsedRowCount = 1;
    d->expandedRowCount = 1;
    d->isExpanded = false;

    m_modelManager->modelProxyCountUpdated(m_modelId, 0, 1);
}

void PixmapCacheModel::dataChanged()
{
    if (m_modelManager->state() == QmlProfilerDataState::Done)
        loadData();

    if (m_modelManager->state() == QmlProfilerDataState::Empty)
        clear();

    emit stateChanged();
    emit dataAvailable();
    emit emptyChanged();
    emit expandedChanged();
}

void PixmapCacheModel::PixmapCacheModelPrivate::computeCacheSizes()
{
    minCacheSize = -1;
    maxCacheSize = -1;
    foreach (const PixmapCacheModel::PixmapCacheEvent &event, eventList) {
        if (event.pixmapEventType == PixmapCacheModel::PixmapCacheCountChanged) {
            if (minCacheSize == -1 || event.cacheSize < minCacheSize)
                minCacheSize = event.cacheSize;
            if (maxCacheSize == -1 || event.cacheSize > maxCacheSize)
                maxCacheSize = event.cacheSize;
        }
    }
}

void PixmapCacheModel::PixmapCacheModelPrivate::resizeUnfinishedLoads()
{
    // all the "load start" events with duration 0 continue till the end of the trace
    for (int i = 0; i < eventList.count(); i++) {
        if (eventList[i].pixmapEventType == PixmapCacheModel::PixmapLoadingStarted &&
                eventList[i].duration == 0) {
            eventList[i].duration = q->traceEndTime() - eventList[i].startTime;
        }
    }
}

void PixmapCacheModel::PixmapCacheModelPrivate::flattenLoads()
{
    // computes "compressed row"
    QVector <qint64> eventEndTimes;
    for (int i = 0; i < eventList.count(); i++) {
        PixmapCacheModel::PixmapCacheEvent *event = &eventList[i];
        if (event->pixmapEventType == PixmapCacheModel::PixmapLoadingStarted) {
            event->rowNumberCollapsed = 0;
            while (eventEndTimes.count() > event->rowNumberCollapsed &&
                   eventEndTimes[event->rowNumberCollapsed] > event->startTime)
                event->rowNumberCollapsed++;

            if (eventEndTimes.count() == event->rowNumberCollapsed)
                eventEndTimes << 0; // increase stack length, proper value added below
            eventEndTimes[event->rowNumberCollapsed] = event->startTime + event->duration;

            // readjust to account for category empty row and bargraph
            event->rowNumberCollapsed += 2;
        }
    }
}

void PixmapCacheModel::PixmapCacheModelPrivate::computeRowCounts()
{
    expandedRowCount = 0;
    collapsedRowCount = 0;
    foreach (const PixmapCacheModel::PixmapCacheEvent &event, eventList) {
        if (event.rowNumberExpanded > expandedRowCount)
            expandedRowCount = event.rowNumberExpanded;
        if (event.rowNumberCollapsed > collapsedRowCount)
            collapsedRowCount = event.rowNumberCollapsed;
    }

    // Starting from 0, count is maxIndex+1
    expandedRowCount++;
    collapsedRowCount++;
}



} // namespace Internal
} // namespace QmlProfilerExtension
