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

#ifndef ANCHORCONTROLLER_H
#define ANCHORCONTROLLER_H

#include <QSharedPointer>
#include <QPainterPath>
#include <QPair>
#include <QTransform>
#include <qmlanchors.h>

namespace QmlDesigner {

struct AnchorHandlePathData
{
    QPainterPath  sourceAnchorLinePath;
    QPainterPath  targetAnchorLinePath;
    QPainterPath  arrowPath;
    QPainterPath  targetNamePath;
    QPointF       beginArrowPoint;
    QPointF       endArrowPoint;
};

class FormEditorItem;
class LayerItem;
class AnchorHandleItem;

class AnchorControllerData
{
public:
    AnchorControllerData(LayerItem *layerItem,
                         FormEditorItem *formEditorItem);
    AnchorControllerData(const AnchorControllerData &other);
    ~AnchorControllerData();

    QWeakPointer<LayerItem> layerItem;
    FormEditorItem *formEditorItem;

    AnchorHandleItem *topItem;
    AnchorHandleItem *leftItem;
    AnchorHandleItem *rightItem;
    AnchorHandleItem *bottomItem;

    QTransform sceneTransform;
};


class AnchorController
{
 public:
    AnchorController();
    AnchorController(LayerItem *layerItem, FormEditorItem *formEditorItem);
    AnchorController(const QSharedPointer<AnchorControllerData> &data);


    void show();
    void hide();

    void updatePosition();

    bool isValid() const;

    QWeakPointer<AnchorControllerData> weakPointer() const;


    FormEditorItem *formEditorItem() const;

    bool isTopHandle(const AnchorHandleItem *handle) const;
    bool isLeftHandle(const AnchorHandleItem *handle) const;
    bool isRightHandle(const AnchorHandleItem *handle) const;
    bool isBottomHandle(const AnchorHandleItem *handle) const;

    void updateTargetPoint(AnchorLine::Type anchorLine, const QPointF &targetPoint);

    void clearHighlight();
    void highlight(AnchorLine::Type anchorLine);

private: //functions
    AnchorHandlePathData createPainterPathForAnchor(const QRectF &boundingRect,
                                      AnchorLine::Type anchorLine,
                                      const QPointF &targetPoint = QPointF()) const;
    QPainterPath createTargetAnchorLinePath(AnchorLine::Type anchorLine) const;
    QPainterPath createTargetNamePathPath(AnchorLine::Type anchorLine) const;
private:
    QSharedPointer<AnchorControllerData> m_data;
};

} // namespace QmlDesigner

#endif // ANCHORCONTROLLER_H
