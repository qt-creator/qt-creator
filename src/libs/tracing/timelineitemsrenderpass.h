// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineabstractrenderer.h"
#include "timelinerenderpass.h"
#include <QSGMaterial>

namespace Timeline {

class TimelineItemsMaterial : public QSGMaterial
{
public:
    TimelineItemsMaterial();
    QVector2D scale() const;
    void setScale(QVector2D scale);

    float selectedItem() const;
    void setSelectedItem(float selectedItem);

    QColor selectionColor() const;
    void setSelectionColor(QColor selectionColor);

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;

private:
    QVector2D m_scale;
    float m_selectedItem;
    QColor m_selectionColor;
};

class OpaqueColoredPoint2DWithSize
{
public:
    enum Direction {
        InvalidDirection,
        TopToBottom,
        BottomToTop,
        MaximumDirection
    };

    void set(float nx, float ny, float nw, float nh, float nid, uchar nr, uchar ng, uchar nb,
             uchar d);

    float top() const;
    void update(float nr, float ny);
    Direction direction() const;

    void setBottomLeft(const OpaqueColoredPoint2DWithSize *master);
    void setBottomRight(const OpaqueColoredPoint2DWithSize *master);
    void setTopLeft(const OpaqueColoredPoint2DWithSize *master);
    void setTopRight(const OpaqueColoredPoint2DWithSize *master);

    static const QSGGeometry::AttributeSet &attributes();
    static OpaqueColoredPoint2DWithSize *fromVertexData(QSGGeometry *geometry);

private:
    float x, y; // vec4 vertexCoord
    float w, h; // vec2 rectSize
    float id; // float selectionId
    unsigned char r, g, b, a; // vec4 vertexColor

    void setCommon(const OpaqueColoredPoint2DWithSize *master);
    void setLeft(const OpaqueColoredPoint2DWithSize *master);
    void setRight(const OpaqueColoredPoint2DWithSize *master);
    void setTop(const OpaqueColoredPoint2DWithSize *master);
    void setBottom(const OpaqueColoredPoint2DWithSize *master);
};

class TRACING_EXPORT TimelineItemsRenderPass : public TimelineRenderPass
{
public:
    static const TimelineItemsRenderPass *instance();
    State *update(const TimelineAbstractRenderer *renderer, const TimelineRenderState *parentState,
                  State *state, int firstIndex, int lastIndex, bool stateChanged,
                  float spacing) const override;
protected:
    TimelineItemsRenderPass();
};

} // namespace Timeline
