// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditoritem.h"
#include "formeditorscene.h"
#include "formeditortracing.h"

#include <auxiliarydataproperties.h>
#include <bindingproperty.h>
#include <qmldesignertr.h>
#include <variantproperty.h>

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
#include <QtMath>

#include <cmath>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using FormEditorTracing::category;

FormEditorScene *FormEditorItem::scene() const {
    return qobject_cast<FormEditorScene*>(QGraphicsItem::scene());
}

FormEditorItem::FormEditorItem(const QmlItemNode &qmlItemNode, FormEditorScene* scene)
    : QGraphicsItem(scene->formLayerItem()),
    m_snappingLineCreator(this),
    m_qmlItemNode(qmlItemNode)
{
    NanotraceHR::Tracer tracer{"form editor item constructor",
                               category(),
                               keyValue("qmlItemNode", qmlItemNode.id())};

    setCacheMode(QGraphicsItem::NoCache);
    setup();
}

void FormEditorItem::setup()
{
    NanotraceHR::Tracer tracer{"form editor item setup", category()};

    setAcceptedMouseButtons(Qt::NoButton);
    const auto &itemNode = qmlItemNode();

    if (itemNode.hasInstanceParent()) {
        setParentItem(scene()->itemForQmlItemNode(itemNode.instanceParent().toQmlItemNode()));
        setOpacity(itemNode.instanceValue("opacity").toDouble());
    }

    setFlag(QGraphicsItem::ItemClipsToShape, itemNode.instanceValue("clip").toBool());
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, itemNode.instanceValue("clip").toBool());

    if (NodeHints::fromModelNode(itemNode).forceClip())
        setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    if (QGraphicsItem::parentItem() == scene()->formLayerItem())
        m_borderWidth = 0.0;

    setContentVisible(itemNode.instanceValue("visible").toBool());

    if (itemNode.modelNode().auxiliaryDataWithDefault(invisibleProperty).toBool())
        setVisible(false);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemNegativeZStacksBehindParent, true);
    updateGeometry();
    updateVisibilty();
}

QRectF FormEditorItem::boundingRect() const
{
    NanotraceHR::Tracer tracer{"form editor item bounding rect", category()};

    // Corner case: painting outside the bounding rectangle (boundingRec < paintedBoundingRect), which can be set in the Text items, for example.
    // QGraphicsItem needs valid painting boundaries returned by boundingRect(). Returning a bounding rectangle that is too small will cause artefacts in the view.
    return m_paintedBoundingRect;
}

QPainterPath FormEditorItem::shape() const
{
    NanotraceHR::Tracer tracer{"form editor item shape", category()};

    QPainterPath painterPath;
    painterPath.addRect(m_selectionBoundingRect);

    return painterPath;
}

bool FormEditorItem::contains(const QPointF &point) const
{
    NanotraceHR::Tracer tracer{"form editor item contains", category()};

    return m_selectionBoundingRect.contains(point);
}

void FormEditorItem::updateGeometry()
{
    NanotraceHR::Tracer tracer{"form editor item update geometry", category()};

    prepareGeometryChange();
    const auto &itemNode = qmlItemNode();

    m_selectionBoundingRect = itemNode.instanceBoundingRect().adjusted(0, 0, 1., 1.);
    m_paintedBoundingRect = itemNode.instancePaintedBoundingRect();
    m_boundingRect = itemNode.instanceBoundingRect();
    setTransform(itemNode.instanceTransformWithContentTransform());
    // the property for zValue is called z in QGraphicsObject
    if (itemNode.instanceValue("z").isValid() && !itemNode.isRootModelNode())
        setZValue(itemNode.instanceValue("z").toDouble());
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
    NanotraceHR::Tracer tracer{"form editor item set highlight bounding rect",
                               category(),
                               keyValue("highlight", highlight)};

    if (m_highlightBoundingRect != highlight) {
        m_highlightBoundingRect = highlight;
        update();
    }
}

void FormEditorItem::blurContent(bool blurContent)
{
    NanotraceHR::Tracer tracer{"form editor item blur content",
                               category(),
                               keyValue("blur content", blurContent)};

    if (!scene())
        return;

    if (m_blurContent != blurContent) {
        m_blurContent = blurContent;
        update();
    }
}

void FormEditorItem::setContentVisible(bool visible)
{
    NanotraceHR::Tracer tracer{"form editor item set content visible",
                               category(),
                               keyValue("visible", visible)};

    if (visible == m_isContentVisible)
        return;

    m_isContentVisible = visible;
    update();
}

bool FormEditorItem::isContentVisible() const
{
    NanotraceHR::Tracer tracer{"form editor item is content visible", category()};

    if (parentItem())
        return parentItem()->isContentVisible() && m_isContentVisible;

    return m_isContentVisible;
}

QPointF FormEditorItem::center() const
{
    NanotraceHR::Tracer tracer{"form editor item center", category()};

    return mapToScene(qmlItemNode().instanceBoundingRect().center());
}

qreal FormEditorItem::selectionWeigth(const QPointF &point, int iteration)
{
    NanotraceHR::Tracer tracer{"form editor item selection weight",
                               category(),
                               keyValue("iteration", iteration)};

    const auto &itemNode = qmlItemNode();

    if (!itemNode.isValid())
        return 100000;

    QRectF boundingRect = mapRectToScene(itemNode.instanceBoundingRect());

    float weight = point.x()- boundingRect.left()
            + point.y() - boundingRect.top()
            + boundingRect.right()- point.x()
            + boundingRect.bottom() - point.y()
            + (center() - point).manhattanLength()
            + sqrt(boundingRect.width() * boundingRect.height()) / 2 * iteration;

    return weight;
}

void FormEditorItem::synchronizeOtherProperty(PropertyNameView propertyName)
{
    NanotraceHR::Tracer tracer{"form editor item synchronize other property",
                               category(),
                               keyValue("propertyName", propertyName)};

    const auto &itemNode = qmlItemNode();

    if (propertyName == "opacity")
        setOpacity(itemNode.instanceValue("opacity").toDouble());

    if (propertyName == "clip") {
        setFlag(QGraphicsItem::ItemClipsToShape, itemNode.instanceValue("clip").toBool());
        setFlag(QGraphicsItem::ItemClipsChildrenToShape, itemNode.instanceValue("clip").toBool());
    }
    if (NodeHints::fromModelNode(itemNode).forceClip())
        setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    if (propertyName == "z")
        setZValue(itemNode.instanceValue("z").toDouble());

    if (propertyName == "visible")
        setContentVisible(itemNode.instanceValue("visible").toBool());
}

void FormEditorItem::setDataModelPosition(const QPointF &position)
{
    NanotraceHR::Tracer tracer{"form editor item set data model position", category()};
    qmlItemNode().setPosition(position);
}

void FormEditorItem::setDataModelPositionInBaseState(const QPointF &position)
{
    NanotraceHR::Tracer tracer{"form editor item set data model position in base state", category()};

    qmlItemNode().setPostionInBaseState(position);
}

QPointF FormEditorItem::instancePosition() const
{
    NanotraceHR::Tracer tracer{"form editor item instance position", category()};

    return qmlItemNode().instancePosition();
}

QTransform FormEditorItem::instanceSceneTransform() const
{
    NanotraceHR::Tracer tracer{"form editor item instance scene transform", category()};

    return qmlItemNode().instanceSceneTransform();
}

QTransform FormEditorItem::instanceSceneContentItemTransform() const
{
    NanotraceHR::Tracer tracer{"form editor item instance scene content item transform", category()};

    return qmlItemNode().instanceSceneContentItemTransform();
}

bool FormEditorItem::flowHitTest(const QPointF & ) const
{
    NanotraceHR::Tracer tracer{"form editor item flow hit test", category()};

    return false;
}

void FormEditorItem::setFrameColor(const QColor &color)
{
    NanotraceHR::Tracer tracer{"form editor item set frame color", category()};

    m_frameColor = color;
    update();
}

void FormEditorItem::setHasEffect(bool hasEffect)
{
    NanotraceHR::Tracer tracer{"form editor item set has effect",
                               category(),
                               keyValue("hasEffect", hasEffect)};

    m_hasEffect = hasEffect;
}

bool FormEditorItem::hasEffect() const
{
    NanotraceHR::Tracer tracer{"form editor item has effect", category()};

    return m_hasEffect;
}

bool FormEditorItem::parentHasEffect() const
{
    NanotraceHR::Tracer tracer{"form editor item parent has effect", category()};

    FormEditorItem *pi = parentItem();
    while (pi) {
        if (pi->hasEffect())
            return true;
        pi = pi->parentItem();
    }
    return false;
}

FormEditorItem::~FormEditorItem()
{
    NanotraceHR::Tracer tracer{"form editor item destructor", category()};

    scene()->removeItemFromHash(this);
}

/* \brief returns the parent item skipping all proxyItem*/
FormEditorItem *FormEditorItem::parentItem() const
{
    NanotraceHR::Tracer tracer{"form editor item parent item", category()};

    return qgraphicsitem_cast<FormEditorItem*> (QGraphicsItem::parentItem());
}

FormEditorItem* FormEditorItem::fromQGraphicsItem(QGraphicsItem *graphicsItem)
{
    return qgraphicsitem_cast<FormEditorItem*>(graphicsItem);
}

void FormEditorItem::paintBoundingRect(QPainter *painter) const
{
    NanotraceHR::Tracer tracer{"form editor item paint bounding rect", category()};

    if (!m_boundingRect.isValid()
        || (QGraphicsItem::parentItem() == scene()->formLayerItem() && qFuzzyIsNull(m_borderWidth)))
          return;

     if (m_boundingRect.width() < 8 || m_boundingRect.height() < 8)
         return;

    QPen pen;
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::MiterJoin);

    const QColor frameColor(0xaa, 0xaa, 0xaa);
    static const QColor selectionColor = Utils::creatorColor(
        Utils::Theme::QmlDesigner_FormEditorSelectionColor);

    if (scene()->showBoundingRects()) {
        pen.setColor(frameColor.darker(150));
        pen.setStyle(Qt::DotLine);
        painter->setPen(pen);
        painter->drawRect(m_boundingRect.adjusted(0., 0., -1., -1.));
    }

    if (m_frameColor.isValid()) {
        pen.setColor(m_frameColor);
        pen.setStyle(Qt::SolidLine);
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
        if (fm.horizontalAdvance(displayText) > (boundingRect.height() - 32) && displayText.size() > 4) {

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
    NanotraceHR::Tracer tracer{"form editor item paint place holder for invisible item", category()};

    painter->save();
    paintDecorationInPlaceHolderForInvisbleItem(painter, m_boundingRect);
    const auto &itemNode = qmlItemNode();
    paintTextInPlaceHolderForInvisbleItem(painter,
                                          itemNode.id(),
                                          itemNode.simplifiedTypeName(),
                                          m_boundingRect);
    painter->restore();
}

void FormEditorItem::paintComponentContentVisualisation(QPainter *painter, const QRectF &clippinRectangle) const
{
    NanotraceHR::Tracer tracer{"form editor item paint component content visualisation", category()};

    painter->setBrush(QColor(0, 0, 0, 150));
    painter->fillRect(clippinRectangle, Qt::BDiagPattern);
}

QList<FormEditorItem *> FormEditorItem::offspringFormEditorItemsRecursive(const FormEditorItem *formEditorItem) const
{
    NanotraceHR::Tracer tracer{"form editor item  offsprint form editor items recursive", category()};

    QList<FormEditorItem *> formEditorItemList;

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
    NanotraceHR::Tracer tracer{"form editor item paint", category()};

    if (!painter->isActive())
        return;

    const auto &itemNode = qmlItemNode();
    if (!qmlItemNode().isValid())
        return;

    painter->save();

    bool showPlaceHolder = itemNode.instanceIsRenderPixmapNull() || !isContentVisible();

    const bool isInStackedContainer = itemNode.isInStackedContainer();

    /* If already the parent is invisible then show nothing */
    const bool hideCompletely = !isContentVisible() && (parentItem() && !parentItem()->isContentVisible());

    if (isInStackedContainer)
        showPlaceHolder = itemNode.instanceIsRenderPixmapNull() && isContentVisible();

    if (!hideCompletely && !parentHasEffect()) {
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
                painter->drawPixmap(m_paintedBoundingRect.topLeft(),
                                    itemNode.instanceBlurredRenderPixmap());
            else
                painter->drawPixmap(m_paintedBoundingRect.topLeft(), itemNode.instanceRenderPixmap());

            painter->restore();
        }
    }

    if (!itemNode.isRootModelNode())
        paintBoundingRect(painter);

    painter->restore();
}

AbstractFormEditorTool* FormEditorItem::tool() const
{
    return static_cast<FormEditorScene*>(scene())->currentTool();
}

SnapLineMap FormEditorItem::topSnappingLines() const
{
    NanotraceHR::Tracer tracer{"form editor item top snapping lines", category()};

    return m_snappingLineCreator.topLines();
}

SnapLineMap FormEditorItem::bottomSnappingLines() const
{
    NanotraceHR::Tracer tracer{"form editor item bottom snapping lines", category()};

    return m_snappingLineCreator.bottomLines();
}

SnapLineMap FormEditorItem::leftSnappingLines() const
{
    NanotraceHR::Tracer tracer{"form editor item left snapping lines", category()};

    return m_snappingLineCreator.leftLines();
}

SnapLineMap FormEditorItem::rightSnappingLines() const
{
    NanotraceHR::Tracer tracer{"form editor item right snapping lines", category()};

    return m_snappingLineCreator.rightLines();
}

SnapLineMap FormEditorItem::horizontalCenterSnappingLines() const
{
    NanotraceHR::Tracer tracer{"form editor item horizontal center snapping lines", category()};

    return m_snappingLineCreator.horizontalCenterLines();
}

SnapLineMap FormEditorItem::verticalCenterSnappingLines() const
{
    NanotraceHR::Tracer tracer{"form editor item vertical center snapping lines", category()};

    return m_snappingLineCreator.verticalCenterLines();
}

SnapLineMap FormEditorItem::topSnappingOffsets() const
{
    NanotraceHR::Tracer tracer{"form editor item top snapping offsets", category()};

    return m_snappingLineCreator.topOffsets();
}

SnapLineMap FormEditorItem::bottomSnappingOffsets() const
{
    NanotraceHR::Tracer tracer{"form editor item bottom snapping offsets", category()};

    return m_snappingLineCreator.bottomOffsets();
}

SnapLineMap FormEditorItem::leftSnappingOffsets() const
{
    NanotraceHR::Tracer tracer{"form editor item left snapping offsets", category()};

    return m_snappingLineCreator.leftOffsets();
}

SnapLineMap FormEditorItem::rightSnappingOffsets() const
{
    NanotraceHR::Tracer tracer{"form editor item right snapping offsets", category()};

    return m_snappingLineCreator.rightOffsets();
}


void FormEditorItem::updateSnappingLines(const QList<FormEditorItem*> &exceptionList,
                                         FormEditorItem *transformationSpaceItem)
{
    NanotraceHR::Tracer tracer{"form editor item update snapping lines", category()};

    m_snappingLineCreator.update(exceptionList, transformationSpaceItem, this);
}

QList<FormEditorItem*> FormEditorItem::childFormEditorItems() const
{
    NanotraceHR::Tracer tracer{"form editor item child form editor items", category()};

    QList<FormEditorItem *> formEditorItemList;

    for (QGraphicsItem *item : childItems()) {
        FormEditorItem *formEditorItem = fromQGraphicsItem(item);
        if (formEditorItem)
            formEditorItemList.append(formEditorItem);
    }

    return formEditorItemList;
}

QList<FormEditorItem *> FormEditorItem::offspringFormEditorItems() const
{
    NanotraceHR::Tracer tracer{"form editor item offspring form editor items", category()};

    return offspringFormEditorItemsRecursive(this);
}

bool FormEditorItem::isContainer() const
{
    NanotraceHR::Tracer tracer{"form editor item is container", category()};

    NodeMetaInfo nodeMetaInfo = qmlItemNode().modelNode().metaInfo();

    if (nodeMetaInfo.isValid())
        return !nodeMetaInfo.defaultPropertyIsComponent() && !nodeMetaInfo.isLayoutable();

    return true;
}

QTransform FormEditorItem::viewportTransform() const
{
    NanotraceHR::Tracer tracer{"form editor transition item viewport transform", category()};

    QTC_ASSERT(scene(), return {});
    QTC_ASSERT(!scene()->views().isEmpty(), return {});

    return scene()->views().first()->viewportTransform();
}

qreal FormEditorItem::getItemScaleFactor() const
{
    NanotraceHR::Tracer tracer{"form editor item get item scale factor", category()};

    return 1.0 / viewportTransform().m11();
}

qreal FormEditorItem::getLineScaleFactor() const
{
    NanotraceHR::Tracer tracer{"form editor item get line scale factor", category()};

    return 2 / qSqrt(viewportTransform().m11());
}

qreal FormEditorItem::getTextScaleFactor() const
{
    NanotraceHR::Tracer tracer{"form editor item get text scale factor", category()};

    return 2 / qSqrt(viewportTransform().m11());
}

void FormEditor3dPreview::updateGeometry()
{
    NanotraceHR::Tracer tracer{"form editor 3d preview update geometry", category()};

    prepareGeometryChange();

    m_boundingRect = qmlItemNode().instanceBoundingRect();
    if (m_boundingRect.isEmpty())
        m_boundingRect = {0, 0, 640, 480}; // Init to default size so initial view is correct
    m_selectionBoundingRect = m_boundingRect.adjusted(0, 0, 1., 1.);
    m_paintedBoundingRect = m_boundingRect;
    setTransform(QTransform());
}

QPointF FormEditor3dPreview::instancePosition() const
{
    NanotraceHR::Tracer tracer{"form editor 3d preview instance position", category()};

    return QPointF(0, 0);
}

void FormEditor3dPreview::paint(QPainter *painter,
                                [[maybe_unused]] const QStyleOptionGraphicsItem *option,
                                [[maybe_unused]] QWidget *widget)
{
    NanotraceHR::Tracer tracer{"form editor 3d preview paint", category()};

    if (!painter->isActive())
        return;

    painter->save();

    const auto &itemNode = qmlItemNode();

    bool showPlaceHolder = itemNode.instanceIsRenderPixmapNull();

    if (showPlaceHolder)
        paintPlaceHolderForInvisbleItem(painter);
    else
        painter->drawPixmap(m_boundingRect.topLeft(), itemNode.instanceRenderPixmap());

    painter->restore();
}

} //QmlDesigner
