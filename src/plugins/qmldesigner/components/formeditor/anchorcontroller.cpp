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

#include "anchorcontroller.h"

#include "formeditoritem.h"
#include "layeritem.h"
#include "formeditorscene.h"
#include "anchorhandleitem.h"
#include <QtDebug>
#include <cmath>
namespace QmlDesigner {

AnchorControllerData::AnchorControllerData(LayerItem *layerItem, FormEditorItem *formEditorItem)
    : layerItem(layerItem),
    formEditorItem(formEditorItem),
    topItem(0),
    leftItem(0),
    rightItem(0),
    bottomItem(0)
{
}

AnchorControllerData::AnchorControllerData(const AnchorControllerData &other)
    : layerItem(other.layerItem),
    formEditorItem(other.formEditorItem),
    topItem(other.topItem),
    leftItem(other.leftItem),
    rightItem(other.rightItem),
    bottomItem(other.bottomItem)
{
}

AnchorControllerData::~AnchorControllerData()
{
    if (layerItem) {
        layerItem->scene()->removeItem(topItem);
        layerItem->scene()->removeItem(leftItem);
        layerItem->scene()->removeItem(rightItem);
        layerItem->scene()->removeItem(bottomItem);
    }
}


AnchorController::AnchorController()
   : m_data(new AnchorControllerData(0, 0))
{

}

AnchorController::AnchorController(const QSharedPointer<AnchorControllerData> &data)
    : m_data(data)
{

}

AnchorController::AnchorController(LayerItem *layerItem, FormEditorItem *formEditorItem)
    : m_data(new AnchorControllerData(layerItem, formEditorItem))
{
    m_data->topItem = new AnchorHandleItem(layerItem, *this);
    m_data->topItem->setZValue(400);
    m_data->topItem->setToolTip(m_data->topItem->toolTipString());

    m_data->leftItem = new AnchorHandleItem(layerItem, *this);
    m_data->leftItem->setZValue(400);
    m_data->leftItem->setToolTip(m_data->leftItem->toolTipString());

    m_data->rightItem = new AnchorHandleItem(layerItem, *this);
    m_data->rightItem->setZValue(400);
    m_data->rightItem->setToolTip(m_data->rightItem->toolTipString());

    m_data->bottomItem = new AnchorHandleItem(layerItem, *this);
    m_data->bottomItem->setZValue(400);
    m_data->bottomItem->setToolTip(m_data->bottomItem->toolTipString());

    m_data->sceneTransform = formEditorItem->sceneTransform();

    updatePosition();
}


bool AnchorController::isValid() const
{
    return m_data->formEditorItem != 0;
}

void AnchorController::show()
{
    m_data->topItem->show();
    m_data->leftItem->show();
    m_data->rightItem->show();
    m_data->bottomItem->show();
}



void AnchorController::hide()
{
    m_data->topItem->hide();
    m_data->leftItem->hide();
    m_data->rightItem->hide();
    m_data->bottomItem->hide();
}


static QPointF topCenter(const QRectF &rect)
{
    return QPointF(rect.center().x(), rect.top());
}

static QPointF leftCenter(const QRectF &rect)
{
    return QPointF(rect.left(), rect.center().y());
}

static QPointF rightCenter(const QRectF &rect)
{
    return QPointF(rect.right(), rect.center().y());
}

static QPointF bottomCenter(const QRectF &rect)
{
    return QPointF(rect.center().x(), rect.bottom());
}

static QPainterPath curveToPath(const QPointF &firstPoint,
                                const QPointF &secondPoint,
                                const QPointF &thirdPoint,
                                const QPointF &fourthPoint)
{
    QPainterPath path;
    path.moveTo(firstPoint);
    path.cubicTo(secondPoint, thirdPoint, fourthPoint);

    return path;
}

static QPointF anchorPoint(const QRectF &boundingRect, AnchorLine::Type anchorLine,  double baseOffset, double innerOffset = 0.0)
{
    switch(anchorLine) {
        case AnchorLine::Top : return topCenter(boundingRect) + QPointF(baseOffset, innerOffset);
        case AnchorLine::Bottom : return bottomCenter(boundingRect) - QPointF(baseOffset, innerOffset);
        case AnchorLine::Left : return leftCenter(boundingRect) + QPointF(innerOffset, baseOffset);
        case AnchorLine::Right : return rightCenter(boundingRect) - QPointF(innerOffset, baseOffset);
        default: return QPointF();
    }

    return QPointF();
}


static QPainterPath createArrowPath(QPointF arrowCenter, double arrowDegrees)
{
    QRectF arrowRect(0.0, 0.0, 16., 16.);
    arrowRect.moveCenter(arrowCenter);
    QPainterPath arrowPath;


    arrowPath.moveTo(arrowCenter);

    arrowPath.arcTo(arrowRect, arrowDegrees + 180 - 20, 40.);

    return arrowPath;
}

AnchorHandlePathData AnchorController::createPainterPathForAnchor(const QRectF &boundingRect,
                                                    AnchorLine::Type anchorLine,
                                                    const QPointF &targetPoint) const
{


    QPointF firstPointInLayerSpace(m_data->sceneTransform.map(anchorPoint(boundingRect, anchorLine, 0.0, 5.0)));

    QPointF topLeftBoundingBoxInLayerSpace(m_data->sceneTransform.map(boundingRect.topLeft()));
    QPointF bottomLeftBoundingBoxInLayerSpace(m_data->sceneTransform.map(boundingRect.bottomLeft()));
    QPointF topRightBoundingBoxInLayerSpace(m_data->sceneTransform.map(boundingRect.topRight()));
    QPointF bottomRightBoundingBoxInLayerSpace(m_data->sceneTransform.map(boundingRect.bottomRight()));

    AnchorLine::Type secondAnchorLine(AnchorLine::Invalid);

    QPointF secondPointInLayerSpace(targetPoint);
    if (targetPoint.isNull()) {
        AnchorLine targetAnchorLine(m_data->formEditorItem->qmlItemNode().anchors().instanceAnchor(anchorLine));
        secondAnchorLine = targetAnchorLine.type();
        FormEditorItem *targetItem = m_data->formEditorItem->scene()->itemForQmlItemNode(targetAnchorLine.qmlItemNode());;
        bool secondItemIsParent = m_data->formEditorItem->parentItem() == targetItem;

        if (secondItemIsParent)
            secondPointInLayerSpace = (targetItem->mapToItem(m_data->layerItem.data(),
                                                             anchorPoint(targetItem->qmlItemNode().instanceBoundingRect(), targetAnchorLine.type(), 0.0)));
        else

            secondPointInLayerSpace = (targetItem->mapToItem(m_data->layerItem.data(),
                                                             anchorPoint(targetItem->qmlItemNode().instanceBoundingRect(), targetAnchorLine.type(), 0.0)));
    }

    QPointF firstControlPointInLayerSpace = (3. * firstPointInLayerSpace + 3 * secondPointInLayerSpace) / 6.;
    QPointF secondControlPointInLayerSpace = (3 * firstPointInLayerSpace + 3. * secondPointInLayerSpace) / 6.;

    bool showAnchorLine(true);
    switch (anchorLine) {
        case AnchorLine::Top :
        case AnchorLine::Bottom :
            firstControlPointInLayerSpace.rx() = firstPointInLayerSpace.x();
            if (qAbs(secondPointInLayerSpace.y() - firstPointInLayerSpace.y()) < 18.0)
                showAnchorLine = false;
            if (qAbs(secondControlPointInLayerSpace.y() - secondPointInLayerSpace.y()) < 20.0 &&
               qAbs(secondControlPointInLayerSpace.x() - secondPointInLayerSpace.x()) > 20.0) {
                firstControlPointInLayerSpace.ry() = firstPointInLayerSpace.y() + ((firstControlPointInLayerSpace.y() - firstPointInLayerSpace.y() > 0) ? 20 : -20);
            }
            break;
        case AnchorLine::Left :
        case AnchorLine::Right :
            firstControlPointInLayerSpace.ry() = firstPointInLayerSpace.y();
            if (qAbs(secondPointInLayerSpace.x() - firstPointInLayerSpace.x()) < 18.0)
                showAnchorLine = false;
            if (qAbs(secondControlPointInLayerSpace.x() - secondPointInLayerSpace.x()) < 20.0 &&
               qAbs(secondControlPointInLayerSpace.y() - secondPointInLayerSpace.y()) > 20.0) {
                firstControlPointInLayerSpace.rx() = firstPointInLayerSpace.x() + ((firstControlPointInLayerSpace.x() - firstPointInLayerSpace.x() > 0) ? 20 : -20);
            }
            break;
        default: break;
    }

    switch(secondAnchorLine) {
        case AnchorLine::Top :
        case AnchorLine::Bottom :
            secondControlPointInLayerSpace.rx() = secondPointInLayerSpace.x();
            if (qAbs(secondControlPointInLayerSpace.y() - secondPointInLayerSpace.y()) < 20.0 &&
               qAbs(secondControlPointInLayerSpace.x() - secondPointInLayerSpace.x()) > 20.0) {
                secondControlPointInLayerSpace.ry() = secondPointInLayerSpace.y() + ((secondControlPointInLayerSpace.y() - secondPointInLayerSpace.y() < 0) ? 20 : -20);
            }
            break;
        case AnchorLine::Left :
        case AnchorLine::Right :
            secondControlPointInLayerSpace.ry() = secondPointInLayerSpace.y();
            if (qAbs(secondControlPointInLayerSpace.x() - secondPointInLayerSpace.x()) < 20.0 &&
               qAbs(secondControlPointInLayerSpace.y() - secondPointInLayerSpace.y()) > 20.0) {
                secondControlPointInLayerSpace.rx() = secondPointInLayerSpace.x() + ((secondControlPointInLayerSpace.x() - secondPointInLayerSpace.x() < 0) ? 20 : -20);
            }
            break;
        default: break;
    }

    QPainterPath anchorLinePath;
    anchorLinePath.setFillRule(Qt::WindingFill);

    QRectF baseRect(0.0, 0.0, 5., 5.);
    baseRect.moveCenter(firstPointInLayerSpace);
    QPainterPath basePath;
    basePath.addRoundedRect(baseRect, 6., 6.);
    anchorLinePath = anchorLinePath.united(basePath);

    QRectF baseLineRect;
    switch (anchorLine) {
        case AnchorLine::Left : {
                baseLineRect = QRectF(topLeftBoundingBoxInLayerSpace, bottomLeftBoundingBoxInLayerSpace);
                baseLineRect.setWidth(3);
            }
            break;
        case AnchorLine::Top : {
                baseLineRect = QRectF(topLeftBoundingBoxInLayerSpace, topRightBoundingBoxInLayerSpace);
                baseLineRect.setHeight(3);
            }
            break;
        case AnchorLine::Right : {
                baseLineRect = QRectF(topRightBoundingBoxInLayerSpace, bottomRightBoundingBoxInLayerSpace);
                baseLineRect.adjust(-3, 0, 0, 0);
            }
            break;
        case AnchorLine::Bottom : {
                baseLineRect = QRectF(bottomLeftBoundingBoxInLayerSpace, bottomRightBoundingBoxInLayerSpace);
                baseLineRect.adjust(0, -3, 0, 0);
            }
            break;
        default: break;
    }

    if (!baseLineRect.isEmpty()) {

        QPainterPath baseLinePath;
        baseLinePath.addRoundedRect(baseLineRect, 1., 1.);
        anchorLinePath = anchorLinePath.united(baseLinePath);
    }

    QPainterPath arrowPath;
    arrowPath.setFillRule(Qt::WindingFill);



    if (showAnchorLine) {
        QPainterPath curvePath(curveToPath(firstPointInLayerSpace,
                                           firstControlPointInLayerSpace,
                                           secondControlPointInLayerSpace,
                                           secondPointInLayerSpace));

        double arrowDegrees = curvePath.angleAtPercent(curvePath.percentAtLength(curvePath.length() - 2.5));



        QPainterPathStroker arrowPathStroker;
        arrowPathStroker.setWidth(2.0);
        arrowPathStroker.setCapStyle(Qt::RoundCap);

        arrowPath = arrowPath.united(arrowPathStroker.createStroke(curvePath));





        QRectF arrowCutRect(0.0, 0.0, 8., 8.);
        arrowCutRect.moveCenter(secondPointInLayerSpace);
        QPainterPath arrowCutPath;
        arrowCutPath.addRect(arrowCutRect);
        arrowPath = arrowPath.subtracted(arrowCutPath);

        arrowPath = arrowPath.united(createArrowPath(secondPointInLayerSpace, arrowDegrees));
    }

    AnchorHandlePathData pathData;
    pathData.arrowPath = arrowPath;
    pathData.sourceAnchorLinePath = anchorLinePath;
    pathData.beginArrowPoint = firstPointInLayerSpace;
    pathData.endArrowPoint = secondPointInLayerSpace;

    pathData.targetAnchorLinePath = createTargetAnchorLinePath(anchorLine);
    pathData.targetNamePath = createTargetNamePathPath(anchorLine);

    return pathData;
}

QPainterPath AnchorController::createTargetNamePathPath(AnchorLine::Type anchorLine) const
{
    QPainterPath path;
    QmlAnchors anchors(formEditorItem()->qmlItemNode().anchors());
    if (anchors.instanceHasAnchor(anchorLine)) {
        AnchorLine targetAnchorLine(anchors.instanceAnchor(anchorLine));

        FormEditorItem *targetItem = formEditorItem()->scene()->itemForQmlItemNode(targetAnchorLine.qmlItemNode());
        QRectF boundingRect(targetItem->qmlItemNode().instanceBoundingRect());

        QTransform sceneTransform(targetItem->qmlItemNode().instanceSceneTransform());

        QPointF centerBoundingBoxInLayerSpace(sceneTransform.map(boundingRect.center()));

        QFont font;
        font.setPixelSize(24);
        QString nameString(QString("%1 (%2)").arg(targetAnchorLine.qmlItemNode().simplfiedTypeName()).arg(targetAnchorLine.qmlItemNode().id()));
        path.addText(0., -4., font, nameString);
        //path.translate(centerBoundingBoxInLayerSpace - path.qmlItemNode().instanceBoundingRect().center());

    }

    return path;
}

QPainterPath AnchorController::createTargetAnchorLinePath(AnchorLine::Type anchorLine) const
{
    QPainterPath path;
    QmlAnchors anchors(formEditorItem()->qmlItemNode().anchors());
    if (anchors.instanceHasAnchor(anchorLine)) {
        AnchorLine targetAnchorLine(anchors.instanceAnchor(anchorLine));

        FormEditorItem *targetItem = formEditorItem()->scene()->itemForQmlItemNode(targetAnchorLine.qmlItemNode());
        QRectF boundingRect(targetItem->qmlItemNode().instanceBoundingRect());

        QTransform sceneTransform(targetItem->qmlItemNode().instanceSceneTransform());

        QPointF topLeftBoundingBoxInLayerSpace(sceneTransform.map(boundingRect.topLeft()));
        QPointF bottomLeftBoundingBoxInLayerSpace(sceneTransform.map(boundingRect.bottomLeft()));
        QPointF topRightBoundingBoxInLayerSpace(sceneTransform.map(boundingRect.topRight()));
        QPointF bottomRightBoundingBoxInLayerSpace(sceneTransform.map(boundingRect.bottomRight()));


        switch(targetAnchorLine.type()) {
            case AnchorLine::Top : {
                    path.moveTo(topLeftBoundingBoxInLayerSpace);
                    path.lineTo(topRightBoundingBoxInLayerSpace);
                }
                break;
            case AnchorLine::Bottom : {
                    path.moveTo(bottomLeftBoundingBoxInLayerSpace);
                    path.lineTo(bottomRightBoundingBoxInLayerSpace);
                }
                break;
            case AnchorLine::Left : {
                    path.moveTo(topLeftBoundingBoxInLayerSpace);
                    path.lineTo(bottomLeftBoundingBoxInLayerSpace);
                }
                break;
            case AnchorLine::Right : {
                    path.moveTo(topRightBoundingBoxInLayerSpace);
                    path.lineTo(bottomRightBoundingBoxInLayerSpace);
                }
                break;
            default: break;
        }

        QPainterPathStroker pathStroker;
        pathStroker.setWidth(20.0);
        pathStroker.setCapStyle(Qt::RoundCap);
        path = pathStroker.createStroke(path);


    }

    return path;
}

void AnchorController::updatePosition()
{
    QRectF boundingRect = m_data->formEditorItem->qmlItemNode().instanceBoundingRect();
    QPointF beginPoint;
    QPointF endPoint;
    QmlAnchors anchors(m_data->formEditorItem->qmlItemNode().anchors());
    m_data->sceneTransform = m_data->formEditorItem->sceneTransform();

    if (anchors.instanceHasAnchor(AnchorLine::Top))
        m_data->topItem->setHandlePath(createPainterPathForAnchor(boundingRect, AnchorLine::Top));
    else
        m_data->topItem->setHandlePath(AnchorHandlePathData());

    if (anchors.instanceHasAnchor(AnchorLine::Bottom))
        m_data->bottomItem->setHandlePath(createPainterPathForAnchor(boundingRect, AnchorLine::Bottom));
    else
        m_data->bottomItem->setHandlePath(AnchorHandlePathData());

    if (anchors.instanceHasAnchor(AnchorLine::Right))
        m_data->rightItem->setHandlePath(createPainterPathForAnchor(boundingRect, AnchorLine::Right));
    else
        m_data->rightItem->setHandlePath(AnchorHandlePathData());

    if (anchors.instanceHasAnchor(AnchorLine::Left))
        m_data->leftItem->setHandlePath(createPainterPathForAnchor(boundingRect, AnchorLine::Left));
    else
        m_data->leftItem->setHandlePath(AnchorHandlePathData());
}


FormEditorItem* AnchorController::formEditorItem() const
{
    return m_data->formEditorItem;
}

QWeakPointer<AnchorControllerData> AnchorController::weakPointer() const
{
    return m_data;
}


bool AnchorController::isTopHandle(const AnchorHandleItem *handle) const
{
    return handle == m_data->topItem;
}

bool AnchorController::isLeftHandle(const AnchorHandleItem *handle) const
{
    return handle == m_data->leftItem;
}

bool AnchorController::isRightHandle(const AnchorHandleItem *handle) const
{
    return handle == m_data->rightItem;
}

bool AnchorController::isBottomHandle(const AnchorHandleItem *handle) const
{
    return handle == m_data->bottomItem;
}

void AnchorController::updateTargetPoint(AnchorLine::Type anchorLine, const QPointF &targetPoint)
{
    QRectF boundingRect = m_data->formEditorItem->qmlItemNode().instanceBoundingRect();

    switch(anchorLine) {
        case AnchorLine::Top :
            m_data->topItem->setHandlePath(createPainterPathForAnchor(boundingRect, anchorLine, targetPoint)); break;
        case AnchorLine::Bottom :
            m_data->bottomItem->setHandlePath(createPainterPathForAnchor(boundingRect, anchorLine, targetPoint)); break;
        case AnchorLine::Left :
            m_data->leftItem->setHandlePath(createPainterPathForAnchor(boundingRect, anchorLine, targetPoint)); break;
        case AnchorLine::Right :
            m_data->rightItem->setHandlePath(createPainterPathForAnchor(boundingRect, anchorLine, targetPoint)); break;
        default: break;
    }
}

void AnchorController::highlight(AnchorLine::Type anchorLine)
{
    switch(anchorLine) {
        case AnchorLine::Top :
            m_data->topItem->setHighlighted(true); break;
        case AnchorLine::Bottom :
            m_data->bottomItem->setHighlighted(true); break;
        case AnchorLine::Left :
            m_data->leftItem->setHighlighted(true); break;
        case AnchorLine::Right :
            m_data->rightItem->setHighlighted(true); break;
        default: break;
    }
}

void AnchorController::clearHighlight()
{
    m_data->topItem->setHighlighted(false);
    m_data->leftItem->setHighlighted(false);
    m_data->rightItem->setHighlighted(false);
    m_data->bottomItem->setHighlighted(false);
}

} // namespace QmlDesigner
