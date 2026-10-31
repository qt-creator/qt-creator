// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "scenegraphtimelinemodel.h"

#include <qmldebug/qmlprofilereventtypes.h>
#include <tracing/timelineformatdata.h>

#include <QCoreApplication>
#include <QDebug>

using namespace QmlDebug;
using namespace std::chrono;

namespace Profiler::Internal {

static const char *ThreadLabels[] = {
    QT_TRANSLATE_NOOP("QtC::Profiler", "GUI Thread"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Render Thread"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Render Thread Details")
};

static const char *StageLabels[] = {
    QT_TRANSLATE_NOOP("QtC::Profiler", "Polish"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Wait"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "GUI Thread Sync"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Animations"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Render Thread Sync"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Render"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Swap"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Render Preprocess"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Render Update"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Render Bind"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Render Render"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Material Compile"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Glyph Render"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Glyph Upload"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Texture Bind"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Texture Convert"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Texture Swizzle"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Texture Upload"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Texture Mipmap"),
    QT_TRANSLATE_NOOP("QtC::Profiler", "Texture Delete")
};

enum SceneGraphCategoryType {
    SceneGraphGUIThread,
    SceneGraphRenderThread,
    SceneGraphRenderThreadDetails,

    MaximumSceneGraphCategoryType
};

Q_STATIC_ASSERT(sizeof(StageLabels) ==
                SceneGraphTimelineModel::MaximumSceneGraphStage * sizeof(const char *));

SceneGraphTimelineModel::SceneGraphTimelineModel(QmlProfilerModelManager *manager,
                                                 Timeline::TimelineModelAggregator *parent) :
    QmlProfilerTimelineModel(manager, SceneGraphFrame, UndefinedRangeType, ProfileSceneGraph, parent)
{
}

int SceneGraphTimelineModel::expandedRow(int index) const
{
    return selectionId(index) + 1;
}

int SceneGraphTimelineModel::collapsedRow(int index) const
{
    return m_data[index].rowNumberCollapsed;
}

int SceneGraphTimelineModel::typeId(int index) const
{
    return m_data[index].typeId;
}

QRgb SceneGraphTimelineModel::color(int index) const
{
    return colorBySelectionId(index);
}

Timeline::RowLabels SceneGraphTimelineModel::labels() const
{
    Timeline::RowLabels result;

    for (SceneGraphStage i = MinimumSceneGraphStage; i < MaximumSceneGraphStage;
         i = static_cast<SceneGraphStage>(i + 1)) {
        result.append({Tr::tr(StageLabels[i]), i});
    }

    return result;
}

Timeline::ItemDetails SceneGraphTimelineModel::details(int index) const
{
    Timeline::ItemDetails result;
    const SceneGraphStage stage = static_cast<SceneGraphStage>(selectionId(index));

    result.insert(QLatin1String("displayName"), Tr::tr(threadLabel(stage)));
    result.insert(Tr::tr("Stage"), Tr::tr(StageLabels[stage]));
    result.insert(Tr::tr("Duration"), Timeline::formatTime(duration(index)));

    const int glyphCount = m_data[index].glyphCount;
    if (glyphCount >= 0)
        result.insert(Tr::tr("Glyphs"), QString::number(glyphCount));

    return result;
}

void SceneGraphTimelineModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    const auto number = [&event](int i) { return nanoseconds(event.number<qint64>(i)); };

    // combine the data of several eventtypes into two rows
    switch ((SceneGraphFrameType)type.detailType()) {
    case SceneGraphRendererFrame: {
        // Breakdown of render times. We repeat "render" here as "net" render time. It would
        // look incomplete if that was left out as the printf profiler lists it, too, and people
        // are apparently comparing that. Unfortunately it is somewhat redundant as the other
        // parts of the breakdown are usually very short.
        nanoseconds startTime = event.timestamp() - number(0) - number(1) - number(2) - number(3);
        startTime += insert(startTime, number(0), event.typeIndex(),
                            RenderPreprocess);
        startTime += insert(startTime, number(1), event.typeIndex(), RenderUpdate);
        startTime += insert(startTime, number(2), event.typeIndex(), RenderBind);
        insert(startTime, number(3), event.typeIndex(), RenderRender);
        break;
    }
    case SceneGraphAdaptationLayerFrame: {
        nanoseconds startTime = event.timestamp() - number(1) - number(2);
        startTime += insert(startTime, number(1), event.typeIndex(), GlyphRender,
                            event.number<qint64>(0));
        insert(startTime, number(2), event.typeIndex(), GlyphStore,
               event.number<qint64>(0));
        break;
    }
    case SceneGraphContextFrame: {
        insert(event.timestamp() - number(0), number(0),
               event.typeIndex(), Material);
        break;
    }
    case SceneGraphRenderLoopFrame: {
        nanoseconds startTime = event.timestamp() - number(0) - number(1) -
                number(2);
        startTime += insert(startTime, number(0), event.typeIndex(),
                               RenderThreadSync);
        startTime += insert(startTime, number(1), event.typeIndex(),
                               Render);
        insert(startTime, number(2), event.typeIndex(), Swap);
        break;
    }
    case SceneGraphTexturePrepare: {
        nanoseconds startTime = event.timestamp() - number(0) - number(1) -
                number(2) - number(3) - number(4);
        startTime += insert(startTime, number(0), event.typeIndex(), TextureBind);
        startTime += insert(startTime, number(1), event.typeIndex(), TextureConvert);
        startTime += insert(startTime, number(2), event.typeIndex(), TextureSwizzle);
        startTime += insert(startTime, number(3), event.typeIndex(), TextureUpload);
        insert(startTime, number(4), event.typeIndex(), TextureMipmap);
        break;
    }
    case SceneGraphTextureDeletion: {
        insert(event.timestamp() - number(0), number(0),
               event.typeIndex(), TextureDeletion);
        break;
    }
    case SceneGraphPolishAndSync: {
        nanoseconds startTime = event.timestamp() - number(0) - number(1) -
                number(2) - number(3);

        startTime += insert(startTime, number(0), event.typeIndex(), Polish);
        startTime += insert(startTime, number(1), event.typeIndex(), Wait);
        startTime += insert(startTime, number(2), event.typeIndex(), GUIThreadSync);
        insert(startTime, number(3), event.typeIndex(), Animations);
        break;
    }
    case SceneGraphWindowsAnimations: {
        // GUI thread, separate animations stage
        insert(event.timestamp() - number(0), number(0),
               event.typeIndex(), Animations);
        break;
    }
    case SceneGraphPolishFrame: {
        // GUI thread, separate polish stage
        insert(event.timestamp() - number(0), number(0),
               event.typeIndex(), Polish);
        break;
    }
    default: break;
    }
}

void SceneGraphTimelineModel::prepare()
{
    computeNesting();
    flattenLoads();
}

void SceneGraphTimelineModel::flattenLoads()
{
    int collapsedRowCount = Timeline::kCollapsedStartRow;

    // computes "compressed row"
    QList<nanoseconds> eventEndTimes;

    for (int i = 0; i < count(); i++) {
        Item &event = m_data[i];
        int stage = selectionId(i);
        // Don't try to put render thread events in GUI row and vice versa.
        // Rows below those are free for all.
        if (stage < MaximumGUIThreadStage)
            event.rowNumberCollapsed = SceneGraphGUIThread;
        else if (stage < MaximumRenderThreadStage)
            event.rowNumberCollapsed = SceneGraphRenderThread;
        else
            event.rowNumberCollapsed = SceneGraphRenderThreadDetails;

        while (eventEndTimes.count() > event.rowNumberCollapsed &&
               eventEndTimes[event.rowNumberCollapsed] > startTime(i))
            ++event.rowNumberCollapsed;

        while (eventEndTimes.count() <= event.rowNumberCollapsed)
            eventEndTimes << 0ns; // increase stack length, proper value added below
        eventEndTimes[event.rowNumberCollapsed] = endTime(i);

        // readjust to account for the start row
        event.rowNumberCollapsed += Timeline::kCollapsedStartRow;
        if (event.rowNumberCollapsed > collapsedRowCount)
            collapsedRowCount = event.rowNumberCollapsed;
    }

    // Starting from 0, count is maxIndex+1
    setCollapsedRowCount(collapsedRowCount + 1);
    setExpandedRowCount(MaximumSceneGraphStage + 1);
}

/*!
 * Inserts an event characterized by \a start time, \a duration, \a typeIndex, \a stage and possibly
 * \a glyphCount (if it's a \c GlyphRender or \c GlyphStore event) into the scene graph model if its
 * \a duration is greater than 0. Returns \a duration in that case; otherwise returns 0.
 */
nanoseconds SceneGraphTimelineModel::insert(nanoseconds start, nanoseconds duration,
                                            int typeIndex, SceneGraphStage stage, int glyphCount)
{
    if (duration <= 0ns)
        return 0ns;

    m_data.insert(QmlProfilerTimelineModel::insert(start, duration, stage),
                  Item(typeIndex, glyphCount));
    return duration;
}

const char *SceneGraphTimelineModel::threadLabel(SceneGraphStage stage)
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
    m_data.clear();
    QmlProfilerTimelineModel::clear();
}

SceneGraphTimelineModel::Item::Item(int typeId, int glyphCount) :
    typeId(typeId), rowNumberCollapsed(-1), glyphCount(glyphCount)
{
}

} // namespace Profiler::Internal
