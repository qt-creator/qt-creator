/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "scenegraphtimelinemodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilereventtypes.h"

#include <timeline/timelineformattime.h>

#include <QCoreApplication>
#include <QDebug>

namespace QmlProfiler {
namespace Internal {

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
    QT_TRANSLATE_NOOP("MainView", "Texture Mipmap"),
    QT_TRANSLATE_NOOP("MainView", "Texture Delete")
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
                                                 QObject *parent) :
    QmlProfilerTimelineModel(manager, SceneGraphFrame, MaximumRangeType, ProfileSceneGraph, parent)
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

QVariantList SceneGraphTimelineModel::labels() const
{
    QVariantList result;

    for (SceneGraphStage i = MinimumSceneGraphStage; i < MaximumSceneGraphStage;
         i = static_cast<SceneGraphStage>(i + 1)) {
        QVariantMap element;
        element.insert(QLatin1String("displayName"), tr(threadLabel(i)));
        element.insert(QLatin1String("description"), tr(StageLabels[i]));
        element.insert(QLatin1String("id"), i);
        result << element;
    }

    return result;
}

QVariantMap SceneGraphTimelineModel::details(int index) const
{
    QVariantMap result;
    const SceneGraphStage stage = static_cast<SceneGraphStage>(selectionId(index));

    result.insert(QLatin1String("displayName"), tr(threadLabel(stage)));
    result.insert(tr("Stage"), tr(StageLabels[stage]));
    result.insert(tr("Duration"), Timeline::formatTime(duration(index)));

    const int glyphCount = m_data[index].glyphCount;
    if (glyphCount >= 0)
        result.insert(tr("Glyphs"), QString::number(glyphCount));

    return result;
}

void SceneGraphTimelineModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    // combine the data of several eventtypes into two rows
    switch ((SceneGraphFrameType)type.detailType()) {
    case SceneGraphRendererFrame: {
        // Breakdown of render times. We repeat "render" here as "net" render time. It would
        // look incomplete if that was left out as the printf profiler lists it, too, and people
        // are apparently comparing that. Unfortunately it is somewhat redundant as the other
        // parts of the breakdown are usually very short.
        qint64 startTime = event.timestamp() - event.number<qint64>(0) - event.number<qint64>(1) -
                event.number<qint64>(2) - event.number<qint64>(3);
        startTime += insert(startTime, event.number<qint64>(0), event.typeIndex(),
                            RenderPreprocess);
        startTime += insert(startTime, event.number<qint64>(1), event.typeIndex(), RenderUpdate);
        startTime += insert(startTime, event.number<qint64>(2), event.typeIndex(), RenderBind);
        insert(startTime, event.number<qint64>(3), event.typeIndex(), RenderRender);
        break;
    }
    case SceneGraphAdaptationLayerFrame: {
        qint64 startTime = event.timestamp() - event.number<qint64>(1) - event.number<qint64>(2);
        startTime += insert(startTime, event.number<qint64>(1), event.typeIndex(), GlyphRender,
                            event.number<qint64>(0));
        insert(startTime, event.number<qint64>(2), event.typeIndex(), GlyphStore,
               event.number<qint64>(0));
        break;
    }
    case SceneGraphContextFrame: {
        insert(event.timestamp() - event.number<qint64>(0), event.number<qint64>(0),
               event.typeIndex(), Material);
        break;
    }
    case SceneGraphRenderLoopFrame: {
        qint64 startTime = event.timestamp() - event.number<qint64>(0) - event.number<qint64>(1) -
                event.number<qint64>(2);
        startTime += insert(startTime, event.number<qint64>(0), event.typeIndex(),
                               RenderThreadSync);
        startTime += insert(startTime, event.number<qint64>(1), event.typeIndex(),
                               Render);
        insert(startTime, event.number<qint64>(2), event.typeIndex(), Swap);
        break;
    }
    case SceneGraphTexturePrepare: {
        qint64 startTime = event.timestamp() - event.number<qint64>(0) - event.number<qint64>(1) -
                event.number<qint64>(2) - event.number<qint64>(3) - event.number<qint64>(4);
        startTime += insert(startTime, event.number<qint64>(0), event.typeIndex(), TextureBind);
        startTime += insert(startTime, event.number<qint64>(1), event.typeIndex(), TextureConvert);
        startTime += insert(startTime, event.number<qint64>(2), event.typeIndex(), TextureSwizzle);
        startTime += insert(startTime, event.number<qint64>(3), event.typeIndex(), TextureUpload);
        insert(startTime, event.number<qint64>(4), event.typeIndex(), TextureMipmap);
        break;
    }
    case SceneGraphTextureDeletion: {
        insert(event.timestamp() - event.number<qint64>(0), event.number<qint64>(0),
               event.typeIndex(), TextureDeletion);
        break;
    }
    case SceneGraphPolishAndSync: {
        qint64 startTime = event.timestamp() - event.number<qint64>(0) - event.number<qint64>(1) -
                event.number<qint64>(2) - event.number<qint64>(3);

        startTime += insert(startTime, event.number<qint64>(0), event.typeIndex(), Polish);
        startTime += insert(startTime, event.number<qint64>(1), event.typeIndex(), Wait);
        startTime += insert(startTime, event.number<qint64>(2), event.typeIndex(), GUIThreadSync);
        insert(startTime, event.number<qint64>(3), event.typeIndex(), Animations);
        break;
    }
    case SceneGraphWindowsAnimations: {
        // GUI thread, separate animations stage
        insert(event.timestamp() - event.number<qint64>(0), event.number<qint64>(0),
               event.typeIndex(), Animations);
        break;
    }
    case SceneGraphPolishFrame: {
        // GUI thread, separate polish stage
        insert(event.timestamp() - event.number<qint64>(0), event.number<qint64>(0),
               event.typeIndex(), Polish);
        break;
    }
    default: break;
    }
}

void SceneGraphTimelineModel::finalize()
{
    computeNesting();
    flattenLoads();
}

void SceneGraphTimelineModel::flattenLoads()
{
    int collapsedRowCount = 0;

    // computes "compressed row"
    QVector <qint64> eventEndTimes;

    for (int i = 0; i < count(); i++) {
        SceneGraphEvent &event = m_data[i];
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
            eventEndTimes << 0; // increase stack length, proper value added below
        eventEndTimes[event.rowNumberCollapsed] = endTime(i);

        // readjust to account for category empty row
        event.rowNumberCollapsed++;
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
qint64 SceneGraphTimelineModel::insert(qint64 start, qint64 duration, int typeIndex,
                                       SceneGraphStage stage, int glyphCount)
{
    if (duration <= 0)
        return 0;

    m_data.insert(QmlProfilerTimelineModel::insert(start, duration, stage),
                  SceneGraphEvent(typeIndex, glyphCount));
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

SceneGraphTimelineModel::SceneGraphEvent::SceneGraphEvent(int typeId, int glyphCount) :
    typeId(typeId), rowNumberCollapsed(-1), glyphCount(glyphCount)
{
}

} // namespace Internal
} // namespace QmlProfiler
