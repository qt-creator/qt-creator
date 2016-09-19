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

#include "warningitem.h"
#include "graphicsscene.h"
#include "warningmodel.h"

#include <utils/utilsicons.h>

#include <QPainter>
#include <QToolTip>

using namespace ScxmlEditor::PluginInterface;

WarningItem::WarningItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_parentItem(qgraphicsitem_cast<BaseItem*>(parent))
{
    setPixmap(Utils::Icons::WARNING.pixmap());

    auto sc = static_cast<GraphicsScene*>(scene());
    if (sc) {
        sc->addWarningItem(this);
        m_warningModel = sc->warningModel();
        connect(m_warningModel.data(), &OutputPane::WarningModel::modelCleared, this, &WarningItem::check);
    }
    setWarningActive(false);
}

WarningItem::~WarningItem()
{
    auto sc = static_cast<GraphicsScene*>(scene());
    if (sc) {
        sc->removeWarningItem(this);
        delete m_warning;
        m_warning = nullptr;
    }
}

void WarningItem::check()
{
}

QRectF WarningItem::boundingRect() const
{
    return QRectF(QPointF(), m_pixmap.size() * m_pixmap.devicePixelRatio());
}

void WarningItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    QToolTip::showText(e->screenPos(), toolTip(), 0);
    QGraphicsObject::mousePressEvent(e);
}

QVariant WarningItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemSceneHasChanged: {
        auto sc = static_cast<GraphicsScene*>(scene());
        if (sc) {
            sc->addWarningItem(this);
            m_warningModel = sc->warningModel();
            connect(m_warningModel.data(), &OutputPane::WarningModel::modelCleared, this, &WarningItem::check);
        }
        break;
    }
    case QGraphicsItem::ItemVisibleHasChanged: {
        auto sc = static_cast<GraphicsScene*>(scene());
        if (sc)
            sc->warningVisibilityChanged(m_severity, this);
        break;
    }
    default:
        break;
    }

    return QGraphicsObject::itemChange(change, value);
}

void WarningItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->drawPixmap(0, 0, m_pixmap);
}

void WarningItem::setReason(const QString &reason)
{
    m_reason = reason;
    if (m_warning)
        m_warning->setReason(reason);

    setToolTip(m_reason);
}

void WarningItem::setDescription(const QString &description)
{
    m_description = description;
    if (m_warning)
        m_warning->setDescription(description);
}

void WarningItem::setSeverity(ScxmlEditor::OutputPane::Warning::Severity type)
{
    m_severity = type;
    if (m_warning)
        m_warning->setSeverity(type);
}

void WarningItem::setTypeName(const QString &name)
{
    m_typeName = name;
    if (m_warning)
        m_warning->setTypeName(name);
}

void WarningItem::setWarningActive(bool active)
{
    if (active && !m_warning && m_warningModel) {
        m_warning = m_warningModel->createWarning(m_severity, m_typeName, m_reason, m_description);
        connect(m_warning.data(), &OutputPane::Warning::dataChanged, this, &WarningItem::checkVisibility);
    } else if (!active && m_warning) {
        m_warning->deleteLater();
        m_warning = nullptr;
    }

    checkVisibility();
}

void WarningItem::checkVisibility()
{
    setVisible(m_warning && m_warning->isActive());
}

void WarningItem::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap.scaled(QSize(25, 25) * pixmap.devicePixelRatio());
}

ScxmlTag *WarningItem::tag() const
{
    return m_parentItem ? m_parentItem->tag() : 0;
}

ScxmlEditor::OutputPane::Warning *WarningItem::warning() const
{
    return m_warning;
}
