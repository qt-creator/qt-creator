// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractaction.h"
#include "componentcoretracing.h"

#include <utils/icon.h>

namespace QmlDesigner {

using ComponentCoreTracing::category;
using NanotraceHR::keyValue;

AbstractAction::AbstractAction(const QString &description)
    : m_pureAction(std::make_unique<DefaultAction>(description))
{
    NanotraceHR::Tracer tracer{"abstract action constructor",
                               ComponentCoreTracing::category(),
                               keyValue("description", description)};

    const Utils::Icon defaultIcon({{":/utils/images/select.png",
                                    Utils::Theme::QmlDesigner_FormEditorForegroundColor}},
                                  Utils::Icon::MenuTintedStyle);

    action()->setIcon(defaultIcon.icon());
}

AbstractAction::AbstractAction(PureActionInterface *action)
    : m_pureAction(action)
{
    NanotraceHR::Tracer tracer{"abstract action constructor", ComponentCoreTracing::category()};
}

QAction *AbstractAction::action() const
{
    NanotraceHR::Tracer tracer{"abstract action action", ComponentCoreTracing::category()};

    return m_pureAction->action();
}

void AbstractAction::currentContextChanged(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"abstract action current context changed",
                               ComponentCoreTracing::category()};

    m_selectionContext = selectionContext;
    updateContext();
}

void AbstractAction::updateContext()
{
    NanotraceHR::Tracer tracer{"abstract action update context", ComponentCoreTracing::category()};

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
    NanotraceHR::Tracer tracer{"abstract action is checked", ComponentCoreTracing::category()};

    return false;
}

void AbstractAction::setCheckable(bool checkable)
{
    NanotraceHR::Tracer tracer{"abstract action set checkable",
                               ComponentCoreTracing::category(),
                               keyValue("checkable", checkable)};

    action()->setCheckable(checkable);
}

PureActionInterface *AbstractAction::pureAction() const
{
    NanotraceHR::Tracer tracer{"abstract action pure action", ComponentCoreTracing::category()};

    return m_pureAction.get();
}

SelectionContext AbstractAction::selectionContext() const
{
    NanotraceHR::Tracer tracer{"abstract action selection context", ComponentCoreTracing::category()};

    return m_selectionContext;
}

DefaultAction::DefaultAction(const QString &description)
    : QAction(description, nullptr)
    , PureActionInterface(this)
{
    NanotraceHR::Tracer tracer{"abstract action default action constructor",
                               ComponentCoreTracing::category(),
                               keyValue("description", description)};

    connect(this, &QAction::triggered, this, &DefaultAction::actionTriggered);
}

void DefaultAction::setSelectionContext(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"abstract action default action set selection context",
                               ComponentCoreTracing::category()};

    m_selectionContext = selectionContext;
}

PureActionInterface::PureActionInterface(QAction *action)
    : m_action(action)
{
    NanotraceHR::Tracer tracer{"abstract action pure action interface constructor",
                               ComponentCoreTracing::category()};
}

QAction *PureActionInterface::action()
{
    NanotraceHR::Tracer tracer{"abstract action pure action interface action",
                               ComponentCoreTracing::category()};

    return m_action;
}

} // namespace QmlDesigner
