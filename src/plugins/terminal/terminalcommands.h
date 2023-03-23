// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "terminaltr.h"

#include <QAction>
#include <QKeyEvent>

namespace Core {
class Command;
class Context;
}

namespace Terminal {

struct WidgetActions
{
    QAction copy{Tr::tr("Copy")};
    QAction paste{Tr::tr("Paste")};
    QAction clearSelection{Tr::tr("Clear Selection")};
    QAction clearTerminal{Tr::tr("Clear Terminal")};
};

struct PaneActions
{
    QAction newTerminal{Tr::tr("New Terminal")};
    QAction closeTerminal{Tr::tr("Close Terminal")};
    QAction nextTerminal{Tr::tr("Next Terminal")};
    QAction prevTerminal{Tr::tr("Previous Terminal")};
    QAction minMax{Tr::tr("Minimize/Maximize Terminal")};
};

class TerminalCommands
{
public:
    TerminalCommands();

    void init(const Core::Context &context);
    static TerminalCommands &instance();
    static WidgetActions &widgetActions() { return instance().m_widgetActions; }
    static PaneActions &paneActions() { return instance().m_paneActions; }

    static QList<QKeySequence> shortcutsFor(QAction *action);

    static bool triggerAction(QKeyEvent *event);

    static QAction *openSettingsAction();

protected:
    void initWidgetActions(const Core::Context &context);
    void initPaneActions(const Core::Context &context);
    void initGlobalCommands();

private:
    WidgetActions m_widgetActions;
    PaneActions m_paneActions;
    QList<Core::Command *> m_commands;
};

} // namespace Terminal
