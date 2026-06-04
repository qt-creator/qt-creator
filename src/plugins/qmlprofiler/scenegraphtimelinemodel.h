// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"
#include "qmlprofilermodelmanager.h"

#include <QStringList>
#include <QColor>

namespace QmlProfiler::Internal {

class SceneGraphTimelineModel final : public QmlProfilerTimelineModel
{
public:
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
        TextureDeletion,
        MaximumTextureStage,

        MaximumSceneGraphStage = MaximumTextureStage
    };

    struct Item {
        Item(int typeId = -1, int glyphCount = -1);
        int typeId;
        int rowNumberCollapsed;
        int glyphCount; // only used for one event type
    };

    SceneGraphTimelineModel(QmlProfilerModelManager *manager,
                            Timeline::TimelineModelAggregator *parent);

    int expandedRow(int index) const final;
    int collapsedRow(int index) const final;
    int typeId(int index) const final;
    QRgb color(int index) const final;

    QVariantList labels() const final;

    QVariantMap details(int index) const final;

private:
    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type) final;
    void finalize() final;
    void clear() final;

    void flattenLoads();
    qint64 insert(qint64 start, qint64 duration, int typeIndex, SceneGraphStage stage,
                  int glyphCount = -1);
    static const char *threadLabel(SceneGraphStage stage);

    QList<Item> m_data;
};

} // namespace QmlProfiler::Internal
