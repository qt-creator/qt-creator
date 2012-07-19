/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef RESIZEMANIPULATOR_H
#define RESIZEMANIPULATOR_H

#include <QWeakPointer>
#include "resizehandleitem.h"
#include <snapper.h>
#include "rewritertransaction.h"
#include "formeditorview.h"

namespace QmlDesigner {

class ResizeHandleItem;
class Model;

class ResizeManipulator
{
public:
    enum Snapping {
        UseSnapping,
        UseSnappingAndAnchoring,
        NoSnapping
    };

    ResizeManipulator(LayerItem *layerItem, FormEditorView *view);
    ~ResizeManipulator();

    void setHandle(ResizeHandleItem *resizeHandle);
    void removeHandle();

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint, Snapping useSnapping);
    void end();

    void moveBy(double deltaX, double deltaY);

    void clear();

    bool isActive() const;

protected:
    bool isInvalidSize(const QSizeF & size);
    void deleteSnapLines();
    ResizeHandleItem *resizeHandle();
    void setSize(QmlItemNode itemNode, const QSizeF &size);
    void setPosition(QmlItemNode itemNode, const QPointF &position);

private:
    Snapper m_snapper;
    QWeakPointer<FormEditorView> m_view;
    QList<QGraphicsItem*> m_graphicsLineList;
    ResizeController m_resizeController; // hold the controller so that the handle cant be deleted
    QTransform m_beginFromSceneTransform;
    QTransform m_beginToSceneTransform;
    QTransform m_beginToParentTransform;
    QRectF m_beginBoundingRect;
    QPointF m_beginBottomRightPoint;
    double m_beginTopMargin;
    double m_beginLeftMargin;
    double m_beginRightMargin;
    double m_beginBottomMargin;
    QWeakPointer<LayerItem> m_layerItem;
    ResizeHandleItem *m_resizeHandle;
    RewriterTransaction m_rewriterTransaction;
    bool m_isActive;
};

}
#endif // RESIZEMANIPULATOR_H
