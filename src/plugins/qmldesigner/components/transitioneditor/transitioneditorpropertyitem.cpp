// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "transitioneditorpropertyitem.h"

#include "abstractview.h"
#include "timelineconstants.h"
#include "timelineicons.h"
#include "transitioneditorgraphicsscene.h"

#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <rewritingexception.h>
#include <theme.h>
#include <variantproperty.h>
#include <qmlobjectnode.h>

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <coreplugin/icore.h>

#include <QCursor>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>

#include <algorithm>

namespace QmlDesigner {

TransitionEditorPropertyItem *TransitionEditorPropertyItem::create(
    const ModelNode &animation, TransitionEditorSectionItem *parent)
{
    auto item = new TransitionEditorPropertyItem(parent);
    item->m_animation = animation;

    auto sectionItem = new QGraphicsWidget(item);

    sectionItem->setGeometry(0,
                             0,
                             TimelineConstants::sectionWidth,
                             TimelineConstants::sectionHeight);

    sectionItem->setZValue(10);
    sectionItem->setCursor(Qt::ArrowCursor);

    item->setToolTip(item->propertyName());
    item->resize(parent->size());

    item->m_barItem = new TransitionEditorBarItem(item);
    item->invalidateBar();

    return item;
}

int TransitionEditorPropertyItem::type() const
{
    return Type;
}

void TransitionEditorPropertyItem::updateData()
{
    invalidateBar();
}

void TransitionEditorPropertyItem::updateParentData()
{
    TransitionEditorSectionItem::invalidateBar(parentItem());
}

bool TransitionEditorPropertyItem::isSelected() const
{
    return transitionEditorGraphicsScene()->selectedPropertyItem() == this;
}

QString TransitionEditorPropertyItem::propertyName() const
{
    if (m_animation.isValid()) {
        const QString propertyName = m_animation.variantProperty("property").value().toString();
        if (!propertyName.isEmpty())
            return propertyName;
        return m_animation.variantProperty("properties").value().toString();
    }
    return QString();
}

void TransitionEditorPropertyItem::paint(QPainter *painter,
                                         const QStyleOptionGraphicsItem *,
                                         QWidget *)
{
    painter->save();

    static const QColor penColor = Theme::getColor(Theme::BackgroundColorDark);
    static const QColor textColor = Theme::getColor(Theme::PanelTextColorLight);
    static const QColor backgroundColor = Theme::getColor(Theme::DScontrolBackground);

    painter->fillRect(0, 0, TimelineConstants::sectionWidth, size().height(), backgroundColor);
    painter->fillRect(TimelineConstants::textIndentationProperties - 4,
                      0,
                      TimelineConstants::sectionWidth - TimelineConstants::textIndentationProperties
                          + 4,
                      size().height(),
                      backgroundColor.darker(110));

    painter->setPen(penColor);

    drawLine(painter,
             TimelineConstants::sectionWidth - 1,
             0,
             TimelineConstants::sectionWidth - 1,
             size().height());

    drawLine(painter,
             TimelineConstants::textIndentationProperties - 4,
             TimelineConstants::sectionHeight - 1,
             size().width(),
             TimelineConstants::sectionHeight - 1);

    painter->setPen(textColor);

    const QFontMetrics metrics(font());

    const QString elidedText = metrics.elidedText(propertyName(),
                                                  Qt::ElideMiddle,
                                                  qreal(TimelineConstants::sectionWidth) * 2.0 / 3
                                                      - TimelineConstants::textIndentationProperties,
                                                  0);

    painter->drawText(TimelineConstants::textIndentationProperties, 12, elidedText);

    painter->restore();
}

void TransitionEditorPropertyItem::contextMenuEvent(QGraphicsSceneContextMenuEvent * /*event */) {}

TransitionEditorPropertyItem::TransitionEditorPropertyItem(TransitionEditorSectionItem *parent)
    : TimelineItem(parent)
{
    setPreferredHeight(TimelineConstants::sectionHeight);
    setMinimumHeight(TimelineConstants::sectionHeight);
    setMaximumHeight(TimelineConstants::sectionHeight);
}

TransitionEditorGraphicsScene *TransitionEditorPropertyItem::transitionEditorGraphicsScene() const
{
    return qobject_cast<TransitionEditorGraphicsScene *>(scene());
}

void TransitionEditorPropertyItem::invalidateBar()
{
    qreal min = 0;
    qreal max = 0;

    QTC_ASSERT(m_animation.isValid(), return );
    QTC_ASSERT(m_animation.hasParentProperty(), return );

    const ModelNode parent = m_animation.parentProperty().parentModelNode();

    for (const ModelNode &child : parent.directSubModelNodes())
        if (child.metaInfo().isQtQuickPauseAnimation())
            min = child.variantProperty("duration").value().toDouble();

    max = m_animation.variantProperty("duration").value().toDouble() + min;

    const qreal sceneMin = m_barItem->mapFromFrameToScene(min);

    QRectF barRect(sceneMin,
                   0,
                   (max - min) * m_barItem->rulerScaling(),
                   TimelineConstants::sectionHeight - 1);

    m_barItem->setRect(barRect);
}

AbstractView *TransitionEditorPropertyItem::view() const
{
    return m_animation.view();
}

ModelNode TransitionEditorPropertyItem::propertyAnimation() const
{
    return m_animation;
}

ModelNode TransitionEditorPropertyItem::pauseAnimation() const
{
    QTC_ASSERT(m_animation.isValid(), return {});
    QTC_ASSERT(m_animation.hasParentProperty(), return {});

    const ModelNode parent = m_animation.parentProperty().parentModelNode();

    for (const ModelNode &child : parent.directSubModelNodes())
        if (child.metaInfo().isQtQuickPauseAnimation())
            return child;

    return {};
}

void TransitionEditorPropertyItem::select()
{
    transitionEditorGraphicsScene()->setSelectedPropertyItem(this);
    m_barItem->update();
}

} // namespace QmlDesigner
