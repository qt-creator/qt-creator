// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "resizehandleitem.h"
#include <snapper.h>
#include "rewritertransaction.h"
#include "formeditorview.h"

#include <QPointer>

namespace QmlDesigner {

class ResizeHandleItem;
class Model;

class ResizeManipulator
{
public:
    ResizeManipulator(LayerItem *layerItem, FormEditorView *view);
    ~ResizeManipulator();

    void setHandle(ResizeHandleItem *resizeHandle);
    void removeHandle();

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint, Snapper::Snapping useSnapping,
                Qt::KeyboardModifiers keyMods = Qt::NoModifier);
    void end(Snapper::Snapping useSnapping);

    void moveBy(double deltaX, double deltaY);

    void clear();

    bool isActive() const;

protected:
    bool isInvalidSize(const QSizeF & size);
    void deleteSnapLines();
    ResizeHandleItem *resizeHandle();

private:
    Snapper m_snapper;
    QPointer<FormEditorView> m_view;
    QList<QGraphicsItem*> m_graphicsLineList;
    ResizeController m_resizeController; // hold the controller so that the handle cant be deleted
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
    ResizeHandleItem *m_resizeHandle;
    RewriterTransaction m_rewriterTransaction;
    bool m_isActive;
};

} // namespace QmlDesigner
