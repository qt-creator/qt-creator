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

#include "pathitem.h"

#include <nodeproperty.h>
#include <variantproperty.h>
#include <nodelistproperty.h>
#include <rewritertransaction.h>
#include <formeditorscene.h>
#include <formeditorview.h>

#include <QPainter>
#include <QMenu>
#include <QtDebug>
#include <QGraphicsSceneMouseEvent>

namespace QmlDesigner {

PathItem::PathItem(FormEditorScene* scene)
            : QGraphicsObject(),
              m_selectionManipulator(this),
              m_lastPercent(-1.),
              m_formEditorItem(0),
              m_dontUpdatePath(false)
{
    scene->addItem(this);
    setFlag(QGraphicsItem::ItemIsMovable, false);
}

PathItem::~PathItem()
{
    m_formEditorItem = 0;
}

static ModelNode pathModelNode(FormEditorItem *formEditorItem)
{
     ModelNode modelNode = formEditorItem->qmlItemNode().modelNode();

     return modelNode.nodeProperty("path").modelNode();
}

typedef QPair<PropertyName, QVariant> PropertyPair;

void PathItem::writeLinePath(ModelNode pathNode, const CubicSegment &cubicSegment)
{
    QList<PropertyPair> propertyList;
    propertyList.append(PropertyPair("x", cubicSegment.fourthControlX()));
    propertyList.append(PropertyPair("y", cubicSegment.fourthControlY()));

    ModelNode lineNode = pathNode.view()->createModelNode("QtQuick.PathLine", pathNode.majorVersion(), pathNode.minorVersion(), propertyList);
    pathNode.nodeListProperty("pathElements").reparentHere(lineNode);
}

void PathItem::writeQuadPath(ModelNode pathNode, const CubicSegment &cubicSegment)
{
    QList<QPair<PropertyName, QVariant> > propertyList;
    propertyList.append(PropertyPair("controlX", cubicSegment.quadraticControlX()));
    propertyList.append(PropertyPair("controlY", cubicSegment.quadraticControlY()));
    propertyList.append(PropertyPair("x", cubicSegment.fourthControlX()));
    propertyList.append(PropertyPair("y", cubicSegment.fourthControlY()));

    ModelNode lineNode = pathNode.view()->createModelNode("QtQuick.PathQuad", pathNode.majorVersion(), pathNode.minorVersion(), propertyList);
    pathNode.nodeListProperty("pathElements").reparentHere(lineNode);
}

void PathItem::writeCubicPath(ModelNode pathNode, const CubicSegment &cubicSegment)
{
    QList<QPair<PropertyName, QVariant> > propertyList;
    propertyList.append(PropertyPair("control1X", cubicSegment.secondControlX()));
    propertyList.append(PropertyPair("control1Y", cubicSegment.secondControlY()));
    propertyList.append(PropertyPair("control2X", cubicSegment.thirdControlX()));
    propertyList.append(PropertyPair("control2Y", cubicSegment.thirdControlY()));
    propertyList.append(PropertyPair("x", cubicSegment.fourthControlX()));
    propertyList.append(PropertyPair("y", cubicSegment.fourthControlY()));

    ModelNode lineNode = pathNode.view()->createModelNode("QtQuick.PathCubic", pathNode.majorVersion(), pathNode.minorVersion(), propertyList);
    pathNode.nodeListProperty("pathElements").reparentHere(lineNode);
}

void PathItem::writePathAttributes(ModelNode pathNode, const QMap<QString, QVariant> &attributes)
{
    QMapIterator<QString, QVariant> attributesIterator(attributes);
    while (attributesIterator.hasNext()) {
        attributesIterator.next();
        QList<QPair<PropertyName, QVariant> > propertyList;
        propertyList.append(PropertyPair("name", attributesIterator.key()));
        propertyList.append(PropertyPair("value", attributesIterator.value()));

        ModelNode lineNode = pathNode.view()->createModelNode("QtQuick.PathAttribute", pathNode.majorVersion(), pathNode.minorVersion(), propertyList);
        pathNode.nodeListProperty("pathElements").reparentHere(lineNode);
    }
}

void PathItem::writePathPercent(ModelNode pathNode, double percent)
{
    if (percent >= 0.0) {
        QList<QPair<PropertyName, QVariant> > propertyList;
        propertyList.append(PropertyPair("value", percent));

        ModelNode lineNode = pathNode.view()->createModelNode("QtQuick.PathPercent", pathNode.majorVersion(), pathNode.minorVersion(), propertyList);
        pathNode.nodeListProperty("pathElements").reparentHere(lineNode);
    }
}

void PathItem::writePathToProperty()
{
    PathUpdateDisabler pathUpdateDisable(this);

    ModelNode pathNode = pathModelNode(formEditorItem());

    RewriterTransaction rewriterTransaction = pathNode.view()->beginRewriterTransaction(QByteArrayLiteral("PathItem::writePathToProperty"));

    QList<ModelNode> pathSegmentNodes = pathNode.nodeListProperty("pathElements").toModelNodeList();

    foreach (ModelNode pathSegment, pathSegmentNodes)
        pathSegment.destroy();

    if (!m_cubicSegments.isEmpty()) {
        pathNode.variantProperty("startX").setValue(m_cubicSegments.first().firstControlPoint().coordinate().x());
        pathNode.variantProperty("startY").setValue(m_cubicSegments.first().firstControlPoint().coordinate().y());

        foreach (const CubicSegment &cubicSegment, m_cubicSegments) {
            writePathAttributes(pathNode, cubicSegment.attributes());
            writePathPercent(pathNode, cubicSegment.percent());

            if (cubicSegment.canBeConvertedToLine())
                writeLinePath(pathNode, cubicSegment);
            else if (cubicSegment.canBeConvertedToQuad())
                writeQuadPath(pathNode, cubicSegment);
            else
                writeCubicPath(pathNode, cubicSegment);
        }

        writePathAttributes(pathNode, m_lastAttributes);
        writePathPercent(pathNode, m_lastPercent);
    }

    rewriterTransaction.commit();
}

void PathItem::writePathAsCubicSegmentsOnly()
{
    PathUpdateDisabler pathUpdateDisabler(this);

    ModelNode pathNode = pathModelNode(formEditorItem());

    RewriterTransaction rewriterTransaction =
        pathNode.view()->beginRewriterTransaction(QByteArrayLiteral("PathItem::writePathAsCubicSegmentsOnly"));

    QList<ModelNode> pathSegmentNodes = pathNode.nodeListProperty("pathElements").toModelNodeList();

    foreach (ModelNode pathSegment, pathSegmentNodes)
        pathSegment.destroy();

    if (!m_cubicSegments.isEmpty()) {
        pathNode.variantProperty("startX").setValue(m_cubicSegments.first().firstControlPoint().coordinate().x());
        pathNode.variantProperty("startY").setValue(m_cubicSegments.first().firstControlPoint().coordinate().y());


        foreach (const CubicSegment &cubicSegment, m_cubicSegments) {
            writePathAttributes(pathNode, cubicSegment.attributes());
            writePathPercent(pathNode, cubicSegment.percent());
            writeCubicPath(pathNode, cubicSegment);
        }

        writePathAttributes(pathNode, m_lastAttributes);
        writePathPercent(pathNode, m_lastPercent);
    }

    rewriterTransaction.commit();
}

void PathItem::setFormEditorItem(FormEditorItem *formEditorItem)
{
    m_formEditorItem = formEditorItem;
    setTransform(formEditorItem->sceneTransform());
    updatePath();

//    m_textEdit->setPlainText(m_formEditorItem->qmlItemNode().modelValue("path").toString());
}

static bool hasPath(FormEditorItem *formEditorItem)
{
    ModelNode modelNode = formEditorItem->qmlItemNode().modelNode();

    return modelNode.hasProperty("path") && modelNode.property("path").isNodeProperty();
}

QPointF startPoint(const ModelNode &modelNode)
{
    QPointF point;

    if (modelNode.hasProperty("startX"))
        point.setX(modelNode.variantProperty("startX").value().toDouble());

    if (modelNode.hasProperty("startY"))
        point.setY(modelNode.variantProperty("startY").value().toDouble());

    return point;
}

static void addCubicSegmentToPainterPath(const CubicSegment &cubicSegment, QPainterPath &painterPath)
{
        painterPath.cubicTo(cubicSegment.secondControlPoint().coordinate(),
                            cubicSegment.thirdControlPoint().coordinate(),
                            cubicSegment.fourthControlPoint().coordinate());

}

static void drawCubicSegments(const QList<CubicSegment> &cubicSegments, QPainter *painter)
{
    painter->save();

    QPainterPath curvePainterPath(cubicSegments.first().firstControlPoint().coordinate());

    foreach (const CubicSegment &cubicSegment, cubicSegments)
        addCubicSegmentToPainterPath(cubicSegment, curvePainterPath);

    painter->setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter->drawPath(curvePainterPath);

    painter->restore();
}

static void drawControlLine(const CubicSegment &cubicSegment, QPainter *painter)
{
    static const QPen solidPen(QColor(104, 183, 214), 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    painter->setPen(solidPen);
    painter->drawLine(cubicSegment.firstControlPoint().coordinate(),
                      cubicSegment.secondControlPoint().coordinate());

    QVector<double> dashVector;
    dashVector.append(4);
    dashVector.append(4);
    QPen dashedPen(QColor(104, 183, 214), 1, Qt::CustomDashLine, Qt::FlatCap, Qt::MiterJoin);
    dashedPen.setDashPattern(dashVector);
    painter->setPen(dashedPen);
    painter->drawLine(cubicSegment.secondControlPoint().coordinate(),
                      cubicSegment.thirdControlPoint().coordinate());

    painter->setPen(QPen(QColor(104, 183, 214), 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter->drawLine(cubicSegment.thirdControlPoint().coordinate(),
                      cubicSegment.fourthControlPoint().coordinate());
}

static void drawControlLines(const QList<CubicSegment> &cubicSegments, QPainter *painter)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    foreach (const CubicSegment &cubicSegment, cubicSegments)
            drawControlLine(cubicSegment, painter);

    painter->restore();
}

static QRectF controlPointShape(-2, -2, 5, 5);

static void drawControlPoint(const ControlPoint &controlPoint, const QList<ControlPoint> &selectionPoints, QPainter *painter)
{
    static const QColor editPointColor(0, 110, 255);
    static const QColor controlVertexColor(0, 110, 255);
    static const QColor selectionPointColor(0, 255, 0);

    double originX = controlPoint.coordinate().x();
    double originY = controlPoint.coordinate().y();

    if (controlPoint.isEditPoint()) {
        if (selectionPoints.contains(controlPoint)) {
            painter->setBrush(selectionPointColor);
            painter->setPen(selectionPointColor);
        } else {
            painter->setBrush(editPointColor);
            painter->setPen(editPointColor);
        }
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->drawRect(controlPointShape.adjusted(originX -1, originY - 1, originX - 1, originY - 1));
        painter->setRenderHint(QPainter::Antialiasing, true);
    } else {
        if (selectionPoints.contains(controlPoint)) {
            painter->setBrush(selectionPointColor);
            painter->setPen(selectionPointColor);
        } else {
            painter->setBrush(controlVertexColor);
            painter->setPen(controlVertexColor);
        }
        painter->drawEllipse(controlPointShape.adjusted(originX, originY, originX, originY));
    }
}

static void drawControlPoints(const QList<ControlPoint> &controlPoints, const QList<ControlPoint> &selectionPoints, QPainter *painter)
{
    painter->save();

    foreach (const ControlPoint &controlPoint, controlPoints)
            drawControlPoint(controlPoint, selectionPoints, painter);

    painter->restore();
}

static void drawPositionOverlay(const ControlPoint &controlPoint, QPainter *painter)
{
    QPoint position = controlPoint.coordinate().toPoint();
    position.rx() += 3;
    position.ry() -= 3;

    QString postionText(QString(QLatin1String("x: %1 y: %2")).arg(controlPoint.coordinate().x()).arg(controlPoint.coordinate().y()));
    painter->drawText(position, postionText);
}

static void drawPostionOverlays(const QList<SelectionPoint> &selectedPoints, QPainter *painter)
{
    painter->save();
    QFont font = painter->font();
    font.setPixelSize(9);
    painter->setFont(font);
    painter->setPen(QColor(0, 0, 0));

    foreach (const SelectionPoint &selectedPoint, selectedPoints)
        drawPositionOverlay(selectedPoint.controlPoint, painter);

    painter->restore();
}

static void drawMultiSelectionRectangle(const QRectF &selectionRectangle, QPainter *painter)
{
    painter->save();
    static QColor selectionBrush(painter->pen().color());
    selectionBrush.setAlpha(50);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setBrush(selectionBrush);
    painter->drawRect(selectionRectangle);
    painter->restore();
}

void PathItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (!m_cubicSegments.isEmpty()) {
        drawCubicSegments(m_cubicSegments, painter);
        drawControlLines(m_cubicSegments, painter);
        drawControlPoints(controlPoints(), m_selectionManipulator.allControlPoints(), painter);
        drawPostionOverlays(m_selectionManipulator.singleSelectedPoints(), painter);
        if (m_selectionManipulator.isMultiSelecting())
            drawMultiSelectionRectangle(m_selectionManipulator.multiSelectionRectangle(), painter);
    }

    painter->restore();
}

FormEditorItem *PathItem::formEditorItem() const
{
    return m_formEditorItem;
}

static CubicSegment createCubicSegmentForLine(const ModelNode &lineNode, const ControlPoint &startControlPoint)
{
    CubicSegment cubicSegment = CubicSegment::create();
    cubicSegment.setModelNode(lineNode);

    if (lineNode.hasProperty("x")
            && lineNode.hasProperty("y")) {

        QPointF controlPoint0Line = startControlPoint.coordinate();
        QPointF controlPoint1Line(lineNode.variantProperty("x").value().toDouble(),
                                  lineNode.variantProperty("y").value().toDouble());

        QPointF controlPoint1Cubic = controlPoint0Line + (1./3.) * (controlPoint1Line - controlPoint0Line);
        QPointF controlPoint2Cubic = controlPoint0Line + (2./3.) * (controlPoint1Line - controlPoint0Line);

        cubicSegment.setFirstControlPoint(startControlPoint);
        cubicSegment.setSecondControlPoint(controlPoint1Cubic);
        cubicSegment.setThirdControlPoint(controlPoint2Cubic);
        cubicSegment.setFourthControlPoint(controlPoint1Line);
    } else {
        qWarning() << "PathLine has not all entries!";
    }

    return cubicSegment;
}

static CubicSegment createCubicSegmentForQuad(const ModelNode &quadNode, const ControlPoint &startControlPoint)
{
    CubicSegment cubicSegment = CubicSegment::create();
    cubicSegment.setModelNode(quadNode);

    if (quadNode.hasProperty("controlX")
            && quadNode.hasProperty("controlY")
            && quadNode.hasProperty("x")
            && quadNode.hasProperty("y")) {
        QPointF controlPoint0Quad = startControlPoint.coordinate();
        QPointF controlPoint1Quad(quadNode.variantProperty("controlX").value().toDouble(),
                                 quadNode.variantProperty("controlY").value().toDouble());
        QPointF controlPoint2Quad(quadNode.variantProperty("x").value().toDouble(),
                                  quadNode.variantProperty("y").value().toDouble());

        QPointF controlPoint1Cubic = controlPoint0Quad + (2./3.) * (controlPoint1Quad - controlPoint0Quad);
        QPointF controlPoint2Cubic = controlPoint2Quad + (2./3.) * (controlPoint1Quad - controlPoint2Quad);

        cubicSegment.setFirstControlPoint(startControlPoint);
        cubicSegment.setSecondControlPoint(controlPoint1Cubic);
        cubicSegment.setThirdControlPoint(controlPoint2Cubic);
        cubicSegment.setFourthControlPoint(controlPoint2Quad);
    } else {
        qWarning() << "PathQuad has not all entries!";
    }

    return cubicSegment;
}

static CubicSegment createCubicSegmentForCubic(const ModelNode &cubicNode, const ControlPoint &startControlPoint)
{
    CubicSegment cubicSegment = CubicSegment::create();
    cubicSegment.setModelNode(cubicNode);

    if (cubicNode.hasProperty("control1X")
            && cubicNode.hasProperty("control1Y")
            && cubicNode.hasProperty("control2X")
            && cubicNode.hasProperty("control2Y")
            && cubicNode.hasProperty("x")
            && cubicNode.hasProperty("y")) {

        cubicSegment.setFirstControlPoint(startControlPoint);
        cubicSegment.setSecondControlPoint(cubicNode.variantProperty("control1X").value().toDouble(),
                                           cubicNode.variantProperty("control1Y").value().toDouble());
        cubicSegment.setThirdControlPoint(cubicNode.variantProperty("control2X").value().toDouble(),
                                          cubicNode.variantProperty("control2Y").value().toDouble());
        cubicSegment.setFourthControlPoint(cubicNode.variantProperty("x").value().toDouble(),
                                           cubicNode.variantProperty("y").value().toDouble());
    } else {
        qWarning() << "PathCubic has not all entries!";
    }

    return cubicSegment;
}

static QRectF boundingRectForPath(const QList<ControlPoint> &controlPoints)
{
    double xMinimum = 0.;
    double xMaximum = 0.;
    double yMinimum = 0.;
    double yMaximum = 0.;

    foreach (const ControlPoint & controlPoint, controlPoints) {
        xMinimum = qMin(xMinimum, controlPoint.coordinate().x());
        xMaximum = qMax(xMaximum, controlPoint.coordinate().x());
        yMinimum = qMin(yMinimum, controlPoint.coordinate().y());
        yMaximum = qMax(yMaximum, controlPoint.coordinate().y());
    }

    return QRect(xMinimum, yMinimum, xMaximum - xMinimum, yMaximum - yMinimum);
}

void PathItem::updateBoundingRect()
{
    QRectF controlBoundingRect = boundingRectForPath(controlPoints()).adjusted(-100, -100, 200, 100);

    if (m_selectionManipulator.isMultiSelecting())
        controlBoundingRect = controlBoundingRect.united(m_selectionManipulator.multiSelectionRectangle());

    setBoundingRect(instanceBoundingRect().united(controlBoundingRect));
}

QRectF PathItem::instanceBoundingRect() const
{
    if (formEditorItem())
        return formEditorItem()->qmlItemNode().instanceBoundingRect();

    return QRectF();
}

void PathItem::readControlPoints()
{
    ModelNode pathNode = pathModelNode(formEditorItem());

    m_cubicSegments.clear();

    if (pathNode.hasNodeListProperty("pathElements")) {
        ControlPoint firstControlPoint(startPoint(pathNode));
        firstControlPoint.setPathModelNode(pathNode);
        firstControlPoint.setPointType(StartPoint);

        QMap<QString, QVariant> actualAttributes;
        double percent = -1.0;

        foreach (const ModelNode &childNode, pathNode.nodeListProperty("pathElements").toModelNodeList()) {

            if (childNode.type() == "QtQuick.PathAttribute") {
                actualAttributes.insert(childNode.variantProperty("name").value().toString(), childNode.variantProperty("value").value());
            } else if (childNode.type() == "QtQuick.PathPercent") {
                percent = childNode.variantProperty("value").value().toDouble();
            } else {
                CubicSegment newCubicSegement;

                if (childNode.type() == "QtQuick.PathLine")
                    newCubicSegement = createCubicSegmentForLine(childNode, firstControlPoint);
                else if (childNode.type() == "QtQuick.PathQuad")
                    newCubicSegement = createCubicSegmentForQuad(childNode, firstControlPoint);
                else if (childNode.type() == "QtQuick.PathCubic")
                    newCubicSegement = createCubicSegmentForCubic(childNode, firstControlPoint);
                else
                    continue;

                newCubicSegement.setPercent(percent);
                newCubicSegement.setAttributes(actualAttributes);

                firstControlPoint = newCubicSegement.fourthControlPoint();
                qDebug() << "Can be converted to quad" << newCubicSegement.canBeConvertedToQuad();
                qDebug() << "Can be converted to line" << newCubicSegement.canBeConvertedToLine();
                m_cubicSegments.append(newCubicSegement);
                actualAttributes.clear();
                percent = -1.0;
            }
        }

        m_lastAttributes = actualAttributes;
        m_lastPercent = percent;

        if (m_cubicSegments.first().firstControlPoint().coordinate() == m_cubicSegments.last().fourthControlPoint().coordinate()) {
            CubicSegment lastCubicSegment = m_cubicSegments.last();
            lastCubicSegment.setFourthControlPoint(m_cubicSegments.first().firstControlPoint());
            lastCubicSegment.fourthControlPoint().setPathModelNode(pathNode);
            lastCubicSegment.fourthControlPoint().setPointType(StartAndEndPoint);
        }
    }
}

static CubicSegment getMinimumDistanceSegment(const QPointF &pickPoint, const QList<CubicSegment> &cubicSegments, double maximumDistance, double *t = 0)
{
    CubicSegment minimumDistanceSegment;
    double actualMinimumDistance = maximumDistance;

    foreach (const CubicSegment &cubicSegment, cubicSegments) {
        double tSegment = 0.;
        double cubicSegmentMinimumDistance = cubicSegment.minimumDistance(pickPoint, tSegment);
        if (cubicSegmentMinimumDistance < actualMinimumDistance) {
            minimumDistanceSegment = cubicSegment;
            actualMinimumDistance = cubicSegmentMinimumDistance;
            if (t)
                *t = tSegment;
        }
    }

    return minimumDistanceSegment;
}

void PathItem::splitCubicSegment(CubicSegment &cubicSegment, double t)
{
    QPair<CubicSegment, CubicSegment> newCubicSegmentPair = cubicSegment.split(t);
    int indexOfOldCubicSegment = m_cubicSegments.indexOf(cubicSegment);

    m_cubicSegments.removeAt(indexOfOldCubicSegment);
    m_cubicSegments.insert(indexOfOldCubicSegment, newCubicSegmentPair.first);
    m_cubicSegments.insert(indexOfOldCubicSegment + 1, newCubicSegmentPair.second);
}

void PathItem::closePath()
{
    if (!m_cubicSegments.isEmpty()) {
        CubicSegment firstCubicSegment = m_cubicSegments.first();
        CubicSegment lastCubicSegment = m_cubicSegments.last();
        lastCubicSegment.setFourthControlPoint(firstCubicSegment.firstControlPoint());
        writePathAsCubicSegmentsOnly();
    }
}

void PathItem::openPath()
{
    if (!m_cubicSegments.isEmpty()) {
        CubicSegment firstCubicSegment = m_cubicSegments.first();
        CubicSegment lastCubicSegment = m_cubicSegments.last();
        QPointF newEndPoint = firstCubicSegment.firstControlPoint().coordinate();
        newEndPoint.setX(newEndPoint.x() + 10.);
        lastCubicSegment.setFourthControlPoint(ControlPoint(newEndPoint));
        writePathAsCubicSegmentsOnly();
    }
}

QAction *PathItem::createClosedPathAction(QMenu *contextMenu) const
{
    QAction *closedPathAction = new QAction(contextMenu);
    closedPathAction->setCheckable(true);
    closedPathAction->setChecked(isClosedPath());
    closedPathAction->setText(tr("Closed Path"));
    contextMenu->addAction(closedPathAction);

    if (m_cubicSegments.count() == 1)
        closedPathAction->setDisabled(true);

    return closedPathAction;
}

void PathItem::createGlobalContextMenu(const QPoint &menuPosition)
{
    QMenu contextMenu;

    QAction *closedPathAction = createClosedPathAction(&contextMenu);

    QAction *activatedAction = contextMenu.exec(menuPosition);

    if (activatedAction == closedPathAction)
        makePathClosed(closedPathAction->isChecked());
}

void PathItem::createCubicSegmentContextMenu(CubicSegment &cubicSegment, const QPoint &menuPosition, double t)
{
    QMenu contextMenu;

    QAction *splitSegmentAction = new QAction(&contextMenu);
    splitSegmentAction->setText(tr("Split Segment"));
    contextMenu.addAction(splitSegmentAction);

    QAction *straightLinePointAction = new QAction(&contextMenu);
    straightLinePointAction->setText(tr("Make Curve Segment Straight"));
    contextMenu.addAction(straightLinePointAction);

    if (m_cubicSegments.count() == 1 && isClosedPath())
        straightLinePointAction->setDisabled(true);

    QAction *closedPathAction = createClosedPathAction(&contextMenu);

    QAction *activatedAction = contextMenu.exec(menuPosition);

    if (activatedAction == straightLinePointAction) {
        cubicSegment.makeStraightLine();
        PathUpdateDisabler pathItemDisabler(this, PathUpdateDisabler::DontUpdatePath);
        RewriterTransaction rewriterTransaction =
            cubicSegment.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("PathItem::createCubicSegmentContextMenu"));
        cubicSegment.updateModelNode();
        rewriterTransaction.commit();
    } else if (activatedAction == splitSegmentAction) {
        splitCubicSegment(cubicSegment, t);
        writePathAsCubicSegmentsOnly();
    } else if (activatedAction == closedPathAction) {
        makePathClosed(closedPathAction->isChecked());
    }
}


void PathItem::createEditPointContextMenu(const ControlPoint &controlPoint, const QPoint &menuPosition)
{
    QMenu contextMenu;
    QAction *removeEditPointAction = new QAction(&contextMenu);
    removeEditPointAction->setText(tr("Remove Edit Point"));
    contextMenu.addAction(removeEditPointAction);

    QAction *closedPathAction = createClosedPathAction(&contextMenu);

    if (m_cubicSegments.count() <= 1 || (m_cubicSegments.count() == 2 && isClosedPath()))
        removeEditPointAction->setDisabled(true);

    QAction *activatedAction = contextMenu.exec(menuPosition);

    if (activatedAction == removeEditPointAction)
        removeEditPoint(controlPoint);
    else if (activatedAction == closedPathAction)
        makePathClosed(closedPathAction->isChecked());
}

const QList<ControlPoint> PathItem::controlPoints() const
{
    QList<ControlPoint> controlPointList;
    controlPointList.reserve((m_cubicSegments.count() * 4));

    if (!m_cubicSegments.isEmpty())
        controlPointList.append(m_cubicSegments.first().firstControlPoint());

    foreach (const CubicSegment &cubicSegment, m_cubicSegments) {
        controlPointList.append(cubicSegment.secondControlPoint());
        controlPointList.append(cubicSegment.thirdControlPoint());
        controlPointList.append(cubicSegment.fourthControlPoint());
    }

    if (isClosedPath())
        controlPointList.pop_back();

    return controlPointList;
}

bool hasLineOrQuadPathElements(const QList<ModelNode> &modelNodes)
{
    foreach (const ModelNode &modelNode, modelNodes) {
        if (modelNode.type() == "QtQuick.PathLine"
                || modelNode.type() == "QtQuick.PathQuad")
            return true;
    }

    return false;
}

void PathItem::updatePath()
{
    if (m_dontUpdatePath)
        return;

    if (hasPath(formEditorItem())) {
        readControlPoints();

        ModelNode pathNode = pathModelNode(formEditorItem());

        if (hasLineOrQuadPathElements(pathNode.nodeListProperty("pathElements").toModelNodeList()))
            writePathAsCubicSegmentsOnly();
    }

    updateBoundingRect();
    update();
}

QRectF PathItem::boundingRect() const
{
    return m_boundingRect;
}

void PathItem::setBoundingRect(const QRectF &boundingRect)
{
    m_boundingRect = boundingRect;
}

static bool controlPointIsNearMousePosition(const ControlPoint &controlPoint, const QPointF &mousePosition)
{
    QPointF distanceVector = controlPoint.coordinate() - mousePosition;

    if (distanceVector.manhattanLength() < 10)
        return true;

    return false;
}

static bool controlPointsAreNearMousePosition(const QList<ControlPoint> &controlPoints, const QPointF &mousePosition)
{
    foreach (const ControlPoint &controlPoint, controlPoints) {
        if (controlPointIsNearMousePosition(controlPoint, mousePosition))
            return true;
    }

    return false;
}

static ControlPoint pickControlPoint(const QList<ControlPoint> &controlPoints, const QPointF &mousePosition)
{
    foreach (const ControlPoint &controlPoint, controlPoints) {
        if (controlPointIsNearMousePosition(controlPoint, mousePosition))
            return controlPoint;
    }

    return ControlPoint();
}

void PathItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_selectionManipulator.hasMultiSelection()) {
            m_selectionManipulator.setStartPoint(event->pos());
        } else {
            ControlPoint pickedControlPoint = pickControlPoint(controlPoints(), event->pos());

            if (pickedControlPoint.isValid()) {
                m_selectionManipulator.addSingleControlPointSmartly(pickedControlPoint);
                m_selectionManipulator.startMoving(event->pos());
            } else {
                m_selectionManipulator.startMultiSelection(event->pos());
            }
        }
    }
}

bool hasMoveStartDistance(const QPointF &startPoint, const QPointF &updatePoint)
{
    return (startPoint - updatePoint).manhattanLength() > 10;
}

void PathItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (controlPointsAreNearMousePosition(controlPoints(), event->pos()))
        setCursor(Qt::SizeAllCursor);
    else
        setCursor(Qt::ArrowCursor);

    PathUpdateDisabler pathUpdateDisabler(this, PathUpdateDisabler::DontUpdatePath);
    if (event->buttons().testFlag(Qt::LeftButton)) {
        if (m_selectionManipulator.isMultiSelecting()) {
            m_selectionManipulator.updateMultiSelection(event->pos());
            update();
        } else if (m_selectionManipulator.hasSingleSelection()) {
            setCursor(Qt::SizeAllCursor);
            m_selectionManipulator.updateMoving(event->pos(), event->modifiers());
            updatePathModelNodes(m_selectionManipulator.allSelectionSinglePoints());
            updateBoundingRect();
            update();
        } else if (m_selectionManipulator.hasMultiSelection()) {
            setCursor(Qt::SizeAllCursor);
            if (m_selectionManipulator.isMoving()) {
                m_selectionManipulator.updateMoving(event->pos(), event->modifiers());
                updatePathModelNodes(m_selectionManipulator.allSelectionSinglePoints());
                updateBoundingRect();
                update();
            } else if (hasMoveStartDistance(m_selectionManipulator.startPoint(), event->pos())) {
                m_selectionManipulator.startMoving(m_selectionManipulator.startPoint());
                m_selectionManipulator.updateMoving(event->pos(), event->modifiers());
                updatePathModelNodes(m_selectionManipulator.allSelectionSinglePoints());
                updateBoundingRect();
                update();
            }
        }
    }
}

void PathItem::updatePathModelNodes(const QList<SelectionPoint> &changedPoints)
{
    PathUpdateDisabler pathUpdateDisabler(this, PathUpdateDisabler::DontUpdatePath);

    RewriterTransaction rewriterTransaction =
        formEditorItem()->qmlItemNode().view()->beginRewriterTransaction(QByteArrayLiteral("PathItem::createCubicSegmentContextMenu"));

    foreach (SelectionPoint changedPoint, changedPoints)
        changedPoint.controlPoint.updateModelNode();

    rewriterTransaction.commit();
}

void PathItem::disablePathUpdates()
{
    m_dontUpdatePath = true;
}

void PathItem::enablePathUpdates()
{
    m_dontUpdatePath = false;
}

bool PathItem::pathUpdatesDisabled() const
{
    return m_dontUpdatePath;
}

void PathItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_selectionManipulator.isMultiSelecting()) {
            m_selectionManipulator.updateMultiSelection(event->pos());
            m_selectionManipulator.endMultiSelection();
        } else if (m_selectionManipulator.hasSingleSelection()) {
            m_selectionManipulator.updateMoving(event->pos(), event->modifiers());
            updatePathModelNodes(m_selectionManipulator.allSelectionSinglePoints());
            updateBoundingRect();
            m_selectionManipulator.clearSingleSelection();
        } else if (m_selectionManipulator.hasMultiSelection()) {
            if (m_selectionManipulator.isMoving()) {
                m_selectionManipulator.updateMoving(event->pos(), event->modifiers());
                m_selectionManipulator.endMoving();
                updatePathModelNodes(m_selectionManipulator.multiSelectedPoints());
                updateBoundingRect();
            } else {
                m_selectionManipulator.clearMultiSelection();
            }
        }
    } else if (event->button() == Qt::RightButton) {
        ControlPoint pickedControlPoint = pickControlPoint(controlPoints(), event->pos());
        if (pickedControlPoint.isEditPoint()) {
            createEditPointContextMenu(pickedControlPoint, event->screenPos());
        } else {
            double t = 0.0;
            CubicSegment minimumDistanceSegment = getMinimumDistanceSegment(event->pos(), m_cubicSegments, 20., &t);
            if (minimumDistanceSegment.isValid())
                createCubicSegmentContextMenu(minimumDistanceSegment, event->screenPos(), t);
            else
                createGlobalContextMenu(event->screenPos());
        }
    }

    update();

}

bool PathItem::isClosedPath() const
{
    if (m_cubicSegments.isEmpty())
        return false;

    ControlPoint firstControlPoint = m_cubicSegments.first().firstControlPoint();
    ControlPoint lastControlPoint = m_cubicSegments.last().fourthControlPoint();

    return firstControlPoint == lastControlPoint;
}

void PathItem::makePathClosed(bool pathShoudlBeClosed)
{
    if (pathShoudlBeClosed && !isClosedPath())
        closePath();
    else if (!pathShoudlBeClosed && isClosedPath())
        openPath();
}

QList<CubicSegment> cubicSegmentsContainingControlPoint(const ControlPoint &controlPoint, const QList<CubicSegment> &allCubicSegments)
{
    QList<CubicSegment> cubicSegmentsHasControlPoint;

    foreach (const CubicSegment &cubicSegment, allCubicSegments) {
        if (cubicSegment.controlPoints().contains(controlPoint))
            cubicSegmentsHasControlPoint.append(cubicSegment);
    }

    return cubicSegmentsHasControlPoint;
}

void PathItem::removeEditPoint(const ControlPoint &controlPoint)
{
    QList<CubicSegment> cubicSegments = cubicSegmentsContainingControlPoint(controlPoint, m_cubicSegments);

    if (cubicSegments.count() == 1) {
        m_cubicSegments.removeOne(cubicSegments.first());
    } else if (cubicSegments.count()  == 2){
        CubicSegment mergedCubicSegment = CubicSegment::create();
        CubicSegment firstCubicSegment = cubicSegments.at(0);
        CubicSegment secondCubicSegment = cubicSegments.at(1);
        mergedCubicSegment.setFirstControlPoint(firstCubicSegment.firstControlPoint());
        mergedCubicSegment.setSecondControlPoint(firstCubicSegment.secondControlPoint());
        mergedCubicSegment.setThirdControlPoint(secondCubicSegment.thirdControlPoint());
        mergedCubicSegment.setFourthControlPoint(secondCubicSegment.fourthControlPoint());

        int indexOfFirstCubicSegment = m_cubicSegments.indexOf(firstCubicSegment);
        m_cubicSegments.removeAt(indexOfFirstCubicSegment);
        m_cubicSegments.removeAt(indexOfFirstCubicSegment);
        m_cubicSegments.insert(indexOfFirstCubicSegment, mergedCubicSegment);
    }

    writePathAsCubicSegmentsOnly();
}

PathUpdateDisabler::PathUpdateDisabler(PathItem *pathItem, PathUpdate updatePath)
    : m_pathItem(pathItem),
      m_updatePath(updatePath)
{
    pathItem->disablePathUpdates();
}

PathUpdateDisabler::~PathUpdateDisabler()
{
    m_pathItem->enablePathUpdates();
    if (m_updatePath == UpdatePath)
        m_pathItem->updatePath();
}

}
