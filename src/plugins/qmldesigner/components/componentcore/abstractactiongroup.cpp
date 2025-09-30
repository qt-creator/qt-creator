// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractactiongroup.h"
#include "componentcoretracing.h"
#include "qmleditormenu.h"

#include <QMenu>

namespace QmlDesigner {

using ComponentCoreTracing::category;
using NanotraceHR::keyValue;

AbstractActionGroup::AbstractActionGroup(const QString &displayName)
    : m_displayName(displayName)
    , m_menu(Utils::makeUniqueObjectPtr<QmlEditorMenu>())
{
    NanotraceHR::Tracer tracer{"abstract action group constructor",
                               ComponentCoreTracing::category(),
                               keyValue("display name", displayName)};

    m_menu->setTitle(displayName);
    m_action = m_menu->menuAction();

    QmlEditorMenu *qmlEditorMenu = qobject_cast<QmlEditorMenu *>(m_menu.get());
    if (qmlEditorMenu)
        qmlEditorMenu->setIconsVisible(false);
}

ActionInterface::Type AbstractActionGroup::type() const
{
    NanotraceHR::Tracer tracer{"abstract action group type", ComponentCoreTracing::category()};

    return ActionInterface::ContextMenu;
}

QAction *AbstractActionGroup::action() const
{
    NanotraceHR::Tracer tracer{"abstract action group action", ComponentCoreTracing::category()};

    return m_action;
}

QMenu *AbstractActionGroup::menu() const
{
    NanotraceHR::Tracer tracer{"abstract action group menu", ComponentCoreTracing::category()};

    return m_menu.get();
}

SelectionContext AbstractActionGroup::selectionContext() const
{
    NanotraceHR::Tracer tracer{"abstract action group selection context",
                               ComponentCoreTracing::category()};

    return m_selectionContext;
}

void AbstractActionGroup::currentContextChanged(const SelectionContext &selectionContext)
{
    NanotraceHR::Tracer tracer{"abstract action group current context changed",
                               ComponentCoreTracing::category()};

    m_selectionContext = selectionContext;
    updateContext();
}

void AbstractActionGroup::updateContext()
{
    NanotraceHR::Tracer tracer{"abstract action group update context",
                               ComponentCoreTracing::category()};

    if (m_selectionContext.isValid()) {
        m_action->setEnabled(isEnabled(m_selectionContext));
        m_action->setVisible(isVisible(m_selectionContext));
    }
}

} // namespace QmlDesigner
