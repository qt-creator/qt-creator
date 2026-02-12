// Copyright (C) 2026 Sebastian Mosiej (sebastian.mosiej@wp.pl)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zenmodeplugin.h"
#include "zenmodepluginconstants.h"
#include "zenmodeplugintr.h"
#include "zenmodesettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/statusbarmanager.h>
#include <texteditor/marginsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QAction>
#include <QMenu>

using namespace Core;

namespace ZenMode::Internal {

const char OUTPUT_PANE_COMMAND_ID[] = "QtCreator.Pane.GeneralMessages";
const char MODE_HIDDEN_CMD_ID[] = "QtCreator.Modes.Hidden";
const char MODE_ICONS_CMD_ID[] = "QtCreator.Modes.IconsOnly";
const char MODE_ICONSTEXT_CMD_ID[] = "QtCreator.Modes.IconsAndText";

static Q_LOGGING_CATEGORY(zenModeLog, "qtc.plugin.zenmode", QtWarningMsg);

ZenModePlugin::~ZenModePlugin()
{
}

void ZenModePlugin::initialize()
{
    ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(Tr::tr("Zen Mode"));
    ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    m_toggleDistractionAction = ActionBuilder(this, Constants::DISTRACTION_FREE_ACTION_ID)
        .addToContainer(Constants::MENU_ID)
        .setText(Tr::tr("Toogle Distraction Free Mode"))
        .setDefaultKeySequence(Tr::tr("Shift+Escape"))
        .addOnTriggered(this, &ZenModePlugin::toggleDistractionFreeMode)
        .contextAction();

    m_toggleZenModeAction = ActionBuilder(this, Constants::ZEN_MODE_ACTION_ID)
        .addToContainer(Constants::MENU_ID)
        .setText(Tr::tr("Toogle Zen Mode"))
        .setDefaultKeySequence(Tr::tr("Shift+Alt+Z"))
        .addOnTriggered(this, &ZenModePlugin::toggleZenMode)
        .contextAction();
}

void ZenModePlugin::settingsChanged()
{ }

void ZenModePlugin::readUserSettings()
{
    m_userSettings.menuBarState = m_menuBar->isVisible();
    m_userSettings.fullScreenState = m_window->isFullScreen();
    m_userSettings.leftSidebarState =
            (m_toggleLeftSidebarAction && m_toggleLeftSidebarAction->isChecked());
    m_userSettings.rightSidebarState =
            (m_toggleRightSidebarAction && m_toggleRightSidebarAction->isChecked());

    m_userSettings.modesSidebarState = activeModeSidebar();

    m_userSettings.editorContentWidth =
            TextEditor::TextEditorSettings::marginSettings().m_centerEditorContentWidthPercent;
}

void ZenModePlugin::restoreUserSettings()
{
    m_menuBar->setVisible(m_userSettings.menuBarState);
    if (m_userSettings.fullScreenState)
        m_window->showFullScreen();
    if (m_toggleLeftSidebarAction
            && m_toggleLeftSidebarAction->isChecked() !=  m_userSettings.leftSidebarState)
        m_toggleLeftSidebarAction->trigger();
    if (m_toggleRightSidebarAction
            && m_toggleRightSidebarAction->isChecked() !=  m_userSettings.rightSidebarState)
        m_toggleRightSidebarAction->trigger();

    if (m_userSettings.modesSidebarState >= ModeStyle::Hidden
        && m_userSettings.modesSidebarState <= ModeStyle::IconsAndText) {
        auto action = m_toggleModesStatesActions[m_userSettings.modesSidebarState];
        if (action && !action->isChecked())
            action->trigger();
    }

    TextEditor::TextEditorSettings::setEditorContentWidth(m_userSettings.editorContentWidth);
}

void ZenModePlugin::extensionsInitialized()
{
    Core::IOptionsPage::registerCategory(
        "ZY.ZenMode", Tr::tr("Zen Mode"), ":/zenmode/images/settingscategory_zenmode.png");
    QObject::connect(&settings(), &Utils::AspectContainer::applied,
                     this, &ZenModePlugin::settingsChanged);

    auto userEditorContentWidth = [this] {
        if (!(m_zenModeActive || m_distractionFreeModeActive))
            m_userSettings.editorContentWidth =
                TextEditor::TextEditorSettings::marginSettings().m_centerEditorContentWidthPercent;
    };

    QObject::connect(TextEditor::TextEditorSettings::instance(),
                     &TextEditor::TextEditorSettings::marginSettingsChanged,
                     this, userEditorContentWidth);

    m_activeModeStyleSheet = "QLabel { color: #2ecc71; "
            "font-weight: bold; font-size: 14px;}";
    m_inactiveModeStyleSheet = "QLabel { color: #95a5a6; "
            "font-weight: normal; font-size: 14px; }";

    m_zenModeStatusBarIcon.setObjectName("zenModeState");
    m_zenModeStatusBarIcon.setText("Z");
    m_zenModeStatusBarIcon.setToolTip(Tr::tr("Zen Mode state"));
    m_zenModeStatusBarIcon.setTextFormat(Qt::RichText);

    m_distractionModeStatusBarIcon.setObjectName("distractionModeState");
    m_distractionModeStatusBarIcon.setText("D");
    m_distractionModeStatusBarIcon.setToolTip(Tr::tr("Distraction Free Mode state"));
    m_distractionModeStatusBarIcon.setTextFormat(Qt::RichText);
    updateStateIcons();

    Core::StatusBarManager::addStatusBarWidget(&m_zenModeStatusBarIcon,
                                               Core::StatusBarManager::RightCorner);
    Core::StatusBarManager::addStatusBarWidget(&m_distractionModeStatusBarIcon,
                                               Core::StatusBarManager::RightCorner);
    settingsChanged();
}

bool ZenModePlugin::delayedInitialize()
{
    getActions();

    m_window = ICore::mainWindow();
    m_menuBar = m_window->menuBar();

    readUserSettings();

    m_menuBar->setVisible(true);
    setFullScreenMode(false);
    return true;
}

ZenModePlugin::ShutdownFlag ZenModePlugin::aboutToShutdown()
{
    restoreUserSettings();
    return SynchronousShutdown;
}

void ZenModePlugin::getActions()
{
    auto getCommandFun = [](const Utils::Id &cmdId) -> QAction* {
        if (const Command *cmd = ActionManager::command(cmdId))
            return cmd->action();
        qCWarning(zenModeLog) << "Failed to get"
                              <<  cmdId.toString() << "action";
        return nullptr;
    };

    m_outputPaneAction = getCommandFun(OUTPUT_PANE_COMMAND_ID);
    m_toggleLeftSidebarAction = getCommandFun(Core::Constants::TOGGLE_LEFT_SIDEBAR);
    m_toggleRightSidebarAction = getCommandFun(Core::Constants::TOGGLE_RIGHT_SIDEBAR);
    m_toggleFullscreenAction = getCommandFun(Core::Constants::TOGGLE_FULLSCREEN);

    m_toggleModesStatesActions.resize(3);
    m_toggleModesStatesActions[0] = getCommandFun(MODE_HIDDEN_CMD_ID);
    m_toggleModesStatesActions[1] = getCommandFun(MODE_ICONS_CMD_ID);
    m_toggleModesStatesActions[2] = getCommandFun(MODE_ICONSTEXT_CMD_ID);
    m_prevModesSidebarState = activeModeSidebar();
}

void ZenModePlugin::hideOutputPanes()
{
    if (m_outputPaneAction) {
        m_outputPaneAction->trigger();
        m_outputPaneAction->trigger();
    }
}

void ZenModePlugin::hideSidebars()
{
    if (m_toggleLeftSidebarAction) {
       m_prevLeftSidebarState = m_toggleLeftSidebarAction->isChecked();
       if (m_prevLeftSidebarState)
           m_toggleLeftSidebarAction->trigger();
    }

    if (m_toggleRightSidebarAction) {
        m_prevRightSidebarState = m_toggleRightSidebarAction->isChecked();
        if (m_prevRightSidebarState)
            m_toggleRightSidebarAction->trigger();
    }
}

void ZenModePlugin::restoreSidebars()
{
    if (m_toggleLeftSidebarAction &&
        !m_toggleLeftSidebarAction->isChecked() &&
        m_prevLeftSidebarState) {
        m_prevLeftSidebarState = false;
        m_toggleLeftSidebarAction->trigger();
    }
    if (m_toggleRightSidebarAction &&
        !m_toggleRightSidebarAction->isChecked() &&
        m_prevRightSidebarState) {
        m_prevRightSidebarState = false;
        m_toggleRightSidebarAction->trigger();
    }
}

ZenModePlugin::ModeStyle ZenModePlugin::activeModeSidebar()
{
    for (size_t i = 0; i < m_toggleModesStatesActions.size(); ++i) {
        auto action = m_toggleModesStatesActions[i];
        if (action && action->isChecked())
            return static_cast<ModeStyle>(i);
    }
    return ModeStyle::Hidden;
}

void ZenModePlugin::hideModeSidebar()
{
    m_prevModesSidebarState = activeModeSidebar();
    auto action = m_toggleModesStatesActions[static_cast<ModeStyle>(settings().modes.value())];
    if (action && !action->isChecked())
        action->trigger();
}

void ZenModePlugin::restoreModeSidebar()
{
    if (m_prevModesSidebarState >= ModeStyle::Hidden
        && m_prevModesSidebarState <= ModeStyle::IconsAndText) {
        auto action = m_toggleModesStatesActions[m_prevModesSidebarState];
        if (action && !action->isChecked())
            action->trigger();
    }
}

void ZenModePlugin::setFullScreenMode(bool fullScreen)
{
    if (m_toggleFullscreenAction && fullScreen != m_window->isFullScreen())
        m_toggleFullscreenAction->trigger();
}

void ZenModePlugin::setSidebarsAndModesVisible(bool visible)
{
    if (visible) {
        restoreSidebars();
        restoreModeSidebar();
    } else {
        hideOutputPanes();
        hideSidebars();
        hideModeSidebar();
    }
}

void ZenModePlugin::updateStateIcons()
{
    m_zenModeStatusBarIcon.setStyleSheet(
        m_zenModeActive ? m_activeModeStyleSheet : m_inactiveModeStyleSheet);

    m_distractionModeStatusBarIcon.setStyleSheet(
        m_distractionFreeModeActive ? m_activeModeStyleSheet : m_inactiveModeStyleSheet);
}

void ZenModePlugin::updateContentEditorWidth()
{
    if (m_zenModeActive || m_distractionFreeModeActive) {
        TextEditor::TextEditorSettings::setEditorContentWidth(settings().contentWidth.value());
    } else {
        TextEditor::TextEditorSettings::setEditorContentWidth(m_userSettings.editorContentWidth);
    }
}

void ZenModePlugin::toggleZenMode()
{
    m_distractionFreeModeActive = false;
    m_zenModeActive = !m_zenModeActive;
    updateStateIcons();
    updateContentEditorWidth();

    setSidebarsAndModesVisible(!m_zenModeActive);
    setFullScreenMode(m_zenModeActive);
    m_menuBar->setVisible(!m_zenModeActive);
}

void ZenModePlugin::toggleDistractionFreeMode()
{
    m_zenModeActive = false;
    m_distractionFreeModeActive = !m_distractionFreeModeActive;
    updateStateIcons();
    updateContentEditorWidth();

    setSidebarsAndModesVisible(m_distractionFreeModeActive);
}
} // namespace ZenMode::Internal
