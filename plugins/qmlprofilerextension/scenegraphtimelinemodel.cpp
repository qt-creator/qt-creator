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

#include "scenegraphtimelinemodel.h"
#include "qmldebug/qmlprofilereventtypes.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"

#include <QCoreApplication>
#include <QDebug>

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler::Internal;

enum SceneGraphEventType {
    SceneGraphRendererFrame,
    SceneGraphAdaptationLayerFrame,
    SceneGraphContextFrame,
    SceneGraphRenderLoopFrame,
    SceneGraphTexturePrepare,
    SceneGraphTextureDeletion,
    SceneGraphPolishAndSync,
    SceneGraphWindowsRenderShow,
    SceneGraphWindowsAnimations,
    SceneGraphWindowsPolishFrame,

    MaximumSceneGraphFrameType
};

enum SceneGraphCategoryType {
    SceneGraphRenderThread,
    SceneGraphGUIThread,

    MaximumSceneGraphCategoryType
};

class SceneGraphTimelineModel::SceneGraphTimelineModelPrivate {
public:
    SceneGraphTimelineModelPrivate(SceneGraphTimelineModel *qq):q(qq) {}
    ~SceneGraphTimelineModelPrivate();

    SceneGraphTimelineModel *q;

    QVector < SceneGraphTimelineModel::SceneGraphEvent > eventList;
    bool isExpanded;
    QString displayTime(double time);
    void addVP(QVariantList &l, QString label, qint64 time);
};

SceneGraphTimelineModel::SceneGraphTimelineModel(QObject *parent)
    : AbstractTimelineModel(parent), d(new SceneGraphTimelineModelPrivate(this))
{
}

SceneGraphTimelineModel::~SceneGraphTimelineModel()
{
}

int SceneGraphTimelineModel::categories() const
{
    return 1;
}

QStringList SceneGraphTimelineModel::categoryTitles() const
{
    QStringList retString;
    retString << categoryLabel(0);
    return retString;
}

QString SceneGraphTimelineModel::name() const
{
    return QLatin1String("SceneGraphTimeLineModel");
}

int SceneGraphTimelineModel::count() const
{
    return d->eventList.count();
}

bool SceneGraphTimelineModel::isEmpty() const
{
    return d->eventList.isEmpty();
}

bool SceneGraphTimelineModel::eventAccepted(const QmlProfiler::Internal::QmlProfilerSimpleModel::QmlEventData &event) const
{
    return (event.eventType == QmlDebug::SceneGraphFrameEvent);
}

qint64 SceneGraphTimelineModel::lastTimeMark() const
{
    return d->eventList.last().startTime;
}

bool SceneGraphTimelineModel::expanded(int ) const
{
    return d->isExpanded;
}

void SceneGraphTimelineModel::setExpanded(int category, bool expanded)
{
    Q_UNUSED(category);
    d->isExpanded = expanded;
}

int SceneGraphTimelineModel::categoryDepth(int categoryIndex) const
{
    Q_UNUSED(categoryIndex);
    if (isEmpty())
        return 1;
    return 3;
}

int SceneGraphTimelineModel::categoryCount() const
{
    return 1;
}

const QString SceneGraphTimelineModel::categoryLabel(int categoryIndex) const
{
    Q_UNUSED(categoryIndex);
    return tr("Scene Graph");
}

int SceneGraphTimelineModel::findFirstIndex(qint64 startTime) const
{
    int candidate = findFirstIndexNoParents(startTime);
    // because there's two threads synchronized, the right index could be one off
    if (candidate > 0 && d->eventList[candidate-1].startTime + d->eventList[candidate-1].duration >= startTime)
        return candidate - 1;

    return candidate;
}

int SceneGraphTimelineModel::findFirstIndexNoParents(qint64 startTime) const
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

int SceneGraphTimelineModel::findLastIndex(qint64 endTime) const
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

int SceneGraphTimelineModel::getEventType(int index) const
{
    Q_UNUSED(index);
    return QmlDebug::SceneGraphFrameEvent;
}

int SceneGraphTimelineModel::getEventCategory(int index) const
{
    Q_UNUSED(index);
    return 0;
}

int SceneGraphTimelineModel::getEventRow(int index) const
{
    return d->eventList[index].sgEventType + 1;
    if (d->isExpanded)
        return d->eventList[index].sgEventType + 1;
    else
        return 0;
}

qint64 SceneGraphTimelineModel::getDuration(int index) const
{
    return d->eventList[index].duration;
}

qint64 SceneGraphTimelineModel::getStartTime(int index) const
{
    return d->eventList[index].startTime;
}

qint64 SceneGraphTimelineModel::getEndTime(int index) const
{
    return getStartTime(index)+getDuration(index);
}

int SceneGraphTimelineModel::getEventId(int index) const
{
    return d->eventList[index].sgEventType;
}

QColor SceneGraphTimelineModel::getColor(int index) const
{
    // get duration in seconds
    double eventDuration = getDuration(index) / 1e9;

    // supposedly never above 60 frames per second
    // limit it in that case
    if (eventDuration < 1/60.0)
        eventDuration = 1/60.0;

    // generate hue based on fraction of the 60fps
    double fpsFraction = 1 / (eventDuration * 60.0);
    if (fpsFraction > 1.0)
        fpsFraction = 1.0;
    return QColor::fromHsl((fpsFraction*96)+10, 76, 166);
}

float SceneGraphTimelineModel::getHeight(int index) const
{
    Q_UNUSED(index);
    return 1.0f;
}

QString labelForSGType(int t)
{
    switch ((SceneGraphCategoryType)t) {
    case SceneGraphRenderThread:
        return QCoreApplication::translate("SceneGraphTimelineModel", "Renderer Thread");
    case SceneGraphGUIThread:
        return QCoreApplication::translate("SceneGraphTimelineModel", "GUI Thread");
    default: return QString();
    }
}

const QVariantList SceneGraphTimelineModel::getLabelsForCategory(int category) const
{
    Q_UNUSED(category);
    QVariantList result;

    if (d->isExpanded && !isEmpty()) {
        for (int i = 0; i < MaximumSceneGraphCategoryType; i++) {
            QVariantMap element;

            element.insert(QLatin1String("displayName"), QVariant(labelForSGType(i)));
            element.insert(QLatin1String("description"), QVariant(labelForSGType(i)));
            element.insert(QLatin1String("id"), QVariant(i));
            result << element;
        }
    }

    return result;
}



QString SceneGraphTimelineModel::SceneGraphTimelineModelPrivate::displayTime(double time)
{
    if (time < 1e6)
        return QString::number(time/1e3,'f',3) + trUtf8(" \xc2\xb5s");
    if (time < 1e9)
        return QString::number(time/1e6,'f',3) + tr(" ms");

    return QString::number(time/1e9,'f',3) + tr(" s");
}

void SceneGraphTimelineModel::SceneGraphTimelineModelPrivate::addVP(QVariantList &l, QString label, qint64 time)
{
    if (time > 0) {
        QVariantMap res;
        res.insert(label, QVariant(displayTime(time)));
        l << res;
    }
}


const QVariantList SceneGraphTimelineModel::getEventDetails(int index) const
{
    QVariantList result;
    SceneGraphEvent *ev = &d->eventList[index];

    {
        QVariantMap res;
        res.insert(QLatin1String("title"), QVariant(labelForSGType(ev->sgEventType)));
        result << res;
    }

    d->addVP(result, tr("Duration"), ev->duration );

    if (ev->sgEventType == SceneGraphRenderThread) {
        d->addVP(result, tr("Polish"), ev->timing[14]);
        d->addVP(result, tr("Sync"), ev->timing[0]);
        d->addVP(result, tr("Preprocess"), ev->timing[1]);
        d->addVP(result, tr("Upload"), ev->timing[2]);
        d->addVP(result, tr("Swizzle"), ev->timing[3]);
        d->addVP(result, tr("Convert"), ev->timing[4]);
        d->addVP(result, tr("Mipmap"), ev->timing[5]);
        d->addVP(result, tr("Bind"), ev->timing[6]);
        d->addVP(result, tr("Material"), ev->timing[7]);
        d->addVP(result, tr("Glyph Render"), ev->timing[8]);
        d->addVP(result, tr("Glyph Store"), ev->timing[9]);
        d->addVP(result, tr("Update"), ev->timing[10]);
        d->addVP(result, tr("Binding"), ev->timing[11]);
        d->addVP(result, tr("Render"), ev->timing[12]);
        d->addVP(result, tr("Swap"), ev->timing[13]);
        d->addVP(result, tr("Animations"), ev->timing[15]);
    }
    if (ev->sgEventType == SceneGraphGUIThread) {
        d->addVP(result, tr("Polish"), ev->timing[0]);
        d->addVP(result, tr("Wait"), ev->timing[1]);
        d->addVP(result, tr("Sync"), ev->timing[2]);
        d->addVP(result, tr("Animations"), ev->timing[3]);
    }

    return result;
}

const QVariantMap SceneGraphTimelineModel::getEventLocation(int /*index*/) const
{
    QVariantMap map;
    return map;
}

int SceneGraphTimelineModel::getEventIdForHash(const QString &) const
{
    return -1;
}

int SceneGraphTimelineModel::getEventIdForLocation(const QString &/*filename*/, int /*line*/, int /*column*/) const
{
    return -1;
}

bool compareStartTimes(const SceneGraphTimelineModel::SceneGraphEvent&t1, const SceneGraphTimelineModel::SceneGraphEvent &t2)
{
    return t1.startTime < t2.startTime;
}

void SceneGraphTimelineModel::loadData()
{
    clear();
    QmlProfilerSimpleModel *simpleModel = m_modelManager->simpleModel();
    if (simpleModel->isEmpty())
        return;

    int lastRenderEvent = -1;

    // combine the data of several eventtypes into two rows
    foreach (const QmlProfilerSimpleModel::QmlEventData &event, simpleModel->getEvents()) {
        if (!eventAccepted(event))
            continue;

        if (event.bindingType == SceneGraphRenderLoopFrame) {
            SceneGraphEvent newEvent;
            newEvent.sgEventType = SceneGraphRenderThread;
            newEvent.duration = event.numericData1 + event.numericData2 + event.numericData3;
            newEvent.startTime = event.startTime - newEvent.duration;
            for (int i=0; i < timingFieldCount; i++)
                newEvent.timing[i] = 0;

            d->eventList << newEvent;
            lastRenderEvent = d->eventList.count()-1;
        }

        if (lastRenderEvent >= 0) {
            switch ((SceneGraphEventType)event.bindingType) {
            case SceneGraphRendererFrame: {
                d->eventList[lastRenderEvent].timing[1] = event.numericData1;
                d->eventList[lastRenderEvent].timing[10] = event.numericData2;
                d->eventList[lastRenderEvent].timing[11] = event.numericData3;
                d->eventList[lastRenderEvent].timing[12] = event.numericData4;
                break;
            }
            case SceneGraphAdaptationLayerFrame: {
                d->eventList[lastRenderEvent].timing[8] = event.numericData2;
                d->eventList[lastRenderEvent].timing[9] = event.numericData3;
                break;
            }
            case SceneGraphContextFrame: {
                d->eventList[lastRenderEvent].timing[7] = event.numericData1;
                break;
            }
            case SceneGraphRenderLoopFrame: {
                d->eventList[lastRenderEvent].timing[0] = event.numericData1;
                d->eventList[lastRenderEvent].timing[13] = event.numericData3;

                break;
            }
            case SceneGraphTexturePrepare: {
                d->eventList[lastRenderEvent].timing[2] = event.numericData4;
                d->eventList[lastRenderEvent].timing[3] = event.numericData3;
                d->eventList[lastRenderEvent].timing[4] = event.numericData2;
                d->eventList[lastRenderEvent].timing[5] = event.numericData5;
                d->eventList[lastRenderEvent].timing[6] = event.numericData1;
                break;
            }
            case SceneGraphPolishAndSync: {
                // GUI thread
                SceneGraphEvent newEvent;
                newEvent.sgEventType = SceneGraphGUIThread;
                newEvent.duration = event.numericData1 + event.numericData2 + event.numericData3 + event.numericData4;
                newEvent.startTime = event.startTime - newEvent.duration;
                for (int i=0; i < timingFieldCount; i++)
                    newEvent.timing[i] = 0;

                newEvent.timing[0] = event.numericData1;
                newEvent.timing[1] = event.numericData2;
                newEvent.timing[2] = event.numericData3;
                newEvent.timing[3] = event.numericData4;

                d->eventList << newEvent;
                break;
            }
            case SceneGraphWindowsAnimations: {
                d->eventList[lastRenderEvent].timing[14] = event.numericData1;
                break;
            }
            case SceneGraphWindowsPolishFrame: {
                d->eventList[lastRenderEvent].timing[15] = event.numericData1;
                break;
            }
            default: break;
            }
        }

        m_modelManager->modelProxyCountUpdated(m_modelId, d->eventList.count(), simpleModel->getEvents().count());
    }

    qSort(d->eventList.begin(), d->eventList.end(), compareStartTimes);
    m_modelManager->modelProxyCountUpdated(m_modelId, 1, 1);
}

void SceneGraphTimelineModel::clear()
{
    d->eventList.clear();
    d->isExpanded = false;
    m_modelManager->modelProxyCountUpdated(m_modelId, 0, 1);
}

void SceneGraphTimelineModel::dataChanged()
{
    if (m_modelManager->state() == QmlProfilerDataState::ProcessingData)
        loadData();

    if (m_modelManager->state() == QmlProfilerDataState::Empty)
        clear();

    emit stateChanged();
    emit dataAvailable();
    emit emptyChanged();
    emit expandedChanged();
}




} // namespace Internal
} // namespace QmlProfilerExtension
