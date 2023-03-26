// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "showoutputtaskhandler.h"

#include "task.h"

#include <coreplugin/ioutputpane.h>
#include <coreplugin/outputwindow.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QAction>

namespace ProjectExplorer {
namespace Internal {

ShowOutputTaskHandler::ShowOutputTaskHandler(Core::IOutputPane *window, const QString &text,
                                             const QString &tooltip, const QString &shortcut)
    : m_window(window), m_text(text), m_tooltip(tooltip), m_shortcut(shortcut)
{
    QTC_CHECK(m_window);
    QTC_CHECK(!m_text.isEmpty());
}

bool ShowOutputTaskHandler::canHandle(const Task &task) const
{
    return Utils::anyOf(m_window->outputWindows(), [task](const Core::OutputWindow *ow) {
        return ow->knowsPositionOf(task.taskId);
    });
}

void ShowOutputTaskHandler::handle(const Task &task)
{
    Q_ASSERT(canHandle(task));
    // popup first as this does move the visible area!
    m_window->popup(Core::IOutputPane::Flags(Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus));
    for (Core::OutputWindow * const ow : m_window->outputWindows()) {
        if (ow->knowsPositionOf(task.taskId)) {
            m_window->ensureWindowVisible(ow);
            ow->showPositionOf(task.taskId);
            break;
        }
    }
}

QAction *ShowOutputTaskHandler::createAction(QObject *parent) const
{
    QAction * const outputAction = new QAction(m_text, parent);
    if (!m_tooltip.isEmpty())
        outputAction->setToolTip(m_tooltip);
    if (!m_shortcut.isEmpty())
        outputAction->setShortcut(QKeySequence(m_shortcut));
    outputAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    return outputAction;
}

} // namespace Internal
} // namespace ProjectExplorer
