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
