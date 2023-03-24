// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAction>
#include <QCoreApplication>
#include <QKeyEvent>

namespace Core {
class Command;
class Context;
}

namespace Terminal {

struct WidgetActions
{
    QAction copy{QCoreApplication::translate("QtC::Terminal", "Copy")};
    QAction paste{QCoreApplication::translate("QtC::Terminal", "Paste")};
    QAction clearSelection{QCoreApplication::translate("QtC::Terminal", "Clear Selection")};
    QAction clearTerminal{QCoreApplication::translate("QtC::Terminal", "Clear Terminal")};
    QAction moveCursorWordLeft{QCoreApplication::translate("QtC::Terminal",
                                                           "Move Cursor Word Left")};
    QAction moveCursorWordRight{QCoreApplication::translate("QtC::Terminal",
                                                            "Move Cursor Word Right")};
};

struct PaneActions
{
    QAction newTerminal{QCoreApplication::translate("QtC::Terminal", "New Terminal")};
    QAction closeTerminal{QCoreApplication::translate("QtC::Terminal", "Close Terminal")};
    QAction nextTerminal{QCoreApplication::translate("QtC::Terminal", "Next Terminal")};
    QAction prevTerminal{QCoreApplication::translate("QtC::Terminal", "Previous Terminal")};
    QAction minMax{QCoreApplication::translate("QtC::Terminal", "Minimize/Maximize Terminal")};
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
