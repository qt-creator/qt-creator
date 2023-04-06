// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <coreplugin/icontext.h>

#include <QAction>
#include <QCoreApplication>
#include <QKeyEvent>

namespace Core {
class Command;
class Context;
} // namespace Core

namespace Terminal {

struct WidgetActions
{
    QAction copy;
    QAction paste;
    QAction copyLink;
    QAction clearSelection;
    QAction clearTerminal;
    QAction moveCursorWordLeft;
    QAction moveCursorWordRight;
    QAction findNext;
    QAction findPrevious;
};

struct PaneActions
{
    QAction newTerminal;
    QAction closeTerminal;
    QAction nextTerminal;
    QAction prevTerminal;
    QAction minMax;
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

    void lazyInitCommands();

protected:
    void initWidgetActions();
    void initPaneActions();
    void initGlobalCommands();

    void lazyInitCommand(const Utils::Id &id);
    void registerAction(QAction &action, const Utils::Id &id, QList<QKeySequence> shortcuts = {});

private:
    WidgetActions m_widgetActions;
    PaneActions m_paneActions;
    QList<Core::Command *> m_commands;
    Core::Context m_context;
};

} // namespace Terminal
