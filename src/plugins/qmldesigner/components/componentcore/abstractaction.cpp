// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractaction.h"

#include <utils/icon.h>

namespace QmlDesigner {

AbstractAction::AbstractAction(const QString &description)
    : m_pureAction(new DefaultAction(description))
{
    const Utils::Icon defaultIcon({
            {":/utils/images/select.png", Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    action()->setIcon(defaultIcon.icon());
}

AbstractAction::AbstractAction(PureActionInterface *action)
    : m_pureAction(action)
{
}

QAction *AbstractAction::action() const
{
    return m_pureAction->action();
}

void AbstractAction::currentContextChanged(const SelectionContext &selectionContext)
{
    m_selectionContext = selectionContext;
    updateContext();
}

void AbstractAction::updateContext()
{
    m_pureAction->setSelectionContext(m_selectionContext);
    if (m_selectionContext.isValid()) {
        QAction *action = m_pureAction->action();
        action->setEnabled(isEnabled(m_selectionContext));
        action->setVisible(isVisible(m_selectionContext));
        if (action->isCheckable())
            action->setChecked(isChecked(m_selectionContext));
    }
}

bool AbstractAction::isChecked(const SelectionContext &) const
{
    return false;
}

void AbstractAction::setCheckable(bool checkable)
{
    action()->setCheckable(checkable);
}

PureActionInterface *AbstractAction::pureAction() const
{
    return m_pureAction.data();
}

SelectionContext AbstractAction::selectionContext() const
{
    return m_selectionContext;
}

DefaultAction::DefaultAction(const QString &description)
    : QAction(description, nullptr)
    , PureActionInterface(this)
{
    connect(this, &QAction::triggered, this, &DefaultAction::actionTriggered);
}

void DefaultAction::setSelectionContext(const SelectionContext &selectionContext)
{
    m_selectionContext = selectionContext;
}

PureActionInterface::PureActionInterface(QAction *action)
    : m_action(action)
{

}

QAction *PureActionInterface::action()
{
    return m_action;
}

} // namespace QmlDesigner
