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

#ifndef SCENEGRAPHTIMELINEMODEL_H
#define SCENEGRAPHTIMELINEMODEL_H

#include "qmlprofiler/qmlprofilertimelinemodel.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/qmlprofilerdatamodel.h"

#include <QStringList>
#include <QColor>

namespace QmlProfilerExtension {
namespace Internal {

class SceneGraphTimelineModel : public QmlProfiler::QmlProfilerTimelineModel
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

    struct SceneGraphEvent {
        SceneGraphEvent(int typeId = -1, int glyphCount = -1);
        int typeId;
        int rowNumberCollapsed;
        int glyphCount; // only used for one event type
    };

    SceneGraphTimelineModel(QmlProfiler::QmlProfilerModelManager *manager, QObject *parent = 0);

    int row(int index) const;
    int typeId(int index) const;
    QColor color(int index) const;

    QVariantList labels() const;

    QVariantMap details(int index) const;

protected:
    void loadData();
    void clear();

private:
    void flattenLoads();
    qint64 insert(qint64 start, qint64 duration, int typeIndex, SceneGraphStage stage,
                  int glyphCount = -1);
    static const char *threadLabel(SceneGraphStage stage);

    QVector<SceneGraphEvent> m_data;
};

} // namespace Internal
} // namespace QmlProfilerExtension

#endif // SCENEGRAPHTIMELINEMODEL_H
