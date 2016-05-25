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

    QSGMaterialType *type() const;
    QSGMaterialShader *createShader() const;

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
    float x, y, w, h, id;
    unsigned char r, g, b, a;

    void setCommon(const OpaqueColoredPoint2DWithSize *master);
    void setLeft(const OpaqueColoredPoint2DWithSize *master);
    void setRight(const OpaqueColoredPoint2DWithSize *master);
    void setTop(const OpaqueColoredPoint2DWithSize *master);
    void setBottom(const OpaqueColoredPoint2DWithSize *master);
};

class TIMELINE_EXPORT TimelineItemsRenderPass : public TimelineRenderPass
{
public:
    static const TimelineItemsRenderPass *instance();
    State *update(const TimelineAbstractRenderer *renderer, const TimelineRenderState *parentState,
                  State *state, int firstIndex, int lastIndex, bool stateChanged,
                  qreal spacing) const;
protected:
    TimelineItemsRenderPass();
};

} // namespace Timeline
