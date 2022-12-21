// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractaction.h"

#include <utils/icon.h>

namespace QmlDesigner {

AbstractAction::AbstractAction(const QString &description)
    : m_defaultAction(new DefaultAction(description))
{
    const Utils::Icon defaultIcon({
            {":/utils/images/select.png", Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    action()->setIcon(defaultIcon.icon());
}

AbstractAction::AbstractAction(DefaultAction *action)
    : m_defaultAction(action)
{
}

QAction *AbstractAction::action() const
{
    return m_defaultAction.data();
}

void AbstractAction::currentContextChanged(const SelectionContext &selectionContext)
{
    m_selectionContext = selectionContext;
    updateContext();
}

void AbstractAction::updateContext()
{
    m_defaultAction->setSelectionContext(m_selectionContext);
    if (m_selectionContext.isValid()) {
        m_defaultAction->setEnabled(isEnabled(m_selectionContext));
        m_defaultAction->setVisible(isVisible(m_selectionContext));
        if (m_defaultAction->isCheckable())
            m_defaultAction->setChecked(isChecked(m_selectionContext));
    }
}

bool AbstractAction::isChecked(const SelectionContext &) const
{
    return false;
}

void AbstractAction::setCheckable(bool checkable)
{
    m_defaultAction->setCheckable(checkable);
}

DefaultAction *AbstractAction::defaultAction() const
{
    return m_defaultAction.data();
}

SelectionContext AbstractAction::selectionContext() const
{
    return m_selectionContext;
}

DefaultAction::DefaultAction(const QString &description)
    : QAction(description, nullptr)
{
    connect(this, &QAction::triggered, this, &DefaultAction::actionTriggered);
}

void DefaultAction::setSelectionContext(const SelectionContext &selectionContext)
{
    m_selectionContext = selectionContext;
}

} // namespace QmlDesigner
