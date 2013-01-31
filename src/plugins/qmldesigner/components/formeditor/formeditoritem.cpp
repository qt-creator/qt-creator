/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "formeditoritem.h"
#include "formeditorscene.h"
#include "formeditornodeinstanceview.h"
#include "selectiontool.h"

#include <modelnode.h>
#include <nodemetainfo.h>
#include <qmlanchors.h>


#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsView>
#include <QTimeLine>

#include <cmath>

#include <invalidmodelnodeexception.h>

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

void FormEditorItem::updateGeometry()
{
    prepareGeometryChange();
    m_boundingRect = qmlItemNode().instancePaintedBoundingRect().adjusted(0, 0, 1., 1.);
    setTransform(qmlItemNode().instanceTransform());
    setTransform(m_attentionTransform, true);
    //the property for zValue is called z in QGraphicsObject
    if (qmlItemNode().instanceValue("z").isValid())
        setZValue(qmlItemNode().instanceValue("z").toDouble());
}

void FormEditorItem::updateVisibilty()
{
//    setVisible(nodeInstance().isVisible());
//    setOpacity(nodeInstance().opacity());
}

void FormEditorItem::showAttention()
{
    if (m_attentionTimeLine.isNull()) {
        m_attentionTimeLine = new QTimeLine(500, this);
        m_attentionTimeLine->setCurveShape(QTimeLine::SineCurve);
        connect(m_attentionTimeLine.data(), SIGNAL(valueChanged(qreal)), SLOT(changeAttention(qreal)));
        connect(m_attentionTimeLine.data(), SIGNAL(finished()), m_attentionTimeLine.data(), SLOT(deleteLater()));

        m_attentionTimeLine->start();
    }
}

void FormEditorItem::changeAttention(qreal value)
{
    if (QGraphicsItem::parentItem() == scene()->formLayerItem()) {
        setAttentionHighlight(value);
    } else {
        setAttentionHighlight(value);
        setAttentionScale(value);
    }
}

FormEditorView *FormEditorItem::formEditorView() const
{
    return scene()->editorView();
}

void FormEditorItem::setAttentionScale(double sinusScale)
{
    if (!qFuzzyIsNull(sinusScale)) {
        double scale = std::sqrt(sinusScale);
        m_attentionTransform.reset();
        QPointF centerPoint(qmlItemNode().instanceBoundingRect().center());
        m_attentionTransform.translate(centerPoint.x(), centerPoint.y());
        m_attentionTransform.scale(scale * 0.15 + 1.0, scale * 0.15 + 1.0);
        m_attentionTransform.translate(-centerPoint.x(), -centerPoint.y());
        m_inverseAttentionTransform = m_attentionTransform.inverted();
        prepareGeometryChange();
        setTransform(qmlItemNode().instanceTransform());
        setTransform(m_attentionTransform, true);
    } else {
        m_attentionTransform.reset();
        prepareGeometryChange();
        setTransform(qmlItemNode().instanceTransform());
    }
}

void FormEditorItem::setAttentionHighlight(double value)
{
    if (QGraphicsItem::parentItem() == scene()->formLayerItem())
        m_borderWidth = value * 4;
    else
        m_borderWidth = 1. + value * 3;

    update();
}

void FormEditorItem::setHighlightBoundingRect(bool highlight)
{
    if (m_highlightBoundingRect != highlight) {
        m_highlightBoundingRect = highlight;
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
    if (!boundingRect().isValid()
        || (QGraphicsItem::parentItem() == scene()->formLayerItem() && qFuzzyIsNull(m_borderWidth)))
          return;

     if (boundingRect().width() < 8 || boundingRect().height() < 8)
         return;

    QPen pen;
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setStyle(Qt::DotLine);

    QColor frameColor("#AAAAAA");

    if (scene()->showBoundingRects()) {
        if (m_highlightBoundingRect)
            pen.setColor(frameColor);
        else
            pen.setColor(frameColor.darker(150));
    } else {
        if (m_highlightBoundingRect)
            pen.setColor(frameColor);
        else
            pen.setColor(Qt::transparent);
    }

    painter->setPen(pen);
//    int offset =  m_borderWidth / 2;

    painter->drawRect(boundingRect().adjusted(0., 0., -1., -1.));
}

void FormEditorItem::paintPlaceHolderForInvisbleItem(QPainter *painter) const
{
    qreal stripesWidth = 12;

    QRegion innerRegion = QRegion(boundingRect().adjusted(stripesWidth, stripesWidth, -stripesWidth, -stripesWidth).toRect());
    QRegion outerRegion  = QRegion(boundingRect().toRect()) - innerRegion;

    painter->setClipRegion(outerRegion);
    painter->setClipping(true);
    painter->fillRect(boundingRect().adjusted(1, 1, -1, -1), Qt::BDiagPattern);
    painter->setClipping(false);

    QString displayText = qmlItemNode().id();

    if (displayText.isEmpty())
        displayText = qmlItemNode().simplifiedTypeName();

    QTextOption textOption;
    textOption.setAlignment(Qt::AlignTop);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    if (boundingRect().height() > 60) {
        painter->save();

        QFont font;
        font.setStyleHint(QFont::SansSerif);
        font.setBold(true);
        font.setPixelSize(12);
        painter->setFont(font);

        QFontMetrics fm(font);
        painter->rotate(90);
        if (fm.width(displayText) > (boundingRect().height() - 32) && displayText.length() > 4) {

            displayText = fm.elidedText(displayText, Qt::ElideRight, boundingRect().height() - 32, Qt::TextShowMnemonic);
        }

        QRectF rotatedBoundingBox;
        rotatedBoundingBox.setWidth(boundingRect().height());
        rotatedBoundingBox.setHeight(12);
        rotatedBoundingBox.setY(-boundingRect().width() + 12);
        rotatedBoundingBox.setX(20);

        painter->setFont(font);
        painter->setPen(QColor(48, 48, 96, 255));
        painter->drawText(rotatedBoundingBox, displayText, textOption);

        painter->restore();
    }
}


void FormEditorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!painter->isActive())
        return;

    if (!qmlItemNode().isValid())
        return;

    painter->save();

    if (qmlItemNode().instanceIsRenderPixmapNull() || !isContentVisible()) {
        if (scene()->showBoundingRects() && boundingRect().width() > 15 && boundingRect().height() > 15)
            paintPlaceHolderForInvisbleItem(painter);
    } else {
        qmlItemNode().paintInstance(painter);
    }

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
    m_snappingLineCreator.update(exceptionList, transformationSpaceItem);
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

bool FormEditorItem::isContainer() const
{
    return true;
}

QmlItemNode FormEditorItem::qmlItemNode() const
{
    return m_qmlItemNode;
}

}

