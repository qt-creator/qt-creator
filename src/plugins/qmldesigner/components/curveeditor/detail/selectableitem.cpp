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

CurveEditorItem::CurveEditorItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_locked(false)
    , m_pinned(false)
    , m_underMouse(false)
{
    setAcceptHoverEvents(true);
}

void CurveEditorItem::lockedCallback() {}
void CurveEditorItem::pinnedCallback() {}
void CurveEditorItem::underMouseCallback() {}

bool CurveEditorItem::locked() const
{
    return m_locked;
}

bool CurveEditorItem::pinned() const
{
    return m_pinned;
}

bool CurveEditorItem::isUnderMouse() const
{
    return m_underMouse;
}

void CurveEditorItem::setLocked(bool locked)
{
    m_locked = locked;
    lockedCallback();
    update();
}

void CurveEditorItem::setPinned(bool pinned)
{
    m_pinned = pinned;
    pinnedCallback();
    update();
}

void CurveEditorItem::setIsUnderMouse(bool under)
{
    if (under != m_underMouse) {
        m_underMouse = under;
        underMouseCallback();
        update();
    }
}

SelectableItem::SelectableItem(QGraphicsItem *parent)
    : CurveEditorItem(parent)
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

void SelectableItem::lockedCallback()
{
    m_preSelected = SelectionMode::Undefined;
    m_selected = false;
    m_active = false;
    selectionCallback();
}

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
    if (locked())
        return;

    m_active = active;
}

void SelectableItem::setSelected(bool selected)
{
    if (locked())
        return;

    m_selected = selected;
}

void SelectableItem::setPreselected(SelectionMode mode)
{
    if (locked())
        return;

    m_preSelected = mode;
    selectionCallback();
}

void SelectableItem::applyPreselection()
{
    m_selected = selected();
    m_preSelected = SelectionMode::Undefined;
}

void SelectableItem::activationCallback() {}

void SelectableItem::selectionCallback() {}

void SelectableItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (locked())
        return;

    m_active = true;
    QGraphicsObject::mousePressEvent(event);
    activationCallback();
}

void SelectableItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (locked())
        return;

    if (type() == KeyframeItem::Type && !selected())
        return;

    QGraphicsObject::mouseMoveEvent(event);
}

void SelectableItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (locked())
        return;

    m_active = false;
    QGraphicsObject::mouseReleaseEvent(event);
    activationCallback();
}

} // End namespace DesignTools.
