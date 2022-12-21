// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectableitem.h"
#include "keyframeitem.h"

namespace QmlDesigner {

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

} // End namespace QmlDesigner.
