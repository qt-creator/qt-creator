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

#include "formeditoritem.h"
#include "formeditorscene.h"

#include <modelnode.h>
#include <nodemetainfo.h>

#include <QDebug>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTimeLine>

#include <cmath>

namespace QmlDesigner {


FormEditorScene *FormEditorItem::scene() const {
    return qobject_cast<FormEditorScene*>(QGraphicsItem::scene());
}

FormEditorItem::FormEditorItem(const QmlItemNode &qmlItemNode, FormEditorScene* scene)
    : QGraphicsObject(scene->formLayerItem()),
    m_snappingLineCreator(this),
    m_qmlItemNode(qmlItemNode),
    m_borderWidth(1.0),
    m_highlightBoundingRect(false),
    m_blurContent(false),
    m_isContentVisible(true),
    m_isFormEditorVisible(true)
{
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setup();
}

void FormEditorItem::setup()
{
    if (qmlItemNode().hasInstanceParent()) {
        setParentItem(scene()->itemForQmlItemNode(qmlItemNode().instanceParent().toQmlItemNode()));
        setOpacity(qmlItemNode().instanceValue("opacity").toDouble());
    }

    setFlag(QGraphicsItem::ItemClipsChildrenToShape, qmlItemNode().instanceValue("clip").toBool());

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
    m_boundingRect = m_paintedBoundingRect.united(m_selectionBoundingRect);
    setTransform(qmlItemNode().instanceTransformWithContentTransform());
    //the property for zValue is called z in QGraphicsObject
    if (qmlItemNode().instanceValue("z").isValid())
        setZValue(qmlItemNode().instanceValue("z").toDouble());
}

void FormEditorItem::updateVisibilty()
{
//    setVisible(nodeInstance().isVisible());
//    setOpacity(nodeInstance().opacity());
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

//static QRectF alignedRect(const QRectF &rect)
//{
//    QRectF alignedRect(rect);
//    alignedRect.setTop(std::floor(rect.top()) + 0.5);
//    alignedRect.setBottom(std::floor(rect.bottom()) + 0.5);
//    alignedRect.setLeft(std::floor(rect.left()) + 0.5);
//    alignedRect.setRight(std::floor(rect.right()) + 0.5);
//
//    return alignedRect;
//}

void FormEditorItem::paintBoundingRect(QPainter *painter) const
{
    if (!m_boundingRect.isValid()
        || (QGraphicsItem::parentItem() == scene()->formLayerItem() && qFuzzyIsNull(m_borderWidth)))
          return;

     if (m_boundingRect.width() < 8 || m_boundingRect.height() < 8)
         return;

    QPen pen;
    pen.setJoinStyle(Qt::MiterJoin);

    QColor frameColor("#AAAAAA");

    if (scene()->showBoundingRects()) {
        if (m_highlightBoundingRect) {
            pen.setColor(frameColor);
        } else {
            pen.setColor(frameColor.darker(150));
            pen.setStyle(Qt::DotLine);
        }
    } else {
        if (m_highlightBoundingRect) {
            pen.setColor(frameColor);
        } else {
            pen.setColor(Qt::transparent);
            pen.setStyle(Qt::DotLine);
        }
    }

    painter->setPen(pen);
//    int offset =  m_borderWidth / 2;

    painter->drawRect(m_boundingRect.adjusted(0., 0., -1., -1.));
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
        if (fm.width(displayText) > (boundingRect.height() - 32) && displayText.length() > 4) {

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
    qreal stripesWidth = 12;

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

    if (qmlItemNode().instanceIsRenderPixmapNull() || !isContentVisible()) {
        if (scene()->showBoundingRects() && m_boundingRect.width() > 15 && m_boundingRect.height() > 15)
            paintPlaceHolderForInvisbleItem(painter);
    } else {
        if (m_blurContent)
            painter->drawPixmap(m_paintedBoundingRect.topLeft(), qmlItemNode().instanceBlurredRenderPixmap());
        else
            painter->drawPixmap(m_paintedBoundingRect.topLeft(), qmlItemNode().instanceRenderPixmap());
    }

    if (!qmlItemNode().isRootModelNode())
        paintBoundingRect(painter);

//    if (qmlItemNode().modelNode().metaInfo().isSubclassOf("QtQuick.Loader", -1, -1))
//        paintComponentContentVisualisation(painter, boundingRect());

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

}

