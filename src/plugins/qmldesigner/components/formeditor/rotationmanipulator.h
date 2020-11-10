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

#include "rotationhandleitem.h"
#include <snapper.h>
#include "rewritertransaction.h"
#include "formeditorview.h"

#include <QPointer>

namespace QmlDesigner {

class RotationHandleItem;
class Model;

class RotationManipulator
{
public:
    RotationManipulator(LayerItem *layerItem, FormEditorView *view);
    ~RotationManipulator();

    void setHandle(RotationHandleItem *rotationHandle);
    void removeHandle();

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint, Qt::KeyboardModifiers keyMods = Qt::NoModifier);
    void end();

    void clear();

    bool isActive() const;

protected:
    void deleteSnapLines();
    RotationHandleItem *rotationHandle();

private:
    QPointer<FormEditorView> m_view;
    QList<QGraphicsItem*> m_graphicsLineList;
    RotationController m_rotationController; // hold the controller so that the handle cant be deleted
    QTransform m_beginFromSceneToContentItemTransform;
    QTransform m_beginFromContentItemToSceneTransform;
    QTransform m_beginFromItemToSceneTransform;
    QTransform m_beginToParentTransform;
    QRectF m_beginBoundingRect;
    QPointF m_beginBottomRightPoint;
    double m_beginTopMargin;
    double m_beginLeftMargin;
    double m_beginRightMargin;
    double m_beginBottomMargin;
    QPointer<LayerItem> m_layerItem;
    RotationHandleItem *m_rotationHandle = nullptr;
    RewriterTransaction m_rewriterTransaction;
    bool m_isActive = false;

    qreal m_beginRotation;
};

} // namespace QmlDesigner
