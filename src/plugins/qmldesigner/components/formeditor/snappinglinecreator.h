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

#include "layeritem.h"
#include <QPair>

namespace QmlDesigner {

class FormEditorItem;

typedef QMultiMap<double, QPair<QRectF, FormEditorItem*> > SnapLineMap;
typedef QMapIterator<double, QPair<QRectF, FormEditorItem*> > SnapLineMapIterator;

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
