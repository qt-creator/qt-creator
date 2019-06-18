/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "curveeditorstyle.h"
#include "selectableitem.h"

#include <QGraphicsObject>

namespace DesignTools {

class AnimationCurve;
class KeyframeItem;
class GraphicsScene;

class CurveItem : public QGraphicsObject
{
    Q_OBJECT

public:
    CurveItem(QGraphicsItem *parent = nullptr);

    CurveItem(unsigned int id, const AnimationCurve &curve, QGraphicsItem *parent = nullptr);

    ~CurveItem() override;

    enum { Type = ItemTypeCurve };

    int type() const override;

    QRectF boundingRect() const override;

    bool contains(const QPointF &point) const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    bool isDirty() const;

    bool hasSelection() const;

    unsigned int id() const;

    AnimationCurve curve() const;

    void setDirty(bool dirty);

    QRectF setComponentTransform(const QTransform &transform);

    void setStyle(const CurveEditorStyle &style);

    void connect(GraphicsScene *scene);

    void setIsUnderMouse(bool under);

private:
    QPainterPath path() const;

    unsigned int m_id;

    CurveItemStyleOption m_style;

    QTransform m_transform;

    std::vector<KeyframeItem *> m_keyframes;

    bool m_underMouse;

    bool m_itemDirty;

    mutable bool m_pathDirty;

    mutable QPainterPath m_path;
};

} // End namespace DesignTools.
