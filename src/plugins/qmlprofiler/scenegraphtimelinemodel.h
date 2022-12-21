// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"
#include "qmlprofilermodelmanager.h"

#include <QStringList>
#include <QColor>

namespace QmlProfiler {
namespace Internal {

class SceneGraphTimelineModel : public QmlProfilerTimelineModel
{
    Q_OBJECT
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

    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    int typeId(int index) const override;
    QRgb color(int index) const override;

    QVariantList labels() const override;

    QVariantMap details(int index) const override;

protected:
    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;

private:
    void flattenLoads();
    qint64 insert(qint64 start, qint64 duration, int typeIndex, SceneGraphStage stage,
                  int glyphCount = -1);
    static const char *threadLabel(SceneGraphStage stage);

    QVector<Item> m_data;
};

} // namespace Internal
} // namespace QmlProfiler
