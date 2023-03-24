// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "terminalcommands.h"
#include "terminaltr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/find/textfindconstants.h>
#include <coreplugin/locator/locatorconstants.h>

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
    m_context = context;
    initWidgetActions();
    initPaneActions();
    initGlobalCommands();
}

void TerminalCommands::registerAction(QAction &action,
                                      const Utils::Id &id,
                                      QList<QKeySequence> shortCuts)
{
    Command *cmd = ActionManager::instance()->registerAction(&action, id, m_context);
    cmd->setKeySequences(shortCuts);
    m_commands.push_back(cmd);
}

void TerminalCommands::initWidgetActions()
{
    m_widgetActions.copy.setText(Tr::tr("Copy"));
    m_widgetActions.paste.setText(Tr::tr("Paste"));
    m_widgetActions.clearSelection.setText(Tr::tr("Clear Selection"));
    m_widgetActions.clearTerminal.setText(Tr::tr("Clear Terminal"));
    m_widgetActions.moveCursorWordLeft.setText(Tr::tr("Move Cursor Word Left"));
    m_widgetActions.moveCursorWordRight.setText(Tr::tr("Move Cursor Word Right"));
    m_widgetActions.findNext.setText(Tr::tr("Find Next"));
    m_widgetActions.findPrevious.setText(Tr::tr("Find Previous"));

    registerAction(m_widgetActions.copy,
                   COPY,
                   {QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+C")
                                                         : QLatin1String("Ctrl+Shift+C"))});

    registerAction(m_widgetActions.paste,
                   PASTE,
                   {QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+V")
                                                         : QLatin1String("Ctrl+Shift+V"))});

    registerAction(m_widgetActions.clearSelection, CLEARSELECTION);

    registerAction(m_widgetActions.moveCursorWordLeft,
                   MOVECURSORWORDLEFT,
                   {QKeySequence("Alt+Left")});

    registerAction(m_widgetActions.moveCursorWordRight,
                   MOVECURSORWORDRIGHT,
                   {QKeySequence("Alt+Right")});
}

void TerminalCommands::initPaneActions()
{
    m_paneActions.newTerminal.setText(Tr::tr("New Terminal"));
    m_paneActions.closeTerminal.setText(Tr::tr("Close Terminal"));
    m_paneActions.nextTerminal.setText(Tr::tr("Next Terminal"));
    m_paneActions.prevTerminal.setText(Tr::tr("Previous Terminal"));
    m_paneActions.minMax.setText(Tr::tr("Minimize/Maximize Terminal"));

    registerAction(m_paneActions.newTerminal,
                   NEWTERMINAL,
                   {QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+T")
                                                         : QLatin1String("Ctrl+Shift+T"))});

    registerAction(m_paneActions.closeTerminal,
                   CLOSETERMINAL,
                   {QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+W")
                                                         : QLatin1String("Ctrl+Shift+W"))});

    registerAction(m_paneActions.nextTerminal,
                   NEXTTERMINAL,
                   {QKeySequence("ALT+TAB"),
                    QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Shift+[")
                                                         : QLatin1String("Ctrl+PgUp"))});

    registerAction(m_paneActions.prevTerminal,
                   PREVTERMINAL,
                   {QKeySequence("ALT+SHIFT+TAB"),
                    QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Shift+]")
                                                         : QLatin1String("Ctrl+PgDown"))});

    registerAction(m_paneActions.minMax,
                   MINMAX,
                   {QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+Return")
                                                         : QLatin1String("Alt+Return"))});
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

void TerminalCommands::lazyInitCommand(const Utils::Id &id)
{
    Command *cmd = ActionManager::command(id);
    QTC_ASSERT(cmd, return);
    m_commands.append(cmd);
}

void TerminalCommands::lazyInitCommands()
{
    static const Utils::Id terminalPaneCmd("QtCreator.Pane.Terminal");
    lazyInitCommand(terminalPaneCmd);
    lazyInitCommand(Core::Constants::FIND_IN_DOCUMENT);
    lazyInitCommand(Core::Constants::FIND_NEXT);
    lazyInitCommand(Core::Constants::FIND_PREVIOUS);
    lazyInitCommand(Core::Constants::LOCATE);
}

} // namespace Terminal
