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

#ifndef RESIZECONTROLLER_H
#define RESIZECONTROLLER_H

#include <QWeakPointer>
#include <QSharedPointer>
#include <QGraphicsItem>
#include <QGraphicsObject>

namespace QmlViewer {

class LayerItem;
class ResizeHandleItem;

class ResizeControllerData
{
public:
    ResizeControllerData(LayerItem *layerItem,
                         QGraphicsObject *formEditorItem);
    //ResizeControllerData(const ResizeControllerData &other);
    ~ResizeControllerData();
    void clear();

    QWeakPointer<LayerItem> layerItem;
    QWeakPointer<QGraphicsObject> formEditorItem;
    ResizeHandleItem *topLeftItem;
    ResizeHandleItem *topRightItem;
    ResizeHandleItem *bottomLeftItem;
    ResizeHandleItem *bottomRightItem;
    ResizeHandleItem *topItem;
    ResizeHandleItem *leftItem;
    ResizeHandleItem *rightItem;
    ResizeHandleItem *bottomItem;
};




class ResizeController
{
public:
    friend class ResizeHandleItem;

    ResizeController();
    ~ResizeController();
    ResizeController(LayerItem *layerItem, QGraphicsObject *formEditorItem);

    void show();
    void hide();
    void setItem(LayerItem *layerItem, QGraphicsObject *item);

    void updatePosition();

    bool isValid() const;

    QGraphicsObject *formEditorItem() const;

    bool isTopLeftHandle(const ResizeHandleItem *handle) const;
    bool isTopRightHandle(const ResizeHandleItem *handle) const;
    bool isBottomLeftHandle(const ResizeHandleItem *handle) const;
    bool isBottomRightHandle(const ResizeHandleItem *handle) const;

    bool isTopHandle(const ResizeHandleItem *handle) const;
    bool isLeftHandle(const ResizeHandleItem *handle) const;
    bool isRightHandle(const ResizeHandleItem *handle) const;
    bool isBottomHandle(const ResizeHandleItem *handle) const;

private: // functions
    void createFor(LayerItem *layerItem, QGraphicsObject *formEditorItem);
    ResizeControllerData *data() const;

private: // variables
    ResizeControllerData *m_data;
};

}

#endif // RESIZECONTROLLER_H
