/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "selectableitem.h"
#include "keyframeitem.h"

namespace DesignTools {

SelectableItem::SelectableItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_active(false)
    , m_selected(false)
    , m_preSelected(SelectionMode::Undefined)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

SelectableItem::~SelectableItem() {}

bool SelectableItem::activated() const
{
    return m_active;
}

bool SelectableItem::selected() const
{
    switch (m_preSelected) {
    case SelectionMode::Clear:
        return false;
    case SelectionMode::New:
        return true;
    case SelectionMode::Add:
        return true;
    case SelectionMode::Remove:
        return false;
    case SelectionMode::Toggle:
        return !m_selected;
    default:
        return m_selected;
    }

    return false;
}

void SelectableItem::setActivated(bool active)
{
    m_active = active;
}

void SelectableItem::setPreselected(SelectionMode mode)
{
    m_preSelected = mode;
    selectionCallback();
}

void SelectableItem::applyPreselection()
{
    m_selected = selected();
    m_preSelected = SelectionMode::Undefined;
}

void SelectableItem::selectionCallback() {}

void SelectableItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_active = true;
    QGraphicsObject::mousePressEvent(event);
}

void SelectableItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (type() == KeyframeItem::Type && !selected())
        return;

    QGraphicsObject::mouseMoveEvent(event);
}

void SelectableItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_active = false;
    QGraphicsObject::mouseReleaseEvent(event);
}

} // End namespace DesignTools.
