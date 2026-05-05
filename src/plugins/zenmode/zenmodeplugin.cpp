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

#include <utils/theme/theme.h>

#include <QAction>
#include <QFont>
#include <QMenuBar>

using namespace Core;

namespace ZenMode::Internal {

static Q_LOGGING_CATEGORY(zenModeLog, "qtc.plugin.zenmode", QtWarningMsg);

ZenModePlugin::~ZenModePlugin() = default;

void ZenModePlugin::initialize()
{
    ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(Tr::tr("Zen Mode"));
    ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    m_toggleDistractionAction = ActionBuilder(this, Constants::DISTRACTION_FREE_ACTION_ID)
        .addToContainer(Constants::MENU_ID)
        .setText(Tr::tr("Toggle Distraction Free Mode"))
        .setDefaultKeySequence(Tr::tr("Shift+Escape"))
        .addOnTriggered(this, &ZenModePlugin::toggleDistractionFreeMode)
        .contextAction();

    m_toggleZenModeAction = ActionBuilder(this, Constants::ZEN_MODE_ACTION_ID)
        .addToContainer(Constants::MENU_ID)
        .setText(Tr::tr("Toggle Zen Mode"))
        .setDefaultKeySequence(Tr::tr("Shift+Alt+Z"))
        .addOnTriggered(this, &ZenModePlugin::toggleZenMode)
        .contextAction();

    connect(ICore::instance(), &ICore::coreAboutToClose, [this] {
        if (m_zenModeActive)
            toggleZenMode();
        else if (m_distractionFreeModeActive)
            toggleDistractionFreeMode();
    });
}

void ZenModePlugin::extensionsInitialized()
{
    Core::IOptionsPage::registerCategory(
        "ZY.ZenMode", Tr::tr("Zen Mode"), ":/zenmode/images/settingscategory_zenmode.png");

    QFont statusFont = m_zenModeStatusBarIcon.font();
    statusFont.setPixelSize(14);
    statusFont.setBold(true);

    m_zenModeStatusBarIcon.setObjectName("zenModeState");
    m_zenModeStatusBarIcon.setText("Z");
    m_zenModeStatusBarIcon.setToolTip(Tr::tr("Zen Mode state"));
    m_zenModeStatusBarIcon.setFont(statusFont);

    m_distractionModeStatusBarIcon.setObjectName("distractionModeState");
    m_distractionModeStatusBarIcon.setText("D");
    m_distractionModeStatusBarIcon.setToolTip(Tr::tr("Distraction Free Mode state"));
    m_distractionModeStatusBarIcon.setFont(statusFont);
    updateStateIcons();

    Core::StatusBarManager::addStatusBarWidget(&m_zenModeStatusBarIcon,
                                               Core::StatusBarManager::RightCorner);
    Core::StatusBarManager::addStatusBarWidget(&m_distractionModeStatusBarIcon,
                                               Core::StatusBarManager::RightCorner);
}

bool ZenModePlugin::delayedInitialize()
{
    getActions();

    m_window = ICore::mainWindow();

    QMenuBar *menuBar = ActionManager::actionContainer(Core::Constants::MENU_BAR)->menuBar();
    m_menuBar = !menuBar->isNativeMenuBar() ? menuBar : nullptr;

    if (m_menuBar)
        m_menuBar->setVisible(true);
    setFullScreenMode(false);
    return true;
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

    m_outputPaneAction = getCommandFun(Core::Constants::OUTPUTPANE_CLOSE);
    m_toggleLeftSidebarAction = getCommandFun(Core::Constants::TOGGLE_LEFT_SIDEBAR);
    m_toggleRightSidebarAction = getCommandFun(Core::Constants::TOGGLE_RIGHT_SIDEBAR);
    m_toggleFullscreenAction = getCommandFun(Core::Constants::TOGGLE_FULLSCREEN);
}

void ZenModePlugin::hideOutputPanes()
{
    if (m_outputPaneAction)
        m_outputPaneAction->trigger();
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
        m_toggleLeftSidebarAction->trigger();
    }
    if (m_toggleRightSidebarAction &&
        !m_toggleRightSidebarAction->isChecked() &&
        m_prevRightSidebarState) {
        m_toggleRightSidebarAction->trigger();
    }
}

void ZenModePlugin::hideModeSidebar()
{
    m_prevModesSidebarState = ModeManager::modeStyle();
    const int styleSetting = settings().modes.itemValue().toInt();
    QTC_ASSERT(
        styleSetting >= int(ModeManager::Style::IconsAndText)
            && int(m_prevModesSidebarState <= ModeManager::Style::Hidden),
        return);
    ModeManager::setModeStyle(static_cast<ModeManager::Style>(styleSetting));
}

void ZenModePlugin::restoreModeSidebar()
{
    ModeManager::setModeStyle(m_prevModesSidebarState);
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
    const auto setIconColor = [](QLabel *label, bool active) {
        QPalette pal = label->palette();
        pal.setColor(
            QPalette::WindowText,
            active ? Utils::creatorColor(Utils::Theme::Token_Notification_Success_Default)
                   : Utils::creatorColor(Utils::Theme::TextColorDisabled));
        label->setPalette(pal);
        label->setEnabled(active);
    };
    setIconColor(&m_zenModeStatusBarIcon, m_zenModeActive);
    setIconColor(&m_distractionModeStatusBarIcon, m_distractionFreeModeActive);
}

void ZenModePlugin::updateContentEditorWidth()
{
    if (m_zenModeActive || m_distractionFreeModeActive) {
        m_prevEditorContentWidth = TextEditor::marginSettings().centerEditorContentWidthPercent();
        TextEditor::marginSettings().centerEditorContentWidthPercent.setValue(
            settings().contentWidth.value());
    } else {
        TextEditor::marginSettings().centerEditorContentWidthPercent.setValue(
            m_prevEditorContentWidth);
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
    if (m_menuBar)
        m_menuBar->setVisible(!m_zenModeActive);
    m_toggleDistractionAction->setEnabled(!m_zenModeActive);
}

void ZenModePlugin::toggleDistractionFreeMode()
{
    m_zenModeActive = false;
    m_distractionFreeModeActive = !m_distractionFreeModeActive;
    updateStateIcons();
    updateContentEditorWidth();

    setSidebarsAndModesVisible(!m_distractionFreeModeActive);
    m_toggleZenModeAction->setEnabled(!m_distractionFreeModeActive);
}
} // namespace ZenMode::Internal
