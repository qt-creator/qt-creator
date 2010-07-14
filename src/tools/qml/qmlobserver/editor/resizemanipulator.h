/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef RESIZEMANIPULATOR_H
#define RESIZEMANIPULATOR_H

#include <QWeakPointer>
#include "layeritem.h"
#include "resizehandleitem.h"

#include <QDeclarativeItem>
#include <QDeclarativeView>

namespace QmlViewer {

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

    ResizeManipulator(LayerItem *layer, QDeclarativeView *view);
    ~ResizeManipulator();

    void setHandle(ResizeHandleItem *resizeHandle);
    void removeHandle();

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint, Snapping useSnapping);
    void end();

    void moveBy(double deltaX, double deltaY);

    void clear();

protected:
    bool isInvalidSize(const QSizeF & size);
    void deleteSnapLines();
    ResizeHandleItem *resizeHandle();

private:
//    Snapper m_snapper;
    QWeakPointer<QDeclarativeView> m_view;
    QList<QGraphicsItem*> m_graphicsLineList;
    ResizeController *m_resizeController;
    QTransform m_beginFromSceneTransform;
    QTransform m_beginToSceneTransform;
    QTransform m_beginToParentTransform;
    QRectF m_beginBoundingRect;
    QPointF m_beginBottomRightPoint;
    double m_beginTopMargin;
    double m_beginLeftMargin;
    double m_beginRightMargin;
    double m_beginBottomMargin;
    ResizeHandleItem *m_resizeHandle;
    QWeakPointer<LayerItem> m_layerItem;
};

}

#endif // RESIZEMANIPULATOR_H
