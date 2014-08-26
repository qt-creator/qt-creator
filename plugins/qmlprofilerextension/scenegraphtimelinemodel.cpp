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
    "GUI Thread",
    "Render Thread"
};

static const char *StageLabels[] = {
    "Polish",
    "Wait",
    "Sync",
    "Animations",
    "Sync",
    "Swap",
    "Material Compile",
    "Render Preprocess",
    "Render Update",
    "Render Bind",
    "Render",
    "Glyph Render",
    "Glyph Upload",
    "Texture Bind",
    "Texture Convert",
    "Texture Swizzle",
    "Texture Upload",
    "Texture Mipmap"
};

enum SceneGraphCategoryType {
    SceneGraphGUIThread,
    SceneGraphRenderThread,

    MaximumSceneGraphCategoryType
};

enum SceneGraphStage {
    Polish = 0,
    Wait,
    GUIThreadSync,
    Animations,
    MaximumGUIThreadStage,

    RenderThreadSync = MaximumGUIThreadStage,
    Swap,
    Material,
    MaximumRenderThreadStage,

    RenderPreprocess = MaximumRenderThreadStage,
    RenderUpdate,
    RenderBind,
    RenderRender,
    MaximumRenderStage,

    GlyphRender = MaximumRenderStage,
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
    SceneGraphTimelineModelPrivate();
    int collapsedRowCount;
    void flattenLoads();

    QVector<SceneGraphEvent> data;
private:
    Q_DECLARE_PUBLIC(SceneGraphTimelineModel)
};

SceneGraphTimelineModel::SceneGraphTimelineModelPrivate::SceneGraphTimelineModelPrivate() :
    collapsedRowCount(1)
{
}

SceneGraphTimelineModel::SceneGraphTimelineModel(QObject *parent)
    : AbstractTimelineModel(new SceneGraphTimelineModelPrivate, tr("Scene Graph"),
                            QmlDebug::SceneGraphFrame, QmlDebug::MaximumRangeType, parent)
{
}

int SceneGraphTimelineModel::rowCount() const
{
    Q_D(const SceneGraphTimelineModel);
    if (isEmpty())
        return 1;
    return expanded() ? (MaximumSceneGraphStage + 1) : d->collapsedRowCount;
}

int SceneGraphTimelineModel::row(int index) const
{
    Q_D(const SceneGraphTimelineModel);
    return expanded() ? (d->data[index].stage + 1) : d->data[index].rowNumberCollapsed;
}

int SceneGraphTimelineModel::eventId(int index) const
{
    Q_D(const SceneGraphTimelineModel);
    return d->data[index].stage;
}

QColor SceneGraphTimelineModel::color(int index) const
{
    return colorByEventId(eventId(index));
}

QVariantList SceneGraphTimelineModel::labels() const
{
    Q_D(const SceneGraphTimelineModel);
    QVariantList result;

    if (d->expanded && !isEmpty()) {
        for (int i = 0; i < MaximumSceneGraphStage; ++i) {
            QVariantMap element;
            element.insert(QLatin1String("displayName"), tr(ThreadLabels[i < MaximumGUIThreadStage ?
                    SceneGraphGUIThread : SceneGraphRenderThread]));
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

    result.insert(QLatin1String("displayName"), tr(ThreadLabels[ev->stage < MaximumGUIThreadStage ?
                        SceneGraphGUIThread : SceneGraphRenderThread]));
    result.insert(tr("Stage"), tr(StageLabels[ev->stage]));
    result.insert(tr("Duration"), QmlProfilerBaseModel::formatTime(range(index).duration));
    if (ev->glyphCount >= 0)
        result.insert(tr("Glyph Count"), QString::number(ev->glyphCount));

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
            qint64 startTime = event.startTime - event.numericData1 - event.numericData2 -
                    event.numericData3 - event.numericData4;
            d->data.insert(insert(startTime, event.numericData1),
                           SceneGraphEvent(RenderPreprocess));
            startTime += event.numericData1;
            d->data.insert(insert(startTime, event.numericData2), SceneGraphEvent(RenderUpdate));
            startTime += event.numericData2;
            d->data.insert(insert(startTime, event.numericData3), SceneGraphEvent(RenderBind));
            startTime += event.numericData3;
            d->data.insert(insert(startTime, event.numericData4), SceneGraphEvent(RenderRender));
            break;
        }
        case QmlDebug::SceneGraphAdaptationLayerFrame: {
            qint64 startTime = event.startTime - event.numericData2 - event.numericData3;
            d->data.insert(insert(startTime, event.numericData2),
                      SceneGraphEvent(GlyphRender, event.numericData1));
            startTime += event.numericData2;
            d->data.insert(insert(startTime, event.numericData3),
                      SceneGraphEvent(GlyphStore, event.numericData1));
            break;
        }
        case QmlDebug::SceneGraphContextFrame: {
            d->data.insert(insert(event.startTime - event.numericData1, event.numericData1),
                      SceneGraphEvent(Material));
            break;
        }
        case QmlDebug::SceneGraphRenderLoopFrame: {
            qint64 startTime = event.startTime - event.numericData1 - event.numericData2 -
                    event.numericData3;
            d->data.insert(insert(startTime, event.numericData1),
                           SceneGraphEvent(RenderThreadSync));
            startTime += event.numericData1 + event.numericData2;
            // Skip actual rendering. We get a SceneGraphRendererFrame for that
            d->data.insert(insert(startTime, event.numericData3), SceneGraphEvent(Swap));
            break;
        }
        case QmlDebug::SceneGraphTexturePrepare: {
            qint64 startTime = event.startTime - event.numericData1 - event.numericData2 -
                    event.numericData3 - event.numericData4 - event.numericData5;

            d->data.insert(insert(startTime, event.numericData1), SceneGraphEvent(TextureBind));
            startTime += event.numericData1;
            d->data.insert(insert(startTime, event.numericData2), SceneGraphEvent(TextureConvert));
            startTime += event.numericData2;
            d->data.insert(insert(startTime, event.numericData3), SceneGraphEvent(TextureSwizzle));
            startTime += event.numericData3;
            d->data.insert(insert(startTime, event.numericData4), SceneGraphEvent(TextureUpload));
            startTime += event.numericData4;
            d->data.insert(insert(startTime, event.numericData4), SceneGraphEvent(TextureMipmap));
            break;
        }
        case QmlDebug::SceneGraphPolishAndSync: {
            qint64 startTime = event.startTime - event.numericData1 - event.numericData2 -
                    event.numericData3 - event.numericData4;

            d->data.insert(insert(startTime, event.numericData1), SceneGraphEvent(Polish));
            startTime += event.numericData1;
            d->data.insert(insert(startTime, event.numericData2), SceneGraphEvent(Wait));
            startTime += event.numericData2;
            d->data.insert(insert(startTime, event.numericData3), SceneGraphEvent(GUIThreadSync));
            startTime += event.numericData3;
            d->data.insert(insert(startTime, event.numericData4), SceneGraphEvent(Animations));
            break;
        }
        case QmlDebug::SceneGraphWindowsAnimations: {
            // GUI thread, separate animations stage
            d->data.insert(insert(event.startTime - event.numericData1, event.numericData1),
                           SceneGraphEvent(Animations));
            break;
        }
        case QmlDebug::SceneGraphPolishFrame: {
            // GUI thread, separate polish stage
            d->data.insert(insert(event.startTime - event.numericData1, event.numericData1),
                           SceneGraphEvent(Polish));
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
        event.rowNumberCollapsed = (event.stage < MaximumGUIThreadStage ? SceneGraphGUIThread :
                                                                          SceneGraphRenderThread);
        while (eventEndTimes.count() > event.rowNumberCollapsed &&
               eventEndTimes[event.rowNumberCollapsed] > start.start) {
            ++event.rowNumberCollapsed;
            if (event.stage < MaximumGUIThreadStage) {
                if (event.rowNumberCollapsed == SceneGraphRenderThread)
                    ++event.rowNumberCollapsed;
            } else if (event.rowNumberCollapsed == SceneGraphGUIThread) {
                ++event.rowNumberCollapsed;
            }
        }

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
