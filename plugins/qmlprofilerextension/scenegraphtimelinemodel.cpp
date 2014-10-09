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
#include "qmlprofiler/abstracttimelinemodel_p.h"

#include <QCoreApplication>
#include <QDebug>

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

static const char *ThreadLabels[] = {
    QT_TRANSLATE_NOOP("MainView", "GUI Thread"),
    QT_TRANSLATE_NOOP("MainView", "Render Thread"),
    QT_TRANSLATE_NOOP("MainView", "Render Thread Details")
};

static const char *StageLabels[] = {
    QT_TRANSLATE_NOOP("MainView", "Polish"),
    QT_TRANSLATE_NOOP("MainView", "Wait"),
    QT_TRANSLATE_NOOP("MainView", "GUI Thread Sync"),
    QT_TRANSLATE_NOOP("MainView", "Animations"),
    QT_TRANSLATE_NOOP("MainView", "Render Thread Sync"),
    QT_TRANSLATE_NOOP("MainView", "Render"),
    QT_TRANSLATE_NOOP("MainView", "Swap"),
    QT_TRANSLATE_NOOP("MainView", "Render Preprocess"),
    QT_TRANSLATE_NOOP("MainView", "Render Update"),
    QT_TRANSLATE_NOOP("MainView", "Render Bind"),
    QT_TRANSLATE_NOOP("MainView", "Render Render"),
    QT_TRANSLATE_NOOP("MainView", "Material Compile"),
    QT_TRANSLATE_NOOP("MainView", "Glyph Render"),
    QT_TRANSLATE_NOOP("MainView", "Glyph Upload"),
    QT_TRANSLATE_NOOP("MainView", "Texture Bind"),
    QT_TRANSLATE_NOOP("MainView", "Texture Convert"),
    QT_TRANSLATE_NOOP("MainView", "Texture Swizzle"),
    QT_TRANSLATE_NOOP("MainView", "Texture Upload"),
    QT_TRANSLATE_NOOP("MainView", "Texture Mipmap")
};

enum SceneGraphCategoryType {
    SceneGraphGUIThread,
    SceneGraphRenderThread,
    SceneGraphRenderThreadDetails,

    MaximumSceneGraphCategoryType
};

enum SceneGraphStage {
    MinimumSceneGraphStage = 0,
    Polish = MinimumSceneGraphStage,
    Wait,
    GUIThreadSync,
    Animations,
    MaximumGUIThreadStage,

    RenderThreadSync = MaximumGUIThreadStage,
    Render,
    Swap,
    MaximumRenderThreadStage,

    RenderPreprocess = MaximumRenderThreadStage,
    RenderUpdate,
    RenderBind,
    RenderRender,
    MaximumRenderStage,

    Material = MaximumRenderStage,
    MaximumMaterialStage,

    GlyphRender = MaximumMaterialStage,
    GlyphStore,
    MaximumGlyphStage,

    TextureBind = MaximumGlyphStage,
    TextureConvert,
    TextureSwizzle,
    TextureUpload,
    TextureMipmap,
    MaximumTextureStage,

    MaximumSceneGraphStage = MaximumTextureStage
};


Q_STATIC_ASSERT(sizeof(StageLabels) == MaximumSceneGraphStage * sizeof(const char *));

class SceneGraphTimelineModel::SceneGraphTimelineModelPrivate :
        public AbstractTimelineModel::AbstractTimelineModelPrivate
{
public:
    void flattenLoads();

    QVector<SceneGraphEvent> data;
    qint64 insert(qint64 start, qint64 duration, int typeIndex, SceneGraphStage stage,
                  int glyphCount = -1);
    static const char *threadLabel(SceneGraphStage stage);

private:
    Q_DECLARE_PUBLIC(SceneGraphTimelineModel)
};

SceneGraphTimelineModel::SceneGraphTimelineModel(QObject *parent)
    : AbstractTimelineModel(new SceneGraphTimelineModelPrivate,
                            tr(QmlProfilerModelManager::featureName(QmlDebug::ProfileSceneGraph)),
                            QmlDebug::SceneGraphFrame, QmlDebug::MaximumRangeType, parent)
{
}

quint64 SceneGraphTimelineModel::features() const
{
    return 1 << QmlDebug::ProfileSceneGraph;
}

int SceneGraphTimelineModel::row(int index) const
{
    Q_D(const SceneGraphTimelineModel);
    return expanded() ? (d->data[index].stage + 1) : d->data[index].rowNumberCollapsed;
}

int SceneGraphTimelineModel::selectionId(int index) const
{
    Q_D(const SceneGraphTimelineModel);
    return d->data[index].stage;
}

QColor SceneGraphTimelineModel::color(int index) const
{
    return colorBySelectionId(index);
}

QVariantList SceneGraphTimelineModel::labels() const
{
    Q_D(const SceneGraphTimelineModel);
    QVariantList result;

    if (d->expanded && !d->hidden && !isEmpty()) {
        for (SceneGraphStage i = MinimumSceneGraphStage; i < MaximumSceneGraphStage;
             i = static_cast<SceneGraphStage>(i + 1)) {
            QVariantMap element;
            element.insert(QLatin1String("displayName"), tr(d->threadLabel(i)));
            element.insert(QLatin1String("description"), tr(StageLabels[i]));
            element.insert(QLatin1String("id"), i);
            result << element;
        }
    }

    return result;
}

QVariantMap SceneGraphTimelineModel::details(int index) const
{
    Q_D(const SceneGraphTimelineModel);
    QVariantMap result;
    const SceneGraphEvent *ev = &d->data[index];

    result.insert(QLatin1String("displayName"),
                  tr(d->threadLabel(static_cast<SceneGraphStage>(ev->stage))));
    result.insert(tr("Stage"), tr(StageLabels[ev->stage]));
    result.insert(tr("Duration"), QmlProfilerBaseModel::formatTime(range(index).duration));
    if (ev->glyphCount >= 0)
        result.insert(tr("Glyphs"), QString::number(ev->glyphCount));

    return result;
}

void SceneGraphTimelineModel::loadData()
{
    Q_D(SceneGraphTimelineModel);
    clear();
    QmlProfilerDataModel *simpleModel = d->modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    // combine the data of several eventtypes into two rows
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        const QmlProfilerDataModel::QmlEventTypeData &type = types[event.typeIndex];
        if (!accepted(type))
            continue;

        switch ((QmlDebug::SceneGraphFrameType)type.detailType) {
        case QmlDebug::SceneGraphRendererFrame: {
            // Breakdown of render times. We repeat "render" here as "net" render time. It would
            // look incomplete if that was left out as the printf profiler lists it, too, and people
            // are apparently comparing that. Unfortunately it is somewhat redundant as the other
            // parts of the breakdown are usually very short.
            qint64 startTime = event.startTime - event.numericData1 - event.numericData2 -
                    event.numericData3 - event.numericData4;
            startTime += d->insert(startTime, event.numericData1, event.typeIndex,
                                   RenderPreprocess);
            startTime += d->insert(startTime, event.numericData2, event.typeIndex, RenderUpdate);
            startTime += d->insert(startTime, event.numericData3, event.typeIndex, RenderBind);
            d->insert(startTime, event.numericData4, event.typeIndex, RenderRender);
            break;
        }
        case QmlDebug::SceneGraphAdaptationLayerFrame: {
            qint64 startTime = event.startTime - event.numericData2 - event.numericData3;
            startTime += d->insert(startTime, event.numericData2, event.typeIndex, GlyphRender,
                                   event.numericData1);
            d->insert(startTime, event.numericData3, event.typeIndex, GlyphStore,
                      event.numericData1);
            break;
        }
        case QmlDebug::SceneGraphContextFrame: {
            d->insert(event.startTime - event.numericData1, event.numericData1, event.typeIndex,
                      Material);
            break;
        }
        case QmlDebug::SceneGraphRenderLoopFrame: {
            qint64 startTime = event.startTime - event.numericData1 - event.numericData2 -
                    event.numericData3;
            startTime += d->insert(startTime, event.numericData1, event.typeIndex,
                                   RenderThreadSync);
            startTime += d->insert(startTime, event.numericData2, event.typeIndex,
                                   Render);
            d->insert(startTime, event.numericData3, event.typeIndex, Swap);
            break;
        }
        case QmlDebug::SceneGraphTexturePrepare: {
            qint64 startTime = event.startTime - event.numericData1 - event.numericData2 -
                    event.numericData3 - event.numericData4 - event.numericData5;
            startTime += d->insert(startTime, event.numericData1, event.typeIndex, TextureBind);
            startTime += d->insert(startTime, event.numericData2, event.typeIndex, TextureConvert);
            startTime += d->insert(startTime, event.numericData3, event.typeIndex, TextureSwizzle);
            startTime += d->insert(startTime, event.numericData4, event.typeIndex, TextureUpload);
            d->insert(startTime, event.numericData5, event.typeIndex, TextureMipmap);
            break;
        }
        case QmlDebug::SceneGraphPolishAndSync: {
            qint64 startTime = event.startTime - event.numericData1 - event.numericData2 -
                    event.numericData3 - event.numericData4;

            startTime += d->insert(startTime, event.numericData1, event.typeIndex, Polish);
            startTime += d->insert(startTime, event.numericData2, event.typeIndex, Wait);
            startTime += d->insert(startTime, event.numericData3, event.typeIndex, GUIThreadSync);
            d->insert(startTime, event.numericData4, event.typeIndex, Animations);
            break;
        }
        case QmlDebug::SceneGraphWindowsAnimations: {
            // GUI thread, separate animations stage
            d->insert(event.startTime - event.numericData1, event.numericData1, event.typeIndex,
                      Animations);
            break;
        }
        case QmlDebug::SceneGraphPolishFrame: {
            // GUI thread, separate polish stage
            d->insert(event.startTime - event.numericData1, event.numericData1, event.typeIndex,
                      Polish);
            break;
        }
        default: break;
        }

        d->modelManager->modelProxyCountUpdated(d->modelId, count(), simpleModel->getEvents().count());
    }

    computeNesting();
    d->flattenLoads();
    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void SceneGraphTimelineModel::SceneGraphTimelineModelPrivate::flattenLoads()
{
    Q_Q(SceneGraphTimelineModel);
    collapsedRowCount = 0;

    // computes "compressed row"
    QVector <qint64> eventEndTimes;

    for (int i = 0; i < q->count(); i++) {
        SceneGraphEvent &event = data[i];
        const Range &start = q->range(i);
        // Don't try to put render thread events in GUI row and vice versa.
        // Rows below those are free for all.
        if (event.stage < MaximumGUIThreadStage)
            event.rowNumberCollapsed = SceneGraphGUIThread;
        else if (event.stage < MaximumRenderThreadStage)
            event.rowNumberCollapsed = SceneGraphRenderThread;
        else
            event.rowNumberCollapsed = SceneGraphRenderThreadDetails;

        while (eventEndTimes.count() > event.rowNumberCollapsed &&
               eventEndTimes[event.rowNumberCollapsed] > start.start)
            ++event.rowNumberCollapsed;

        while (eventEndTimes.count() <= event.rowNumberCollapsed)
            eventEndTimes << 0; // increase stack length, proper value added below
        eventEndTimes[event.rowNumberCollapsed] = start.start + start.duration;

        // readjust to account for category empty row
        event.rowNumberCollapsed++;
        if (event.rowNumberCollapsed > collapsedRowCount)
            collapsedRowCount = event.rowNumberCollapsed;
    }

    // Starting from 0, count is maxIndex+1
    collapsedRowCount++;
    expandedRowCount = MaximumSceneGraphStage + 1;
}

/*!
 * Inserts an event characterized by \a start time, \a duration, \a typeIndex, \a stage and possibly
 * \a glyphCount (if it's a \c GlyphRender or \c GlyphStore event) into the scene graph model if its
 * \a duration is greater than 0. Returns \a duration in that case; otherwise returns 0.
 */
qint64 SceneGraphTimelineModel::SceneGraphTimelineModelPrivate::insert(qint64 start,
        qint64 duration, int typeIndex, SceneGraphStage stage, int glyphCount)
{
    if (duration <= 0)
        return 0;

    Q_Q(SceneGraphTimelineModel);
    data.insert(q->insert(start, duration, typeIndex), SceneGraphEvent(stage, glyphCount));
    return duration;
}

const char *SceneGraphTimelineModel::SceneGraphTimelineModelPrivate::threadLabel(
        SceneGraphStage stage)
{
    if (stage < MaximumGUIThreadStage)
        return ThreadLabels[SceneGraphGUIThread];
    else if (stage < MaximumRenderThreadStage)
        return ThreadLabels[SceneGraphRenderThread];
    else
        return ThreadLabels[SceneGraphRenderThreadDetails];

}

void SceneGraphTimelineModel::clear()
{
    Q_D(SceneGraphTimelineModel);
    d->collapsedRowCount = 1;
    d->data.clear();
    AbstractTimelineModel::clear();
}

SceneGraphTimelineModel::SceneGraphEvent::SceneGraphEvent(int stage, int glyphCount) :
    stage(stage), rowNumberCollapsed(-1), glyphCount(glyphCount)
{
}

} // namespace Internal
} // namespace QmlProfilerExtension
