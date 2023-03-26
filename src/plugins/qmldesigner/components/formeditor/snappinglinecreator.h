// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "layeritem.h"
#include <QPair>

namespace QmlDesigner {

class FormEditorItem;

using SnapLineMap = QMultiMap<double, QPair<QRectF, FormEditorItem*> >;

class FormEditorItem;

class SnappingLineCreator
{
public:
    SnappingLineCreator(FormEditorItem *formEditorItem);

    void update(const QList<FormEditorItem*> &exceptionList,
                FormEditorItem *transformationSpaceItem,
                FormEditorItem *containerFormEditorItem);


    SnapLineMap topLines() const;
    SnapLineMap bottomLines() const;
    SnapLineMap leftLines() const;
    SnapLineMap rightLines() const;
    SnapLineMap horizontalCenterLines() const;
    SnapLineMap verticalCenterLines() const;

    SnapLineMap topOffsets() const;
    SnapLineMap bottomOffsets() const;
    SnapLineMap leftOffsets() const;
    SnapLineMap rightOffsets() const;

    void setContainerPaddingItem(FormEditorItem *transformationSpaceItem);
    void setContainerPaddingByGloablPadding(double containerPadding);
    void setContainerPaddingByContentItem(const QRectF &contentRectangle, const QRectF &itemBoundingRectangle);
    void setSpacing(double spacing);

protected:
    void addLines(const QRectF &rectInSceneSpace, FormEditorItem *item);
    void addOffsets(const QRectF &rectInSceneSpace, FormEditorItem *item);

    void clearLines();
    void generateLines(const QList<FormEditorItem*> &exceptionList,
                       FormEditorItem *transformationSpaceItem);

private:
    SnapLineMap m_topLineMap;
    SnapLineMap m_bottomLineMap;
    SnapLineMap m_leftLineMap;
    SnapLineMap m_rightLineMap;
    SnapLineMap m_horizontalCenterLineMap;
    SnapLineMap m_verticalCenterLineMap;

    SnapLineMap m_topOffsetMap;
    SnapLineMap m_bottomOffsetMap;
    SnapLineMap m_leftOffsetMap;
    SnapLineMap m_rightOffsetMap;

    FormEditorItem *m_formEditorItem;
    double m_topOffset;
    double m_bottomOffset;
    double m_leftOffset;
    double m_rightOffset;
    double m_topPadding;
    double m_bottomPadding;
    double m_leftPadding;
    double m_rightPadding;
};

} // namespace QmlDesigner
