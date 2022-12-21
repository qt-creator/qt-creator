// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
