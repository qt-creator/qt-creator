/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef RESIZEMANIPULATOR_H
#define RESIZEMANIPULATOR_H

#include <QPointer>
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
    ResizeManipulator(LayerItem *layerItem, FormEditorView *view);
    ~ResizeManipulator();

    void setHandle(ResizeHandleItem *resizeHandle);
    void removeHandle();

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint, Snapper::Snapping useSnapping);
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

}
#endif // RESIZEMANIPULATOR_H
