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

#include "formeditoritem.h"
#include "formeditorscene.h"

#include <variantproperty.h>
#include <bindingproperty.h>

#include <modelnode.h>
#include <nodehints.h>
#include <nodemetainfo.h>

#include <theme.h>

#include <utils/algorithm.h>
#include <utils/theme/theme.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFontDatabase>
#include <QtGlobal>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#include <QTimeLine>
#include <QGraphicsView>

#include <cmath>

namespace QmlDesigner {

const int flowBlockSize = 200;
const int blockRadius = 18;
const int blockAdjust = 40;
const int startItemOffset = 96;
const int labelFontSize = 16;

void drawIcon(QPainter *painter,
              int x,
              int y,
              const QString &iconSymbol,
              int fontSize,
              int iconSize,
              const QColor &penColor)
{
    static QFontDatabase a;

    const QString fontName = "qtds_propertyIconFont.ttf";

    Q_ASSERT(a.hasFamily(fontName));

    if (a.hasFamily(fontName)) {
        QFont font(fontName);
        font.setPixelSize(fontSize);

        painter->save();
        painter->setPen(penColor);
        painter->setFont(font);
        painter->drawText(QRectF(x, y, iconSize, iconSize), iconSymbol);

        painter->restore();
    }
}

FormEditorScene *FormEditorItem::scene() const {
    return qobject_cast<FormEditorScene*>(QGraphicsItem::scene());
}

FormEditorItem::FormEditorItem(const QmlItemNode &qmlItemNode, FormEditorScene* scene)
    : QGraphicsItem(scene->formLayerItem()),
    m_snappingLineCreator(this),
    m_qmlItemNode(qmlItemNode),
    m_borderWidth(1.0),
    m_highlightBoundingRect(false),
    m_blurContent(false),
    m_isContentVisible(true),
    m_isFormEditorVisible(true)
{
    setCacheMode(QGraphicsItem::NoCache);
    setup();
}

void FormEditorItem::setup()
{
    setAcceptedMouseButtons(Qt::NoButton);
    if (qmlItemNode().hasInstanceParent()) {
        setParentItem(scene()->itemForQmlItemNode(qmlItemNode().instanceParent().toQmlItemNode()));
        setOpacity(qmlItemNode().instanceValue("opacity").toDouble());
    }

    setFlag(QGraphicsItem::ItemClipsChildrenToShape, qmlItemNode().instanceValue("clip").toBool());

    if (NodeHints::fromModelNode(qmlItemNode()).forceClip())
        setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    if (QGraphicsItem::parentItem() == scene()->formLayerItem())
        m_borderWidth = 0.0;

    setContentVisible(qmlItemNode().instanceValue("visible").toBool());

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemNegativeZStacksBehindParent, true);
    updateGeometry();
    updateVisibilty();
}

QRectF FormEditorItem::boundingRect() const
{
    return m_boundingRect;
}

QPainterPath FormEditorItem::shape() const
{
    QPainterPath painterPath;
    painterPath.addRect(m_selectionBoundingRect);

    return painterPath;
}

bool FormEditorItem::contains(const QPointF &point) const
{
    return m_selectionBoundingRect.contains(point);
}

void FormEditorItem::updateGeometry()
{
    prepareGeometryChange();
    m_selectionBoundingRect = qmlItemNode().instanceBoundingRect().adjusted(0, 0, 1., 1.);
    m_paintedBoundingRect = qmlItemNode().instancePaintedBoundingRect();
    m_boundingRect = qmlItemNode().instanceBoundingRect();
    setTransform(qmlItemNode().instanceTransformWithContentTransform());
    // the property for zValue is called z in QGraphicsObject
    if (qmlItemNode().instanceValue("z").isValid() && !qmlItemNode().isRootModelNode())
        setZValue(qmlItemNode().instanceValue("z").toDouble());
}

void FormEditorItem::updateVisibilty()
{
}

FormEditorView *FormEditorItem::formEditorView() const
{
    return scene()->editorView();
}

void FormEditorItem::setHighlightBoundingRect(bool highlight)
{
    if (m_highlightBoundingRect != highlight) {
        m_highlightBoundingRect = highlight;
        update();
    }
}

void FormEditorItem::blurContent(bool blurContent)
{
    if (!scene())
        return;

    if (m_blurContent != blurContent) {
        m_blurContent = blurContent;
        update();
    }
}

void FormEditorItem::setContentVisible(bool visible)
{
    if (visible == m_isContentVisible)
        return;

    m_isContentVisible = visible;
    update();
}

bool FormEditorItem::isContentVisible() const
{
    if (parentItem())
        return parentItem()->isContentVisible() && m_isContentVisible;

    return m_isContentVisible;
}


bool FormEditorItem::isFormEditorVisible() const
{
    return m_isFormEditorVisible;
}
void FormEditorItem::setFormEditorVisible(bool isVisible)
{
    m_isFormEditorVisible = isVisible;
    setVisible(isVisible);
}

QPointF FormEditorItem::center() const
{
    return mapToScene(qmlItemNode().instanceBoundingRect().center());
}

qreal FormEditorItem::selectionWeigth(const QPointF &point, int iteration)
{
    if (!qmlItemNode().isValid())
        return 100000;

    QRectF boundingRect = mapRectToScene(qmlItemNode().instanceBoundingRect());

    float weight = point.x()- boundingRect.left()
            + point.y() - boundingRect.top()
            + boundingRect.right()- point.x()
            + boundingRect.bottom() - point.y()
            + (center() - point).manhattanLength()
            + sqrt(boundingRect.width() * boundingRect.height()) / 2 * iteration;

    return weight;
}

void FormEditorItem::synchronizeOtherProperty(const QByteArray &propertyName)
{
    if (propertyName == "opacity")
        setOpacity(qmlItemNode().instanceValue("opacity").toDouble());

    if (propertyName == "clip")
        setFlag(QGraphicsItem::ItemClipsChildrenToShape, qmlItemNode().instanceValue("clip").toBool());

    if (NodeHints::fromModelNode(qmlItemNode()).forceClip())
        setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    if (propertyName == "z")
        setZValue(qmlItemNode().instanceValue("z").toDouble());

    if (propertyName == "visible")
        setContentVisible(qmlItemNode().instanceValue("visible").toBool());
}

void FormEditorItem::setDataModelPosition(const QPointF &position)
{
    qmlItemNode().setPosition(position);
}

void FormEditorItem::setDataModelPositionInBaseState(const QPointF &position)
{
    qmlItemNode().setPostionInBaseState(position);
}

QPointF FormEditorItem::instancePosition() const
{
    return qmlItemNode().instancePosition();
}

QTransform FormEditorItem::instanceSceneTransform() const
{
    return qmlItemNode().instanceSceneTransform();
}

QTransform FormEditorItem::instanceSceneContentItemTransform() const
{
    return qmlItemNode().instanceSceneContentItemTransform();
}

bool FormEditorItem::flowHitTest(const QPointF & ) const
{
    return false;
}

FormEditorItem::~FormEditorItem()
{
    scene()->removeItemFromHash(this);
}

/* \brief returns the parent item skipping all proxyItem*/
FormEditorItem *FormEditorItem::parentItem() const
{
    return qgraphicsitem_cast<FormEditorItem*> (QGraphicsItem::parentItem());
}

FormEditorItem* FormEditorItem::fromQGraphicsItem(QGraphicsItem *graphicsItem)
{
    return qgraphicsitem_cast<FormEditorItem*>(graphicsItem);
}

void FormEditorItem::paintBoundingRect(QPainter *painter) const
{
    if (!m_boundingRect.isValid()
        || (QGraphicsItem::parentItem() == scene()->formLayerItem() && qFuzzyIsNull(m_borderWidth)))
          return;

     if (m_boundingRect.width() < 8 || m_boundingRect.height() < 8)
         return;

    QPen pen;
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::MiterJoin);

    const QColor frameColor(0xaa, 0xaa, 0xaa);
    static const QColor selectionColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorSelectionColor);

    if (scene()->showBoundingRects()) {
        pen.setColor(frameColor.darker(150));
        pen.setStyle(Qt::DotLine);
        painter->setPen(pen);
        painter->drawRect(m_boundingRect.adjusted(0., 0., -1., -1.));

    }

    if (m_highlightBoundingRect) {
        pen.setColor(selectionColor);
        pen.setStyle(Qt::SolidLine);
        painter->setPen(pen);
        painter->drawRect(m_selectionBoundingRect.adjusted(0., 0., -1., -1.));
    }
}

static void paintTextInPlaceHolderForInvisbleItem(QPainter *painter,
                                                  const QString &id,
                                                  const QString &typeName,
                                                  const QRectF &boundingRect)
{
    QString displayText = id.isEmpty() ? typeName : id;

    QTextOption textOption;
    textOption.setAlignment(Qt::AlignTop);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    if (boundingRect.height() > 60) {
        QFont font;
        font.setStyleHint(QFont::SansSerif);
        font.setBold(true);
        font.setPixelSize(12);
        painter->setFont(font);

        QFontMetrics fm(font);
        painter->rotate(90);
        if (fm.horizontalAdvance(displayText) > (boundingRect.height() - 32) && displayText.length() > 4) {

            displayText = fm.elidedText(displayText, Qt::ElideRight, boundingRect.height() - 32, Qt::TextShowMnemonic);
        }

        QRectF rotatedBoundingBox;
        rotatedBoundingBox.setWidth(boundingRect.height());
        rotatedBoundingBox.setHeight(12);
        rotatedBoundingBox.setY(-boundingRect.width() + 12);
        rotatedBoundingBox.setX(20);

        painter->setFont(font);
        painter->setPen(QColor(48, 48, 96, 255));
        painter->setClipping(false);
        painter->drawText(rotatedBoundingBox, displayText, textOption);
    }
}

void paintDecorationInPlaceHolderForInvisbleItem(QPainter *painter, const QRectF &boundingRect)
{
    qreal stripesWidth = 8;

    QRegion innerRegion = QRegion(boundingRect.adjusted(stripesWidth, stripesWidth, -stripesWidth, -stripesWidth).toRect());
    QRegion outerRegion  = QRegion(boundingRect.toRect()) - innerRegion;

    painter->setClipRegion(outerRegion);
    painter->setClipping(true);
    painter->fillRect(boundingRect.adjusted(1, 1, -1, -1), Qt::BDiagPattern);
}

void FormEditorItem::paintPlaceHolderForInvisbleItem(QPainter *painter) const
{
    painter->save();
    paintDecorationInPlaceHolderForInvisbleItem(painter, m_boundingRect);
    paintTextInPlaceHolderForInvisbleItem(painter, qmlItemNode().id(), qmlItemNode().simplifiedTypeName(), m_boundingRect);
    painter->restore();
}

void FormEditorItem::paintComponentContentVisualisation(QPainter *painter, const QRectF &clippinRectangle) const
{
    painter->setBrush(QColor(0, 0, 0, 150));
    painter->fillRect(clippinRectangle, Qt::BDiagPattern);
}

QList<FormEditorItem *> FormEditorItem::offspringFormEditorItemsRecursive(const FormEditorItem *formEditorItem) const
{
    QList<FormEditorItem*> formEditorItemList;

    for (QGraphicsItem *item : formEditorItem->childItems()) {
        FormEditorItem *formEditorItem = fromQGraphicsItem(item);
        if (formEditorItem) {
            formEditorItemList.append(formEditorItem);
        }
    }

    return formEditorItemList;
}

void FormEditorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!painter->isActive())
        return;

    if (!qmlItemNode().isValid())
        return;

    painter->save();

    bool showPlaceHolder = qmlItemNode().instanceIsRenderPixmapNull() || !isContentVisible();

    const bool isInStackedContainer = qmlItemNode().isInStackedContainer();

    /* If already the parent is invisible then show nothing */
    const bool hideCompletely = !isContentVisible() && (parentItem() && !parentItem()->isContentVisible());

    if (isInStackedContainer)
        showPlaceHolder = qmlItemNode().instanceIsRenderPixmapNull() && isContentVisible();

    QRegion clipRegion = painter->clipRegion();
    if (clipRegion.contains(m_selectionBoundingRect.toRect().topLeft())
            && clipRegion.contains(m_selectionBoundingRect.toRect().bottomRight()))
        painter->setClipRegion(boundingRect().toRect());
    painter->setClipping(true);

    if (!hideCompletely) {
        if (showPlaceHolder) {
            if (scene()->showBoundingRects() && m_boundingRect.width() > 15 && m_boundingRect.height() > 15)
                paintPlaceHolderForInvisbleItem(painter);
        } else if (!isInStackedContainer || isContentVisible() ) {
            painter->save();
            const QTransform &painterTransform = painter->transform();
            if (painterTransform.m11() < 1.0 // horizontally scaled down?
                    || painterTransform.m22() < 1.0 // vertically scaled down?
                    || painterTransform.isRotating())
                painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

            if (m_blurContent)
                painter->drawPixmap(m_paintedBoundingRect.topLeft(), qmlItemNode().instanceBlurredRenderPixmap());
            else
                painter->drawPixmap(m_paintedBoundingRect.topLeft(), qmlItemNode().instanceRenderPixmap());

            painter->restore();
        }
    }

    painter->setClipping(false);
    if (!qmlItemNode().isRootModelNode())
        paintBoundingRect(painter);

    painter->restore();
}

AbstractFormEditorTool* FormEditorItem::tool() const
{
    return static_cast<FormEditorScene*>(scene())->currentTool();
}

SnapLineMap FormEditorItem::topSnappingLines() const
{
    return m_snappingLineCreator.topLines();
}

SnapLineMap FormEditorItem::bottomSnappingLines() const
{
    return m_snappingLineCreator.bottomLines();
}

SnapLineMap FormEditorItem::leftSnappingLines() const
{
    return m_snappingLineCreator.leftLines();
}

SnapLineMap FormEditorItem::rightSnappingLines() const
{
    return m_snappingLineCreator.rightLines();
}

SnapLineMap FormEditorItem::horizontalCenterSnappingLines() const
{
    return m_snappingLineCreator.horizontalCenterLines();
}

SnapLineMap FormEditorItem::verticalCenterSnappingLines() const
{
    return m_snappingLineCreator.verticalCenterLines();
}

SnapLineMap FormEditorItem::topSnappingOffsets() const
{
    return m_snappingLineCreator.topOffsets();
}

SnapLineMap FormEditorItem::bottomSnappingOffsets() const
{
    return m_snappingLineCreator.bottomOffsets();
}

SnapLineMap FormEditorItem::leftSnappingOffsets() const
{
    return m_snappingLineCreator.leftOffsets();
}

SnapLineMap FormEditorItem::rightSnappingOffsets() const
{
    return m_snappingLineCreator.rightOffsets();
}


void FormEditorItem::updateSnappingLines(const QList<FormEditorItem*> &exceptionList,
                                         FormEditorItem *transformationSpaceItem)
{
    m_snappingLineCreator.update(exceptionList, transformationSpaceItem, this);
}

QList<FormEditorItem*> FormEditorItem::childFormEditorItems() const
{
    QList<FormEditorItem*> formEditorItemList;

    for (QGraphicsItem *item : childItems()) {
        FormEditorItem *formEditorItem = fromQGraphicsItem(item);
        if (formEditorItem)
            formEditorItemList.append(formEditorItem);
    }

    return formEditorItemList;
}

QList<FormEditorItem *> FormEditorItem::offspringFormEditorItems() const
{
    return offspringFormEditorItemsRecursive(this);
}

bool FormEditorItem::isContainer() const
{
    NodeMetaInfo nodeMetaInfo = qmlItemNode().modelNode().metaInfo();

    if (nodeMetaInfo.isValid())
        return !nodeMetaInfo.defaultPropertyIsComponent() && !nodeMetaInfo.isLayoutable();

    return true;
}

QmlItemNode FormEditorItem::qmlItemNode() const
{
    return m_qmlItemNode;
}

void FormEditorFlowItem::synchronizeOtherProperty(const QByteArray &)
{
    setContentVisible(true);
}

void FormEditorFlowItem::setDataModelPosition(const QPointF &position)
{
    qmlItemNode().setFlowItemPosition(position);
    updateGeometry();

/* TODO
    for (QGraphicsItem *item : scene()->items()) {
        if (item == this)
            continue;

        auto formEditorItem = qgraphicsitem_cast<FormEditorItem *>(item);
        if (formEditorItem)
            formEditorItem->updateGeometry();
    }
*/
}

void FormEditorFlowItem::setDataModelPositionInBaseState(const QPointF &position)
{
    setDataModelPosition(position);
}

void FormEditorFlowItem::updateGeometry()
{
    FormEditorItem::updateGeometry();
    const QPointF pos = qmlItemNode().flowPosition();
    setTransform(QTransform::fromTranslate(pos.x(), pos.y()));

    // Call updateGeometry() on all related transitions
    QmlFlowTargetNode flowItem(qmlItemNode());
    if (flowItem.isValid() && flowItem.flowView().isValid()) {
        const auto nodes = flowItem.flowView().transitions();
        for (const ModelNode &node : nodes) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(node))
                item->updateGeometry();
        }
    }
}

QPointF FormEditorFlowItem::instancePosition() const
{
    return qmlItemNode().flowPosition();
}


void FormEditorFlowActionItem::setDataModelPosition(const QPointF &position)
{
    qmlItemNode().setPosition(position);
    updateGeometry();

/* TODO
    for (QGraphicsItem *item : scene()->items()) {
        if (item == this)
            continue;

        auto formEditorItem = qgraphicsitem_cast<FormEditorItem *>(item);
        if (formEditorItem)
            formEditorItem->updateGeometry();
    }
*/
}

void FormEditorFlowActionItem::setDataModelPositionInBaseState(const QPointF &position)
{
    qmlItemNode().setPostionInBaseState(position);
    updateGeometry();
}

void FormEditorFlowActionItem::updateGeometry()
{
    FormEditorItem::updateGeometry();
    //const QPointF pos = qmlItemNode().flowPosition();
    //setTransform(QTransform::fromTranslate(pos.x(), pos.y()));

    // Call updateGeometry() on all related transitions
    QmlFlowItemNode flowItem = QmlFlowActionAreaNode(qmlItemNode()).flowItemParent();
    if (flowItem.isValid() && flowItem.flowView().isValid()) {
        const auto nodes = flowItem.flowView().transitions();
        for (const ModelNode &node : nodes) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(node))
                item->updateGeometry();
        }
    }
}

void FormEditorFlowActionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!painter->isActive())
        return;

    if (!qmlItemNode().isValid())
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen;
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCosmetic(true);

    QColor flowColor = "#e71919";

    if (qmlItemNode().rootModelNode().hasAuxiliaryData("areaColor"))
        flowColor = qmlItemNode().rootModelNode().auxiliaryData("areaColor").value<QColor>();

    if (qmlItemNode().modelNode().hasAuxiliaryData("color"))
        flowColor = qmlItemNode().modelNode().auxiliaryData("color").value<QColor>();

    qreal width = 2;

    if (qmlItemNode().modelNode().hasAuxiliaryData("width"))
        width = qmlItemNode().modelNode().auxiliaryData("width").toInt();

    bool dash = false;


    if (qmlItemNode().modelNode().hasAuxiliaryData("dash"))
        dash = qmlItemNode().modelNode().auxiliaryData("dash").toBool();

    pen.setColor(flowColor);
    if (dash)
        pen.setStyle(Qt::DashLine);
    else
        pen.setStyle(Qt::SolidLine);

    pen.setWidthF(width);
    pen.setCosmetic(true);
    painter->setPen(pen);

    QColor fillColor = QColor(Qt::transparent);

    if (qmlItemNode().rootModelNode().hasAuxiliaryData("areaFillColor"))
        fillColor = qmlItemNode().rootModelNode().auxiliaryData("areaFillColor").value<QColor>();

    if (qmlItemNode().modelNode().hasAuxiliaryData("fillColor"))
        fillColor = qmlItemNode().modelNode().auxiliaryData("fillColor").value<QColor>();

    if (fillColor.alpha() > 0)
        painter->setBrush(fillColor);

    painter->drawRoundedRect(boundingRect(), blockRadius, blockRadius);

    painter->restore();
}

QTransform FormEditorFlowActionItem::instanceSceneTransform() const
{
    return sceneTransform();
}

QTransform FormEditorFlowActionItem::instanceSceneContentItemTransform() const
{
    return sceneTransform();
}

void FormEditorTransitionItem::synchronizeOtherProperty(const QByteArray &)
{
    setContentVisible(true);
}

void FormEditorTransitionItem::setDataModelPosition(const QPointF &)
{

}

void FormEditorTransitionItem::setDataModelPositionInBaseState(const QPointF &)
{

}

static bool isValid(const QList<QmlItemNode> &list)
{
    for (const auto &item : list)
        if (!item.isValid())
            return false;

    return !list.isEmpty();
}

static bool isModelNodeValid(const QList<QmlItemNode> &list)
{
    for (const auto &item : list)
        if (!item.modelNode().isValid())
            return false;

    return !list.isEmpty();
}

class ResolveConnection
{
public:
    ResolveConnection(const QmlItemNode &node)
        : from()
        , to()
        , areaNode(ModelNode())
    {
        if (node.modelNode().hasBindingProperty("from")) {
            if (node.modelNode().bindingProperty("from").isList())
                from = Utils::transform<QList>(node.modelNode().bindingProperty("from").resolveToModelNodeList(),
                                               [](const ModelNode &node) {
                    return QmlItemNode(node);
                });
            else
                from = QList<QmlItemNode>({node.modelNode().bindingProperty("from").resolveToModelNode()});
        }

        if (node.modelNode().hasBindingProperty("to")) {
            if (node.modelNode().bindingProperty("to").isList())
                to = Utils::transform<QList>(node.modelNode().bindingProperty("to").resolveToModelNodeList(),
                                               [](const ModelNode &node) {
                    return QmlItemNode(node);
                });
            else
                to = QList<QmlItemNode>({node.modelNode().bindingProperty("to").resolveToModelNode()});
        }

        if (from.empty()) {
            for (const ModelNode &wildcard : QmlFlowViewNode(node.rootModelNode()).wildcards()) {
                if (wildcard.bindingProperty("target").resolveToModelNode() == node.modelNode()) {
                    from.clear();
                    from.append(wildcard);
                    isWildcardLine = true;
                }
            }
        }

        // Only assign area node if there is exactly one from (QmlFlowItemNode)
        if (from.size() == 1) {
            const QmlItemNode tmp = from.back();
            const QmlFlowItemNode f(tmp.modelNode());

            if (f.isValid()) {
                for (const QmlFlowActionAreaNode &area : f.flowActionAreas()) {
                    ModelNode target = area.targetTransition();
                    if (target == node.modelNode())
                        areaNode = area;
                }

                const ModelNode decisionNode = QmlFlowItemNode::decisionNodeForTransition(node.modelNode());
                if (decisionNode.isValid()) {
                    from.clear();
                    from.append(decisionNode);
                    areaNode = ModelNode();
                }

                if (f.modelNode().hasAuxiliaryData("joinConnection"))
                    joinConnection = f.modelNode().auxiliaryData("joinConnection").toBool();
            } else {
                if (f == node.rootModelNode())
                    isStartLine = true;
            }
        }
    }

    bool joinConnection = false;
    bool isStartLine = false;
    bool isWildcardLine = false;

    QList<QmlItemNode> from;
    QList<QmlItemNode> to;
    QmlFlowActionAreaNode areaNode;
};

enum ConnectionType
{
    Default = 0,
    Bezier
};

class ConnectionConfiguration
{
public:
    ConnectionConfiguration(const QmlItemNode &node,
                            const ResolveConnection &resolveConnection,
                            const qreal scaleFactor,
                            bool hitTest = false)
        : width(2)
        , adjustedWidth(width / scaleFactor)
        , color(QColor("#e71919"))
        , lineBrush(QBrush(color))
        , penStyle(Qt::SolidLine)
        , dashPattern()
        , drawStart(true)
        , drawEnd(true)
        , joinEnd(false)
        , outOffset(0)
        , inOffset(0)
        , breakOffset(50)
        , radius(8)
        , bezier(50)
        , type(ConnectionType::Default)
        , label()
        , fontSize(labelFontSize / scaleFactor)
        , labelOffset(14 / scaleFactor)
        , labelPosition(50.0)
        , labelFlags(Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextDontClip)
        , labelFlipSide(false)
        , hitTesting(hitTest)
    {
        // width
        if (node.modelNode().hasAuxiliaryData("width"))
            width = node.modelNode().auxiliaryData("width").toInt();
        // adjusted width
        if (node.modelNode().isSelected())
            width += 2;
        if (hitTest)
            width = width * 8  / scaleFactor;
        // color
        if (resolveConnection.isStartLine)
            color = QColor("blue");
        if (resolveConnection.isWildcardLine)
            color = QColor("green");
        if (node.rootModelNode().hasAuxiliaryData("transitionColor"))
            color = node.rootModelNode().auxiliaryData("transitionColor").value<QColor>();
        if (node.modelNode().hasAuxiliaryData("color"))
            color = node.modelNode().auxiliaryData("color").value<QColor>();
        // linbe brush
        lineBrush = QBrush(color);

        // pen style

        // dash
        if (node.modelNode().hasAuxiliaryData("dash") && node.modelNode().auxiliaryData("dash").toBool())
            penStyle = Qt::DashLine;
        // in/out offset
        if (node.modelNode().hasAuxiliaryData("outOffset"))
            outOffset = node.modelNode().auxiliaryData("outOffset").toInt();
        if (node.modelNode().hasAuxiliaryData("inOffset"))
            inOffset = node.modelNode().auxiliaryData("inOffset").toInt();
        // break offset
        if (node.modelNode().hasAuxiliaryData("breakPoint"))
            breakOffset = node.modelNode().auxiliaryData("breakPoint").toInt();
        // radius
        if (node.rootModelNode().hasAuxiliaryData("transitionRadius"))
            radius = node.rootModelNode().auxiliaryData("transitionRadius").toInt();
        if (node.modelNode().hasAuxiliaryData("radius"))
            radius = node.modelNode().auxiliaryData("radius").toInt();
        // bezier
        if (node.rootModelNode().hasAuxiliaryData("transitionBezier"))
            bezier = node.rootModelNode().auxiliaryData("transitionBezier").toInt();
        if (node.modelNode().hasAuxiliaryData("bezier"))
            bezier = node.modelNode().auxiliaryData("bezier").toInt();
        // type
        if (node.rootModelNode().hasAuxiliaryData("transitionType"))
            type = static_cast<ConnectionType>(node.rootModelNode().auxiliaryData("transitionType").toInt());
        if (node.modelNode().hasAuxiliaryData("type"))
            type = static_cast<ConnectionType>(node.modelNode().auxiliaryData("type").toInt());
        // label
        if (node.modelNode().hasBindingProperty("condition"))
            label = node.modelNode().bindingProperty("condition").expression();
        if (node.modelNode().hasVariantProperty("question"))
            label = node.modelNode().variantProperty("question").value().toString();
        // font size

        // label offset

        // label position
        if (node.modelNode().hasAuxiliaryData("labelPosition"))
            labelPosition = node.modelNode().auxiliaryData("labelPosition").toReal();
        // label flip side
        if (node.modelNode().hasAuxiliaryData("labelFlipSide"))
            labelFlipSide = node.modelNode().auxiliaryData("labelFlipSide").toBool();
    }

    qreal width;
    qreal adjustedWidth;
    QColor color; // TODO private/setter
    QBrush lineBrush;
    Qt::PenStyle penStyle;
    QVector<qreal> dashPattern;
    bool drawStart;
    bool drawEnd;
    // Dirty hack for joining/merging arrow heads on many-to-one transitions
    bool joinEnd;
    int outOffset;
    int inOffset;
    int breakOffset;
    int radius;
    int bezier;
    ConnectionType type;
    QString label;
    int fontSize;
    qreal labelOffset;
    qreal labelPosition;
    int labelFlags;
    bool labelFlipSide;
    bool hitTesting;
};

static bool verticalOverlap(const QRectF &from, const QRectF &to)
{
    if (from.top() < to.bottom() && from.bottom() > to.top())
        return true;

    if (to.top() < from.bottom() && to.bottom() > from.top())
        return true;

    return false;
}

static bool horizontalOverlap(const QRectF &from, const QRectF &to)
{
    if (from.left() < to.right() && from.right() > to.left())
        return true;

    if (to.left() < from.right() && to.right() > from.left())
        return true;

    return false;
}

static QPainterPath roundedCorner(const QPointF &s,
                                  const QPointF &m,
                                  const QPointF &e,
                                  int radius)
{
    const QVector2D sm(m - s);
    const QVector2D me(e - m);
    const float smLength = sm.length();
    const float meLength = me.length();
    const int actualRadius = qMin(radius, static_cast<int>(qMin(smLength, meLength)));
    const QVector2D smNorm = sm.normalized();
    const QVector2D meNorm = me.normalized();
    QRectF rect(m, QSizeF(actualRadius * 2, actualRadius * 2));

    QPainterPath path(s);

    if (smNorm.y() < 0 && meNorm.x() > 0) {
        rect.moveTopLeft(m);
        path.arcTo(rect, 180, -90);
    } else if (smNorm.x() < 0 && meNorm.y() > 0) {
        rect.moveTopLeft(m);
        path.arcTo(rect, 90, 90);
    } else if (smNorm.y() > 0 && meNorm.x() > 0) {
        rect.moveBottomLeft(m);
        path.arcTo(rect, 180, 90);
    } else if (smNorm.x() < 0 && meNorm.y() < 0) {
        rect.moveBottomLeft(m);
        path.arcTo(rect, 270, -90);
    } else if (smNorm.x() > 0 && meNorm.y() > 0) {
        rect.moveTopRight(m);
        path.arcTo(rect, 90, -90);
    } else if (smNorm.y() < 0 && meNorm.x() < 0) {
        rect.moveTopRight(m);
        path.arcTo(rect, 0, 90);
    } else if (smNorm.y() > 0 && meNorm.x() < 0) {
        rect.moveBottomRight(m);
        path.arcTo(rect, 0, -90);
    } else if (smNorm.x() > 0 && meNorm.y() < 0) {
        rect.moveBottomRight(m);
        path.arcTo(rect, 270, 90);
    }

    path.lineTo(e);
    return path;
}

// This function determines whether the vertices are in cw or ccw order.
// It finds the lowest and rightmost vertex, and computes the cross-product
// of the vectors along its incident edges.
// Written by Joseph O'Rourke, 25 August 1995. orourke@cs.smith.edu
//  1: ccw
//  0: default
// -1: cw

static int counterClockWise(const std::vector<QPointF> &points)
{
    if (points.empty())
        return 0;

    // FindLR finds the lowest, rightmost point.
    auto findLR = [](const std::vector<QPointF> &points) {
        int i = 0;
        int m = 0;
        QPointF min = points.front();

        for (const auto p : points) {
            if ((p.y() < min.y()) || ((p.y() == min.y()) && (p.x() > min.x()))) {
                m = i;
                min = p;
            }
            ++i;
        }
        return m;
    };

    const int m = findLR(points);
    const int n = points.size();

    // Determine previous and next point to m (the lowest, rightmost point).
    const QPointF a = points[(m + (n - 1)) % n];
    const QPointF b = points[m];
    const QPointF c = points[(m + 1) % n];

    const int area = a.x() * b.y() - a.y() * b.x() +
                     a.y() * c.x() - a.x() * c.y() +
                     b.x() * c.y() - c.x() * b.y();

    if (area > 0)
        return 1;
    else if (area < 0)
        return -1;
    else
        return 0;
}

static QPainterPath quadBezier(const QPointF &s,
                               const QPointF &c,
                               const QPointF &e,
                               int bezier,
                               int breakOffset)
{
    const QLineF se(s, e);
    const QPointF breakPoint = se.pointAt(breakOffset / 100.0);
    QLineF breakLine;

    if (counterClockWise({s, c, e}) == 1)
        breakLine = QLineF(breakPoint, breakPoint + QPointF(se.dy(), -se.dx()));
    else
        breakLine = QLineF(breakPoint, breakPoint + QPointF(-se.dy(), se.dx()));

    breakLine.setLength(se.length());

    const QPointF controlPoint = breakLine.pointAt(bezier / 100.0);

    QPainterPath path(s);
    path.quadTo(controlPoint, e);

    return path;
}

static QPainterPath cubicBezier(const QPointF &s,
                                const QPointF &c1,
                                const QPointF &c2,
                                const QPointF &e,
                                int bezier)
{
    QPainterPath path(s);
    const QPointF adjustedC1 = QLineF(s, c1).pointAt(bezier / 100.0);
    const QPointF adjustedC2 = QLineF(e, c2).pointAt(bezier / 100.0);

    path.cubicTo(adjustedC1, adjustedC2, e);

    return path;
}

static QPainterPath lShapedConnection(const QPointF &start,
                                      const QPointF &mid,
                                      const QPointF &end,
                                      const ConnectionConfiguration &config)
{
    if (config.type == ConnectionType::Default) {
        if (config.radius == 0) {
            QPainterPath path(start);
            path.lineTo(mid);
            path.lineTo(end);
            return path;
        } else {
            return roundedCorner(start, mid, end, config.radius);
        }
    } else {
        return quadBezier(start, mid, end, config.bezier, config.breakOffset);
    }
}

static QPainterPath sShapedConnection(const QPointF &start,
                                      const QPointF &mid1,
                                      const QPointF &mid2,
                                      const QPointF &end,
                                      const ConnectionConfiguration &config)
{
    if (config.type == ConnectionType::Default) {
        if (config.radius == 0) {
            QPainterPath path(start);
            path.lineTo(mid1);
            path.lineTo(mid2);
            path.lineTo(end);
            return path;
        } else {
            const QLineF breakLine(mid1, mid2);
            QPainterPath path1 = roundedCorner(start, mid1, breakLine.center(), config.radius);
            QPainterPath path2 = roundedCorner(breakLine.center(), mid2, end, config.radius);
            return path1 + path2;
        }
    } else {
        return cubicBezier(start, mid1, mid2, end, config.bezier);
    }
}

class Connection
{
public:
    Connection(const ResolveConnection &resolveConnection,
               const QPointF &position,
               const QmlItemNode &from,
               const QmlItemNode &to,
               const ConnectionConfiguration &connectionConfig)
        : config(connectionConfig)
    {
        if (from.isFlowDecision()) {
            int size = flowBlockSize;
            if (from.modelNode().hasAuxiliaryData("blockSize"))
                size = from.modelNode().auxiliaryData("blockSize").toInt();

            fromRect = QRectF(0, 0, size, size);

            QTransform transform;
            transform.translate(fromRect.center().x(), fromRect.center().y());
            transform.rotate(45);
            transform.translate(-fromRect.center().x(), -fromRect.center().y());

            fromRect = transform.mapRect(fromRect);
        } else if (from.isFlowWildcard()) {
            int size = flowBlockSize;
            if (from.modelNode().hasAuxiliaryData("blockSize"))
                size = from.modelNode().auxiliaryData("blockSize").toInt();
            fromRect = QRectF(0, 0, size, size);
        } else if (from.isFlowView()) {
            fromRect = QRectF(0, 0, flowBlockSize, flowBlockSize);
        } else {
            fromRect = from.instanceBoundingRect();
        }

        fromRect.translate(from.flowPosition());

        if (!resolveConnection.joinConnection && resolveConnection.areaNode.isValid()) {
            fromRect = QmlItemNode(resolveConnection.areaNode).instanceBoundingRect();
            fromRect.translate(from.flowPosition());
            fromRect.translate(resolveConnection.areaNode.instancePosition());
        }

        if (to.isFlowDecision()) {
            int size = flowBlockSize;
            if (to.modelNode().hasAuxiliaryData("blockSize"))
                size = to.modelNode().auxiliaryData("blockSize").toInt();

            toRect = QRectF(0, 0, size, size);

            QTransform transform;
            transform.translate(toRect.center().x(), toRect.center().y());
            transform.rotate(45);
            transform.translate(-toRect.center().x(), -toRect.center().y());

            toRect = transform.mapRect(toRect);
        } else {
            toRect = to.instanceBoundingRect();
        }

        toRect.translate(to.flowPosition());

        if (resolveConnection.isStartLine) {
            fromRect = QRectF(0, 0, 96, 96); // TODO Use const startItemOffset
            fromRect.translate(to.flowPosition() + QPoint(-180, toRect.height() / 2 - 96 / 2));
            fromRect.translate(0, config.outOffset);
        }

        fromRect.translate(-position);
        toRect.translate(-position);

        bool horizontalFirst = true;
        extraLine = false;

        if (horizontalFirst) {
            if (toRect.center().x() > fromRect.left() && toRect.center().x() < fromRect.right()) {
                horizontalFirst = false;
                extraLine = true;
            } else if (verticalOverlap(fromRect, toRect)) {
                horizontalFirst = true;
                extraLine = true;
            }
        } else {
            if (toRect.center().y() > fromRect.top() && toRect.center().y() < fromRect.bottom()) {
                horizontalFirst = true;
                extraLine = true;
            } else if (horizontalOverlap(fromRect, toRect)) {
                horizontalFirst = false;
                extraLine = true;
            }
        }

        const bool boolExitRight = fromRect.right() < toRect.center().x();
        const bool boolExitBottom = fromRect.bottom() < toRect.center().y();

        const int padding = 4 * config.adjustedWidth;

        if (horizontalFirst) {
            const qreal startX = boolExitRight ? fromRect.right() + padding : fromRect.x() - padding;
            const qreal startY = fromRect.center().y() + config.outOffset;
            start = QPointF(startX, startY);

            if (!extraLine) {
                const qreal endY = (fromRect.bottom() > toRect.y()) ? toRect.bottom() + padding : toRect.top() - padding;
                end = QPointF(toRect.center().x() + config.inOffset, endY);
                mid1 = mid2 = QPointF(end.x(), start.y());
                path = lShapedConnection(start, mid1, end, config);
            } else {
                const qreal endX = (fromRect.right() > toRect.x()) ? toRect.right() + padding : toRect.left() - padding;
                end = QPointF(endX, toRect.center().y() + config.inOffset);
                const qreal middleFactor = config.breakOffset / 100.0;
                mid1 = QPointF(start.x() * middleFactor + end.x() * (1 - middleFactor), start.y());
                mid2 = QPointF(mid1.x(), end.y());
                path = sShapedConnection(start, mid1, mid2, end, config);
            }
        } else {
            const qreal startX = fromRect.center().x() + config.outOffset;
            const qreal startY = boolExitBottom ? fromRect.bottom() + padding : fromRect.top() - padding;
            start = QPointF(startX, startY);

            if (!extraLine) {
                const qreal endX = (fromRect.right() > toRect.x()) ? toRect.right() + padding : toRect.left() - padding;
                end = QPointF(endX, toRect.center().y() + config.inOffset);
                mid1 = mid2 = QPointF(start.x(), end.y());
                path = lShapedConnection(start, mid1, end, config);
            } else {
                const qreal endY = (fromRect.bottom() > toRect.y()) ? toRect.bottom() + padding : toRect.top() - padding;
                end = QPointF(toRect.center().x() + config.inOffset, endY);
                const qreal middleFactor = config.breakOffset / 100.0;
                mid1 = QPointF(start.x(), start.y() * middleFactor + end.y() * (1 - middleFactor));
                mid2 = QPointF(end.x(), mid1.y());
                path = sShapedConnection(start, mid1, mid2, end, config);
            }
        }
    }

    QRectF fromRect;
    QRectF toRect;

    QPointF start;
    QPointF end;

    QPointF mid1;
    QPointF mid2;

    bool extraLine;

    ConnectionConfiguration config;
    QPainterPath path;
};

static int normalizeAngle(int angle)
{
    int newAngle = angle;
    while (newAngle <= -90) newAngle += 180;
    while (newAngle > 90) newAngle -= 180;
    return newAngle;
}

void FormEditorTransitionItem::updateGeometry()
{
    FormEditorItem::updateGeometry();

    ResolveConnection resolved(qmlItemNode());

    if (!isValid(resolved.from) || !isValid(resolved.to))
        return;

    QPointF min(std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::max());
    QPointF max(std::numeric_limits<qreal>::min(), std::numeric_limits<qreal>::min());

    auto minMaxHelper = [&](const QList<QmlItemNode> &items) {
        QRectF boundingRect;
        for (const auto &i : items) {
            const QPointF p = QmlItemNode(i).flowPosition();

            if (p.x() < min.x())
                min.setX(p.x());

            if (p.y() < min.y())
                min.setY(p.y());

            if (p.x() > max.x())
                max.setX(p.x());

            if (p.y() > max.y())
                max.setY(p.y());

            const QRectF tmp(p, i.instanceSize());
            boundingRect = boundingRect.united(tmp);
        }
        return boundingRect;
    };

    const QRectF toBoundingRect = minMaxHelper(resolved.to);
    // Special treatment for start line bounding rect calculation
    const QRectF fromBoundingRect = !resolved.isStartLine ? minMaxHelper(resolved.from)
                                                          : toBoundingRect + QMarginsF(startItemOffset, 0, 0, 0);

    QRectF overallBoundingRect(min, max);
    overallBoundingRect = overallBoundingRect.united(fromBoundingRect);
    overallBoundingRect = overallBoundingRect.united(toBoundingRect);

    setPos(overallBoundingRect.topLeft());

    // Needed due to the upcoming rects are relative to the set position. If this one is not
    // translate to the newly set position, then th resulting bounding box would be to big.
    overallBoundingRect.translate(-pos());

    ConnectionConfiguration config(qmlItemNode(), resolved, viewportTransform().m11());

    // Local painter is used to get the labels bounding rect by using drawText()
    QPixmap pixmap(640, 480);
    QPainter localPainter(&pixmap);
    QFont font = localPainter.font();
    font.setPixelSize(config.fontSize);
    localPainter.setFont(font);

    for (const auto &from : resolved.from) {
        for (const auto &to : resolved.to) {
            Connection connection(resolved, pos(), from, to, config);

            // Just add the configured transition width to the bounding box to make sure the BB is not cutting
            // off half of the transition resulting in a bad selection experience.
            QRectF pathBoundingRect = connection.path.boundingRect() + QMarginsF(config.width, config.width, config.width, config.width);
            overallBoundingRect = overallBoundingRect.united(connection.fromRect);
            overallBoundingRect = overallBoundingRect.united(connection.toRect);
            overallBoundingRect = overallBoundingRect.united(pathBoundingRect);

            // Calculate bounding rect for label
            // TODO The calculation should be put into a separate function to avoid code duplication as this
            // can also be found in drawLabel()
            if (!connection.config.label.isEmpty()) {
                const qreal percent = connection.config.labelPosition / 100.0;
                const QPointF pos = connection.path.pointAtPercent(percent);
                const qreal angle = connection.path.angleAtPercent(percent);

                QLineF tmp(pos, QPointF(10, 10));
                tmp.setLength(connection.config.labelOffset);
                tmp.setAngle(angle + (connection.config.labelFlipSide ? 270 : 90));

                QRectF textRect(0, 0, 100, 50);
                textRect.moveCenter(tmp.p2());

                QRectF labelRect;

                QTransform transform;
                transform.translate(textRect.center().x(), textRect.center().y());
                transform.rotate(-normalizeAngle(angle));
                transform.translate(-textRect.center().x(), -textRect.center().y());

                localPainter.setTransform(transform);
                localPainter.drawText(textRect,
                                      connection.config.labelFlags,
                                      connection.config.label,
                                      &labelRect);
                QRectF labelBoundingBox = transform.mapRect(labelRect);
                overallBoundingRect = overallBoundingRect.united(labelBoundingBox);
            }
        }
    }

    m_selectionBoundingRect = overallBoundingRect;
    m_paintedBoundingRect = m_selectionBoundingRect;
    m_boundingRect = m_selectionBoundingRect;
    setZValue(10);
}

QPointF FormEditorTransitionItem::instancePosition() const
{
    return qmlItemNode().flowPosition();
}

static void drawLabel(QPainter *painter, const Connection &connection)
{
    if (connection.config.label.isEmpty())
        return;

    const qreal percent = connection.config.labelPosition / 100.0;
    const QPointF pos = connection.path.pointAtPercent(percent);
    const qreal angle = connection.path.angleAtPercent(percent);

    QLineF tmp(pos, QPointF(10, 10));
    tmp.setLength(connection.config.labelOffset);
    tmp.setAngle(angle + (connection.config.labelFlipSide ? 270 : 90));

    QRectF textRect(0, 0, 100, 50);
    textRect.moveCenter(tmp.p2());

    painter->save();
    painter->translate(textRect.center());
    painter->rotate(-normalizeAngle(angle));
    painter->translate(-textRect.center());
    painter->drawText(textRect, connection.config.labelFlags, connection.config.label);
    painter->restore();
}

static void drawArrow(QPainter *painter,
                      const QPointF &point,
                      const qreal &angle,
                      int arrowLength,
                      int arrowWidth)
{
    const QPointF peakP(0, 0);
    const QPointF leftP(-arrowLength, -arrowWidth * 0.5);
    const QPointF rightP(-arrowLength, arrowWidth * 0.5);

    painter->save();

    painter->translate(point);
    painter->rotate(-angle);
    painter->drawLine(leftP, peakP);
    painter->drawLine(rightP, peakP);

    painter->restore();
}

static void paintConnection(QPainter *painter, const Connection &connection)
{
    const int arrowLength = 4 * connection.config.adjustedWidth;
    const int arrowWidth = 8 * connection.config.adjustedWidth;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Draw path/connection line
    QPen pen;
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCapStyle(Qt::RoundCap);
    pen.setBrush(connection.config.lineBrush);

    if (connection.config.dashPattern.size()) {
        pen.setStyle(Qt::CustomDashLine);
        pen.setDashPattern(connection.config.dashPattern);
    } else {
        pen.setStyle(connection.config.penStyle);
    }

    pen.setWidthF(connection.config.width);
    painter->setPen(pen);

    painter->drawPath(connection.path);

    pen.setWidthF(connection.config.width);
    pen.setStyle(Qt::SolidLine);
    pen.setColor(connection.config.color);
    painter->setPen(pen);

    // Draw arrow
    qreal angle = QLineF(connection.mid2, connection.end).angle();

    if (!connection.config.joinEnd) {
        qreal anglePercent = 1.0;
        if (connection.extraLine && connection.config.bezier < 80) {
            anglePercent = 1.0 - qMin(1.0, (80 - connection.config.bezier) / 10.0) * 0.05;
            angle = connection.path.angleAtPercent(anglePercent);
        }
    }

    if (connection.config.drawEnd)
        drawArrow(painter, connection.end, angle, arrowLength, arrowWidth);

    // Draw start ellipse
    if (connection.config.drawStart) {
        painter->setBrush(Qt::white);
        painter->drawEllipse(connection.start, arrowLength / 3, arrowLength / 3);
    }

    // Draw label
    drawLabel(painter, connection);

    painter->restore();
}

void FormEditorTransitionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!painter->isActive())
        return;

    if (!qmlItemNode().modelNode().isValid())
        return;

    if (!qmlItemNode().modelNode().hasBindingProperty("to"))
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    ResolveConnection resolved(qmlItemNode());

    if (!isModelNodeValid(resolved.from))
        return;

    ConnectionConfiguration config(qmlItemNode(), resolved, viewportTransform().m11(), m_hitTest);

    QFont font = painter->font();
    font.setPixelSize(config.fontSize);
    painter->setFont(font);

    for (const auto &f : resolved.from) {
        for (const auto &t : resolved.to) {
            Connection connection(resolved, pos(), f, t, config);
            if (!config.hitTesting) {
                // The following if statement is a special treatment for n-to-n, 1-to-n and n-to-1
                // transitions. This block is setting up the connection configuration for drawing.
                QPointF start = connection.path.pointAtPercent(0.0);
                QPointF end = connection.path.pointAtPercent(1.0);

                // many-to-many
                if ((resolved.from.size() > 1 && resolved.to.size() > 1)) {
                    // TODO
                }
                // many-to-one
                else if (resolved.from.size() > 1 && resolved.to.size() == 1) {
                    connection.config.joinEnd = true;

                    if (qmlItemNode().modelNode().isSelected()) {
                        connection.config.dashPattern << 2 << 3;
                    } else {
                        QLinearGradient gradient(start, end);
                        QColor color = config.color;
                        color.setAlphaF(0);
                        gradient.setColorAt(0.25, color);
                        color.setAlphaF(1);
                        gradient.setColorAt(1, color);

                        connection.config.lineBrush = QBrush(gradient);
                        connection.config.drawStart = false;
                        connection.config.drawEnd = true;
                        connection.config.dashPattern << 1 << 6;
                    }
                }
                // one-to-many
                else if (resolved.from.size() == 1 && resolved.to.size() > 1) {

                    if (qmlItemNode().modelNode().isSelected()) {
                        connection.config.dashPattern << 2 << 3;
                    } else {
                        QLinearGradient gradient(start, end);
                        QColor color = config.color;
                        color.setAlphaF(1);
                        gradient.setColorAt(0, color);
                        color.setAlphaF(0);
                        gradient.setColorAt(0.75, color);

                        connection.config.lineBrush = QBrush(gradient);
                        connection.config.drawStart = true;
                        connection.config.drawEnd = false;
                        connection.config.dashPattern << 1 << 6;
                    }
                }
            } else {
                connection.config.penStyle = Qt::SolidLine;
            }

            paintConnection(painter, connection);

            if (resolved.isStartLine) {
                const QString icon = Theme::getIconUnicode(Theme::startNode);

                QPen pen;
                pen.setCosmetic(true);
                pen.setColor(config.color);
                painter->setPen(pen);

                const int iconAdjust = 48;
                const int size = connection.fromRect.width();
                const int iconSize = size - iconAdjust;
                const int x = connection.fromRect.topRight().x() - startItemOffset;
                const int y = connection.fromRect.topRight().y();
                painter->drawRoundedRect(x, y , size - 10, size, size / 2, iconSize / 2);
                drawIcon(painter, x + iconAdjust / 2, y + iconAdjust / 2, icon, iconSize, iconSize, config.color);
            }
        }
    }

    painter->restore();
}

bool FormEditorTransitionItem::flowHitTest(const QPointF &point) const
{
    QImage image(boundingRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(QColor("black"));

    QPainter p(&image);

    m_hitTest = true;
    const_cast<FormEditorTransitionItem *>(this)->paint(&p, nullptr, nullptr);
    m_hitTest = false;

    QPoint pos = mapFromScene(point).toPoint();
    return image.pixelColor(pos).value() > 0;
}

QTransform FormEditorItem::viewportTransform() const
{
    QTC_ASSERT(scene(), return {});
    QTC_ASSERT(!scene()->views().isEmpty(), return {});

    return scene()->views().first()->viewportTransform();
}

void FormEditorFlowDecisionItem::updateGeometry()
{
    prepareGeometryChange();

    int size = flowBlockSize;
    if (qmlItemNode().modelNode().hasAuxiliaryData("blockSize"))
        size = qmlItemNode().modelNode().auxiliaryData("blockSize").toInt();

    QRectF boundingRect(0, 0, size, size);
    QTransform transform;
    if (qmlItemNode().isFlowDecision()) {
        transform.translate(boundingRect.center().x(), boundingRect.center().y());
        transform.rotate(45);
        transform.translate(-boundingRect.center().x(), -boundingRect.center().y());

        // If drawing the dialog title is requested we need to add it to the bounding rect.
        QRectF labelBoundingRect;
        int showDialogLabel = false;
        if (qmlItemNode().modelNode().hasAuxiliaryData("showDialogLabel"))
            showDialogLabel = qmlItemNode().modelNode().auxiliaryData("showDialogLabel").toBool();

        if (showDialogLabel) {
            QString dialogTitle;
            if (qmlItemNode().modelNode().hasVariantProperty("dialogTitle"))
                dialogTitle = qmlItemNode().modelNode().variantProperty("dialogTitle").value().toString();

            if (!dialogTitle.isEmpty()) {
                // Local painter is used to get the labels bounding rect by using drawText()
                QPixmap pixmap(640, 480);
                QPainter localPainter(&pixmap);
                QFont font = localPainter.font();
                font.setPixelSize(labelFontSize / viewportTransform().m11());
                localPainter.setFont(font);

                int margin = blockAdjust * 0.5;
                const QRectF adjustedRect = boundingRect.adjusted(margin, margin, -margin, -margin);

                QRectF textRect(0, 0, 100, 20);

                Qt::Corner corner = Qt::TopRightCorner;
                if (qmlItemNode().modelNode().hasAuxiliaryData("dialogLabelPosition"))
                   corner = qmlItemNode().modelNode().auxiliaryData("dialogLabelPosition").value<Qt::Corner>();

                int flag = 0;
                switch (corner) {
                    case Qt::TopLeftCorner:
                        flag = Qt::AlignRight;
                        textRect.moveBottomRight(adjustedRect.topLeft());
                        break;
                    case Qt::TopRightCorner:
                        flag = Qt::AlignLeft;
                        textRect.moveBottomLeft(adjustedRect.topRight());
                        break;
                    case Qt::BottomLeftCorner:
                        flag = Qt::AlignRight;
                        textRect.moveTopRight(adjustedRect.bottomLeft());
                        break;
                    case Qt::BottomRightCorner:
                        flag = Qt::AlignLeft;
                        textRect.moveTopLeft(adjustedRect.bottomRight());
                        break;
                }

                localPainter.drawText(textRect, flag | Qt::TextDontClip, dialogTitle, &labelBoundingRect);
            }
        }

        // Unite the rotate item bounding rect with the label bounding rect.
        boundingRect = transform.mapRect(boundingRect);
        boundingRect = boundingRect.united(labelBoundingRect);
    }

    m_selectionBoundingRect = boundingRect;
    m_paintedBoundingRect = m_selectionBoundingRect;
    m_boundingRect = m_paintedBoundingRect;
    setTransform(qmlItemNode().instanceTransformWithContentTransform());
    const QPointF pos = qmlItemNode().flowPosition();
    setTransform(QTransform::fromTranslate(pos.x(), pos.y()));

    // Call updateGeometry() on all related transitions
    QmlFlowTargetNode flowItem(qmlItemNode());
    if (flowItem.isValid() && flowItem.flowView().isValid()) {
        const auto nodes = flowItem.flowView().transitions();
        for (const ModelNode &node : nodes) {
            if (FormEditorItem *item = scene()->itemForQmlItemNode(node))
                item->updateGeometry();
        }
    }
}

void FormEditorFlowDecisionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    if (!painter->isActive())
        return;

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    QPen pen;
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCosmetic(true);

    QColor flowColor = "#e71919";

    if (qmlItemNode().rootModelNode().hasAuxiliaryData("blockColor"))
        flowColor = qmlItemNode().rootModelNode().auxiliaryData("blockColor").value<QColor>();

    if (qmlItemNode().modelNode().hasAuxiliaryData("color"))
        flowColor = qmlItemNode().modelNode().auxiliaryData("color").value<QColor>();

    const qreal scaleFactor = viewportTransform().m11();
    qreal width = 2;

    if (qmlItemNode().modelNode().hasAuxiliaryData("width"))
        width = qmlItemNode().modelNode().auxiliaryData("width").toInt();

    bool dash = false;

    if (qmlItemNode().modelNode().hasAuxiliaryData("dash"))
        dash = qmlItemNode().modelNode().auxiliaryData("dash").toBool();

    pen.setColor(flowColor);
    if (dash)
        pen.setStyle(Qt::DashLine);
    else
        pen.setStyle(Qt::SolidLine);

    pen.setWidthF(width);
    pen.setCosmetic(true);
    painter->setPen(pen);

    QColor fillColor = QColor(Qt::transparent);

    if (qmlItemNode().modelNode().hasAuxiliaryData("fillColor"))
       fillColor = qmlItemNode().modelNode().auxiliaryData("fillColor").value<QColor>();

    painter->save();

    if (fillColor.alpha() > 0)
        painter->setBrush(fillColor);

    int radius = blockRadius;
    if (qmlItemNode().modelNode().hasAuxiliaryData("blockRadius"))
        radius = qmlItemNode().modelNode().auxiliaryData("blockRadius").toInt();

    int size = flowBlockSize;
    if (qmlItemNode().modelNode().hasAuxiliaryData("blockSize"))
        size = qmlItemNode().modelNode().auxiliaryData("blockSize").toInt();

    QRectF boundingRect(0, 0, size, size);
    QTransform transform;
    int margin = blockAdjust;
    if (m_iconType == DecisionIcon) {
        transform.translate(boundingRect.center().x(), boundingRect.center().y());
        transform.rotate(45);
        transform.translate(-boundingRect.center().x(), -boundingRect.center().y());
        margin *= 0.5;
    }

    const QRectF adjustedRect = boundingRect.adjusted(margin, margin, -margin, -margin);

    painter->setTransform(transform, true);
    painter->drawRoundedRect(adjustedRect, radius, radius);

    const int iconDecrement = 32;
    const int iconSize = adjustedRect.width() - iconDecrement;
    const int offset = iconDecrement / 2 + margin;

    painter->restore();

    // Draw the dialog title inside the form view if requested. Decision item only.
    int showDialogLabel = false;
    if (qmlItemNode().modelNode().hasAuxiliaryData("showDialogLabel"))
        showDialogLabel = qmlItemNode().modelNode().auxiliaryData("showDialogLabel").toBool();

    if (showDialogLabel) {
        QString dialogTitle;
        if (qmlItemNode().modelNode().hasVariantProperty("dialogTitle"))
            dialogTitle = qmlItemNode().modelNode().variantProperty("dialogTitle").value().toString();

        if (!dialogTitle.isEmpty()) {

            QFont font = painter->font();
            font.setPixelSize(labelFontSize / scaleFactor);
            painter->setFont(font);

            QRectF textRect(0, 0, 100, 20);

            Qt::Corner corner = Qt::TopRightCorner;
            if (qmlItemNode().modelNode().hasAuxiliaryData("dialogLabelPosition"))
               corner = qmlItemNode().modelNode().auxiliaryData("dialogLabelPosition").value<Qt::Corner>();

            int flag = 0;
            switch (corner) {
                case Qt::TopLeftCorner:
                    flag = Qt::AlignRight;
                    textRect.moveBottomRight(adjustedRect.topLeft());
                    break;
                case Qt::TopRightCorner:
                    flag = Qt::AlignLeft;
                    textRect.moveBottomLeft(adjustedRect.topRight());
                    break;
                case Qt::BottomLeftCorner:
                    flag = Qt::AlignRight;
                    textRect.moveTopRight(adjustedRect.bottomLeft());
                    break;
                case Qt::BottomRightCorner:
                    flag = Qt::AlignLeft;
                    textRect.moveTopLeft(adjustedRect.bottomRight());
                    break;
            }

            painter->drawText(textRect, flag | Qt::TextDontClip, dialogTitle);
        }
    }

    const QString icon = (m_iconType ==
                          WildcardIcon) ? Theme::getIconUnicode(Theme::wildcard)
                                        : Theme::getIconUnicode(Theme::decisionNode);

    drawIcon(painter, offset, offset, icon, iconSize, iconSize, flowColor);

    painter->restore();
}

bool FormEditorFlowDecisionItem::flowHitTest(const QPointF &point) const
{
    Q_UNUSED(point)
    return true;
}

void FormEditorFlowWildcardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    FormEditorFlowDecisionItem::paint(painter, option, widget);
}

} //QmlDesigner
