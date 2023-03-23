// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "terminalcommands.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>

#include <utils/hostosinfo.h>

//#include <coreplugin/context.h>

using namespace Core;
using namespace Utils;

namespace Terminal {

constexpr char COPY[] = "Terminal.Copy";
constexpr char PASTE[] = "Terminal.Paste";
constexpr char CLEARSELECTION[] = "Terminal.ClearSelection";
constexpr char MOVECURSORWORDLEFT[] = "Terminal.MoveCursorWordLeft";
constexpr char MOVECURSORWORDRIGHT[] = "Terminal.MoveCursorWordRight";

constexpr char NEWTERMINAL[] = "Terminal.NewTerminal";
constexpr char CLOSETERMINAL[] = "Terminal.CloseTerminal";
constexpr char NEXTTERMINAL[] = "Terminal.NextTerminal";
constexpr char PREVTERMINAL[] = "Terminal.PrevTerminal";
constexpr char MINMAX[] = "Terminal.MinMax";

TerminalCommands &TerminalCommands::instance()
{
    static TerminalCommands instance;
    return instance;
}

TerminalCommands::TerminalCommands() {}

void TerminalCommands::init(const Core::Context &context)
{
    initWidgetActions(context);
    initPaneActions(context);
    initGlobalCommands();
}

void TerminalCommands::initWidgetActions(const Core::Context &context)
{
    Command *command = ActionManager::instance()->registerAction(&m_widgetActions.copy,
                                                                 COPY,
                                                                 context);
    command->setDefaultKeySequences(
        {QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+C")
                                              : QLatin1String("Ctrl+Shift+C")),
         QKeySequence(Qt::Key_Return)});
    m_commands.push_back(command);

    command = ActionManager::instance()->registerAction(&m_widgetActions.paste, PASTE, context);
    command->setDefaultKeySequence(QKeySequence(
        HostOsInfo::isMacHost() ? QLatin1String("Ctrl+V") : QLatin1String("Ctrl+Shift+V")));
    m_commands.push_back(command);

    command = ActionManager::instance()->registerAction(&m_widgetActions.clearSelection,
                                                        CLEARSELECTION);
    command->setDefaultKeySequence(QKeySequence("Esc"));
    m_commands.push_back(command);

    command = ActionManager::instance()->registerAction(&m_widgetActions.moveCursorWordLeft,
                                                        MOVECURSORWORDLEFT);
    command->setDefaultKeySequence(QKeySequence("Alt+Left"));
    m_commands.push_back(command);

    command = ActionManager::instance()->registerAction(&m_widgetActions.moveCursorWordRight,
                                                        MOVECURSORWORDRIGHT);
    command->setDefaultKeySequence(QKeySequence("Alt+Right"));
    m_commands.push_back(command);
}

void TerminalCommands::initPaneActions(const Core::Context &context)
{
    Command *command = ActionManager::instance()->registerAction(&m_paneActions.newTerminal,
                                                                 NEWTERMINAL,
                                                                 context);
    command->setDefaultKeySequence(QKeySequence(
        HostOsInfo::isMacHost() ? QLatin1String("Ctrl+T") : QLatin1String("Ctrl+Shift+T")));
    m_commands.push_back(command);

    command = ActionManager::instance()->registerAction(&m_paneActions.closeTerminal,
                                                        CLOSETERMINAL,
                                                        context);
    command->setDefaultKeySequence(QKeySequence(
        HostOsInfo::isMacHost() ? QLatin1String("Ctrl+W") : QLatin1String("Ctrl+Shift+W")));
    m_commands.push_back(command);

    command = ActionManager::instance()->registerAction(&m_paneActions.nextTerminal,
                                                        NEXTTERMINAL,
                                                        context);
    command->setDefaultKeySequences(
        {QKeySequence("ALT+TAB"),
         QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Shift+[")
                                              : QLatin1String("Ctrl+PgUp"))});
    m_commands.push_back(command);

    command = ActionManager::instance()->registerAction(&m_paneActions.prevTerminal,
                                                        PREVTERMINAL,
                                                        context);
    command->setDefaultKeySequences(
        {QKeySequence("ALT+SHIFT+TAB"),
         QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Shift+]")
                                              : QLatin1String("Ctrl+PgDown"))});
    m_commands.push_back(command);

    command = ActionManager::instance()->registerAction(&m_paneActions.minMax, MINMAX, context);
    command->setDefaultKeySequence(QKeySequence(
        HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Return") : QLatin1String("Alt+Return")));
    m_commands.push_back(command);
}

void TerminalCommands::initGlobalCommands()
{
    // Global commands we still want to allow
    m_commands.push_back(ActionManager::command(Constants::ZOOM_IN));
    m_commands.push_back(ActionManager::command(Constants::ZOOM_OUT));
    m_commands.push_back(ActionManager::command(Constants::EXIT));
    m_commands.push_back(ActionManager::command(Constants::OPTIONS));
}

bool TerminalCommands::triggerAction(QKeyEvent *event)
{
    QKeyCombination combination = event->keyCombination();

    // On macOS, the arrow keys include the KeypadModifier, which we don't want.
    if (HostOsInfo::isMacHost() && combination.keyboardModifiers() & Qt::KeypadModifier)
        combination = QKeyCombination(combination.keyboardModifiers() & ~Qt::KeypadModifier,
                                      combination.key());

    for (const auto &command : TerminalCommands::instance().m_commands) {
        if (!command->action()->isEnabled())
            continue;

        for (const auto &shortcut : command->keySequences()) {
            const auto result = shortcut.matches(QKeySequence(combination));
            if (result == QKeySequence::ExactMatch) {
                command->action()->trigger();
                return true;
            }
        }
    }

    return false;
}

QAction *TerminalCommands::openSettingsAction()
{
    return ActionManager::command("Preferences.Terminal.General")->action();
}

} // namespace Terminal
