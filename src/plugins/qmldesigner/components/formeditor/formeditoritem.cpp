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

#include <bindingproperty.h>

#include <modelnode.h>
#include <nodehints.h>
#include <nodemetainfo.h>

#include <theme.h>

#include <utils/theme/theme.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFontDatabase>
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

const char startNodeIcon[] = "\u0055";

void drawIcon(QPainter *painter,
              int x,
              int y,
              const QString &iconSymbol,
              int fontSize, int iconSize,
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
    return m_boundingRect.adjusted(-2, -2, 2, 2);
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
    m_boundingRect = m_paintedBoundingRect.united(m_selectionBoundingRect);
    setTransform(qmlItemNode().instanceTransformWithContentTransform());
    //the property for zValue is called z in QGraphicsObject
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

    foreach (QGraphicsItem *item, formEditorItem->childItems()) {
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

            painter->setClipRegion(boundingRect().toRect());

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

    foreach (QGraphicsItem *item, childItems()) {
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
    for (QGraphicsItem *item : scene()->items()) {
        if (item == this)
            continue;

        auto formEditorItem = qgraphicsitem_cast<FormEditorItem*>(item);
        if (formEditorItem)
            formEditorItem->updateGeometry();
    }

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

    QmlFlowItemNode flowItem(qmlItemNode());

    if (flowItem.isValid() && flowItem.flowView().isValid()) {
        const auto nodes = flowItem.flowView().transitions();
        for (const ModelNode &node : nodes) {
            FormEditorItem *item = scene()->itemForQmlItemNode(node);
            if (item)
                item->updateGeometry();
        }
    }

}

QPointF FormEditorFlowItem::instancePosition() const
{
    return qmlItemNode().flowPosition();
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

class ResolveConnection
{
public:
    ResolveConnection(const QmlItemNode &node) :
       from({})
      ,to(node.modelNode().bindingProperty("to").resolveToModelNode())
      ,areaNode(ModelNode())
    {
        if (node.modelNode().hasBindingProperty("from"))
            from = node.modelNode().bindingProperty("from").resolveToModelNode();
        const QmlFlowItemNode to = node.modelNode().bindingProperty("to").resolveToModelNode();

        if (from.isValid()) {
            for (const QmlFlowActionAreaNode &area : from.flowActionAreas()) {
                ModelNode target = area.targetTransition();
                if (target ==  node.modelNode()) {
                    areaNode = area;
                } else  {
                    const ModelNode decisionNode = area.decisionNodeForTransition(node.modelNode());
                    if (decisionNode.isValid()) {
                        from = decisionNode;
                        areaNode = ModelNode();
                    }
                }
            }
            if (from.modelNode().hasAuxiliaryData("joinConnection"))
                joinConnection = from.modelNode().auxiliaryData("joinConnection").toBool();
        } else {
            if (from == node.rootModelNode()) {
                isStartLine = true;
            } else {
                for (const ModelNode wildcard : QmlFlowViewNode(node.rootModelNode()).wildcards()) {
                    if (wildcard.bindingProperty("target").resolveToModelNode() == node.modelNode()) {
                        from = wildcard;
                        isWildcardLine = true;
                    }
                }
            }
        }
    }

    bool joinConnection = false;

    bool isStartLine = false;

    bool isWildcardLine = false;

    QmlFlowItemNode from;
    QmlFlowItemNode to;
    QmlFlowActionAreaNode areaNode;
};

void FormEditorTransitionItem::updateGeometry()
{
    FormEditorItem::updateGeometry();

    ResolveConnection resolved(qmlItemNode());

    QPointF fromP = QmlItemNode(resolved.from).flowPosition();
    QRectF sizeTo = resolved.to.instanceBoundingRect();

    QPointF toP = QmlItemNode(resolved.to).flowPosition();

    if (QmlItemNode(resolved.to).isFlowDecision())
        sizeTo = QRectF(0, 0, flowBlockSize, flowBlockSize);

    qreal x1 = fromP.x();
    qreal x2 = toP.x();

    if (x2 < x1) {
        qreal s = x1;
        x1 = x2;
        x2 = s;
    }

    qreal y1 = fromP.y();
    qreal y2 = toP.y();

    if (y2 < y1) {
        qreal s = y1;
        y1 = y2;
        y2 = s;
    }

    x2 += sizeTo.width();
    y2 += sizeTo.height();

    setX(x1);
    setY(y1);
    m_selectionBoundingRect = QRectF(0,0,x2-x1,y2-y1);
    m_paintedBoundingRect = m_selectionBoundingRect;
    m_boundingRect = m_selectionBoundingRect;
    setZValue(10);
}

QPointF FormEditorTransitionItem::instancePosition() const
{
    return qmlItemNode().flowPosition();
}

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

static void drawArrow(QPainter *painter,
                      const QLineF &line,
                      int arrowLength,
                      int arrowWidth)
{
    const QPointF peakP(0, 0);
    const QPointF leftP(-arrowLength, -arrowWidth * 0.5);
    const QPointF rightP(-arrowLength, arrowWidth * 0.5);

    painter->save();

    painter->translate(line.p2());
    painter->rotate(-line.angle());
    painter->drawLine(leftP, peakP);
    painter->drawLine(rightP, peakP);

    painter->restore();
}

static void drawRoundedCorner(QPainter *painter,
                              const QPointF &s,
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

    const QPointF arcStartP = s + (smNorm * (smLength - actualRadius)).toPointF();

    QRectF rect(m, QSizeF(actualRadius * 2, actualRadius * 2));

    painter->drawLine(s, arcStartP);

    if ((smNorm.y() < 0 && meNorm.x() > 0) || (smNorm.x() < 0 && meNorm.y() > 0)) {
        rect.moveTopLeft(m);
        painter->drawArc(rect, 90 * 16, 90 * 16);
    } else if ((smNorm.y() > 0 && meNorm.x() > 0) || (smNorm.x() < 0 && meNorm.y() < 0)) {
        rect.moveBottomLeft(m);
        painter->drawArc(rect, 180 * 16, 90 * 16);
    } else if ((smNorm.x() > 0 && meNorm.y() > 0) || (smNorm.y() < 0 && meNorm.x() < 0)) {
        rect.moveTopRight(m);
        painter->drawArc(rect, 0 * 16, 90 * 16);
    } else if ((smNorm.y() > 0 && meNorm.x() < 0) || (smNorm.x() > 0 && meNorm.y() < 0)) {
        rect.moveBottomRight(m);
        painter->drawArc(rect, 270 * 16, 90 * 16);
    }

    const QPointF arcEndP = e - (meNorm * (meLength - actualRadius)).toPointF();

    painter->drawLine(arcEndP, e);
}

static void paintConnection(QPainter *painter,
                            const QRectF &from,
                            const QRectF &to,
                            const ConnectionStyle &style)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen;
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCapStyle(Qt::RoundCap);
    pen.setColor(style.color);

    if (style.dash)
        pen.setStyle(Qt::DashLine);
    else
        pen.setStyle(Qt::SolidLine);
    pen.setWidthF(style.width);
    painter->setPen(pen);

    //const bool forceVertical = false;
    //const bool forceHorizontal = false;

    const int padding = 2 * style.width + 2 * style.adjustedWidth;

    const int arrowLength = 4 * style.adjustedWidth;
    const int arrowWidth = 8 * style.adjustedWidth;

    const bool boolExitRight = from.right() < to.center().x();
    const bool boolExitBottom = from.bottom() < to.center().y();

    bool horizontalFirst = true;

    /*
    if (verticalOverlap(from, to) && !horizontalOverlap(from, to))
        horizontalFirst = false;
    */

    const qreal middleFactor = style.breakOffset / 100.0;

    QPointF startP;
    QLineF lastSegment;

    bool extraLine = false;

    if (horizontalFirst) {
        if (to.center().x() > from.left() && to.center().x() < from.right()) {
            horizontalFirst = false;
            extraLine = true;
        } else if (verticalOverlap(from, to)) {
            horizontalFirst = true;
            extraLine = true;
        }

    } else {
        if (to.center().y() > from.top() && to.center().y() < from.bottom()) {
            horizontalFirst = true;
            extraLine = true;
        } else if (horizontalOverlap(from, to)) {
            horizontalFirst = false;
            extraLine = true;
        }
    }

    if (horizontalFirst) {
        const qreal startX = boolExitRight ? from.right() + padding : from.x() - padding;
        const qreal startY = from.center().y() + style.outOffset;

        startP = QPointF(startX, startY);

        const qreal endY = (from.bottom() > to.y()) ? to.bottom() + padding : to.top() - padding;

        if (!extraLine) {
            const qreal endX = to.center().x() + style.inOffset;
            const QPointF midP(endX, startY);
            const QPointF endP(endX, endY);

            if (style.radius == 0) {
                painter->drawLine(startP, midP);
                painter->drawLine(midP, endP);
            } else {
                drawRoundedCorner(painter, startP, midP, endP, style.radius);
            }

            lastSegment = QLineF(midP, endP);
        } else {
            const qreal endX = (from.right() > to.x()) ? to.right() + padding : to.left() - padding;
            const qreal midX = startX * middleFactor + endX * (1 - middleFactor);
            const QPointF midP(midX, startY);
            const QPointF midP2(midX, to.center().y() + style.inOffset);
            const QPointF endP(endX, to.center().y() + style.inOffset);

            if (style.radius == 0) {
                painter->drawLine(startP, midP);
                painter->drawLine(midP, midP2);
                painter->drawLine(midP2, endP);
            } else {
                const QLineF breakLine(midP, midP2);
                drawRoundedCorner(painter, startP, midP, breakLine.center(), style.radius);
                drawRoundedCorner(painter, breakLine.center(), midP2, endP, style.radius);
            }

            lastSegment = QLineF(midP2, endP);
        }

    } else {
        const qreal startX = from.center().x() + style.outOffset;
        const qreal startY = boolExitBottom ? from.bottom() + padding : from.top() - padding;

        startP = QPointF(startX, startY);

        const qreal endX = (from.right() > to.x()) ? to.right() + padding : to.left() - padding;

        if (!extraLine) {
            const qreal endY = to.center().y() + style.inOffset;
            const QPointF midP(startX, endY);
            const QPointF endP(endX, endY);

            if (style.radius == 0) {
                painter->drawLine(startP, midP);
                painter->drawLine(midP, endP);
            } else {
                drawRoundedCorner(painter, startP, midP, endP, style.radius);
            }

            lastSegment = QLineF(midP, endP);
        } else {
            const qreal endY = (from.bottom() > to.y()) ? to.bottom() + padding : to.top() - padding;

            const qreal midY = startY * middleFactor + endY * (1 - middleFactor);
            const QPointF midP(startX, midY);
            const QPointF midP2(to.center().x() + style.inOffset, midY);
            const QPointF endP(to.center().x() + style.inOffset, endY);

            if (style.radius == 0) {
                painter->drawLine(startP, midP);
                painter->drawLine(midP, midP2);
                painter->drawLine(midP2, endP);
            } else {
                const QLineF breakLine(midP, midP2);
                drawRoundedCorner(painter, startP, midP, breakLine.center(), style.radius);
                drawRoundedCorner(painter, breakLine.center(), midP2, endP, style.radius);
            }

            lastSegment = QLineF(midP2, endP);
        }
    }

    pen.setWidthF(style.width);
    pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);

    drawArrow(painter, lastSegment, arrowLength, arrowWidth);

    painter->setBrush(Qt::white);
    painter->drawEllipse(startP, arrowLength / 3, arrowLength / 3);

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

    if (!resolved.from.modelNode().isValid())
        return;

    QRectF fromRect = QmlItemNode(resolved.from).instanceBoundingRect();
    if (QmlItemNode(resolved.from).isFlowDecision())
        fromRect = QRectF(0, 0, flowBlockSize, flowBlockSize);

    if (QmlItemNode(resolved.from).isFlowWildcard())
        fromRect = QRectF(0, 0, flowBlockSize, flowBlockSize);
    fromRect.translate(QmlItemNode(resolved.from).flowPosition());

    if (resolved.isStartLine) {
        fromRect = QRectF(0,0,100,100);
        fromRect.translate(QmlItemNode(resolved.to).flowPosition()- QPoint(200, 0));
    }

    if (!resolved.joinConnection && resolved.areaNode.isValid()) {
        fromRect = QmlItemNode(resolved.areaNode).instanceBoundingRect();
        fromRect.translate(QmlItemNode(resolved.from).flowPosition());
        fromRect.translate(resolved.areaNode.instancePosition());
    }

    QRectF toRect = QmlItemNode(resolved.to).instanceBoundingRect();
    if (QmlItemNode(resolved.to).isFlowDecision())
        toRect = QRectF(0, 0, flowBlockSize,flowBlockSize);

    toRect.translate(QmlItemNode(resolved.to).flowPosition());

    if (resolved.isStartLine) {
        fromRect = QRectF(0, 0, 96, 96);
        fromRect.translate(QmlItemNode(resolved.to).flowPosition() + QPoint(-180, toRect.height() / 2 - 96 / 2));
    }

    toRect.translate(-pos());
    fromRect.translate(-pos());

    ConnectionStyle style;

    style.width = 2;

    const qreal scaleFactor = viewportTransform().m11();

    if (qmlItemNode().modelNode().hasAuxiliaryData("width"))
        style.width = qmlItemNode().modelNode().auxiliaryData("width").toInt();

    style.adjustedWidth = style.width / scaleFactor;

    if (qmlItemNode().modelNode().isSelected())
        style.width += 2;
    if (m_hitTest)
        style.width *= 8;

    style.color = "#e71919";

    if (resolved.isStartLine)
        style.color = "blue";

    if (resolved.isWildcardLine)
        style.color = "green";

    if (qmlItemNode().rootModelNode().hasAuxiliaryData("transitionColor"))
        style.color = qmlItemNode().rootModelNode().auxiliaryData("transitionColor").value<QColor>();

    if (qmlItemNode().modelNode().hasAuxiliaryData("color"))
        style.color = qmlItemNode().modelNode().auxiliaryData("color").value<QColor>();

    style.dash = false;

    if (qmlItemNode().modelNode().hasAuxiliaryData("dash"))
        style.dash = qmlItemNode().modelNode().auxiliaryData("dash").toBool();

    style.outOffset = 0;
    style.inOffset = 0;

    if (qmlItemNode().modelNode().hasAuxiliaryData("outOffset"))
        style.outOffset = qmlItemNode().modelNode().auxiliaryData("outOffset").toInt();

    if (qmlItemNode().modelNode().hasAuxiliaryData("inOffset"))
        style.inOffset = qmlItemNode().modelNode().auxiliaryData("inOffset").toInt();

    style.breakOffset = 50;

    if (qmlItemNode().modelNode().hasAuxiliaryData("breakPoint"))
        style.breakOffset = qmlItemNode().modelNode().auxiliaryData("breakPoint").toInt();

    style.radius = 8;

    if (qmlItemNode().rootModelNode().hasAuxiliaryData("transitionRadius"))
        style.radius = qmlItemNode().rootModelNode().auxiliaryData("transitionRadius").toInt();

    if (qmlItemNode().modelNode().hasAuxiliaryData("radius"))
        style.radius = qmlItemNode().modelNode().auxiliaryData("radius").toInt();

    if (resolved.isStartLine)
        fromRect.translate(0, style.outOffset);

    paintConnection(painter, fromRect, toRect, style);

    if (resolved.isStartLine) {

        const QString icon = Theme::getIconUnicode(Theme::startNode);

        QPen pen;
        pen.setCosmetic(true);
        pen.setColor(style.color);
        painter->setPen(pen);

        const int iconAdjust = 48;
        const int offset = 96;
        const int size = fromRect.width();
        const int iconSize = size - iconAdjust;
        const int x = fromRect.topRight().x() - offset;
        const int y = fromRect.topRight().y();
        painter->drawRoundedRect(x, y , size - 10, size, size / 2, iconSize / 2);
        drawIcon(painter, x + iconAdjust / 2, y + iconAdjust / 2, icon, iconSize, iconSize, style.color);
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
    m_selectionBoundingRect = QRectF(0,0, flowBlockSize, flowBlockSize);
    m_paintedBoundingRect = m_selectionBoundingRect;
    m_boundingRect = m_paintedBoundingRect;
    setTransform(qmlItemNode().instanceTransformWithContentTransform());
    const QPointF pos = qmlItemNode().flowPosition();
    setTransform(QTransform::fromTranslate(pos.x(), pos.y()));
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

    if (m_iconType == DecisionIcon) {
        painter->translate(boundingRect().center());
        painter->rotate(45);
        painter->translate(-boundingRect().center());
    }

    if (fillColor.alpha() > 0)
        painter->setBrush(fillColor);

    int radius = blockRadius;

    const QRectF adjustedRect = boundingRect().adjusted(blockAdjust,
                                                        blockAdjust,
                                                        -blockAdjust,
                                                        -blockAdjust);

    painter->drawRoundedRect(adjustedRect, radius, radius);

    const int iconDecrement = 32;
    const int iconSize = adjustedRect.width() - iconDecrement;
    const int offset = iconDecrement / 2 + blockAdjust;

    painter->restore();

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
