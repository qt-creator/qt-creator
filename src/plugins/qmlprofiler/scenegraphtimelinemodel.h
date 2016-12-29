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

    struct SceneGraphEvent {
        SceneGraphEvent(int typeId = -1, int glyphCount = -1);
        int typeId;
        int rowNumberCollapsed;
        int glyphCount; // only used for one event type
    };

    SceneGraphTimelineModel(QmlProfilerModelManager *manager, QObject *parent = 0);

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

    QVector<SceneGraphEvent> m_data;
};

} // namespace Internal
} // namespace QmlProfiler
