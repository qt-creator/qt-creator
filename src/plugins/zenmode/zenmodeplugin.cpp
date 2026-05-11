// Copyright (C) 2026 Sebastian Mosiej (sebastian.mosiej@wp.pl)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zenmodepluginconstants.h"
#include "zenmodeplugintr.h"
#include "zenmodesettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/statusbarmanager.h>

#include <extensionsystem/iplugin.h>
#include <texteditor/marginsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QAction>
#include <QMenuBar>
#include <QToolButton>

using namespace Core;
using namespace Utils;

namespace ZenMode::Internal {

static Q_LOGGING_CATEGORY(zenModeLog, "qtc.plugin.zenmode", QtWarningMsg);

class ZenModePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ZenMode.json")

public:
    ZenModePlugin() = default;

    void initialize() final;
    void extensionsInitialized() final;
    bool delayedInitialize() final;

private:
    void getActions();

    void toggleDistractionFreeMode();
    void toggleZenMode();

    void hideOutputPanes();
    void hideSidebars();
    void restoreSidebars();
    void hideModeSidebar();
    void restoreModeSidebar();

    void setFullScreenMode(bool fullScreen);

    void setSidebarsAndModesVisible(bool visible);
    void updateStateIcons();
    void updateContentEditorWidth();

private:
    bool m_distractionFreeModeActive = false;
    bool m_zenModeActive = false;

    QAction *m_toggleDistractionAction = nullptr;
    QAction *m_toggleZenModeAction = nullptr;

    QPointer<QAction> m_outputPaneAction;
    QPointer<QAction> m_toggleLeftSidebarAction;
    QPointer<QAction> m_toggleRightSidebarAction;
    bool m_prevLeftSidebarState = false;
    bool m_prevRightSidebarState = false;
    unsigned int m_prevEditorContentWidth = 100;

    Core::ModeManager::Style m_prevModesSidebarState = Core::ModeManager::Style::Hidden;
    Core::ModeManager::Style m_modesStateWhenZenActive = Core::ModeManager::Style::Hidden;

    QPointer<QAction> m_toggleFullscreenAction;
    QPointer<QMainWindow> m_window;
    QPointer<QMenuBar> m_menuBar;

    QToolButton m_zenModeStatusBarIcon;
    QToolButton m_distractionModeStatusBarIcon;
};

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
    connect(ActionManager::command(Constants::DISTRACTION_FREE_ACTION_ID),
            &Command::keySequenceChanged,
            this, &ZenModePlugin::updateStateIcons);

    m_toggleZenModeAction = ActionBuilder(this, Constants::ZEN_MODE_ACTION_ID)
        .addToContainer(Constants::MENU_ID)
        .setText(Tr::tr("Toggle Zen Mode"))
        .setDefaultKeySequence(Tr::tr("Shift+Alt+Z"))
        .addOnTriggered(this, &ZenModePlugin::toggleZenMode)
        .contextAction();
    connect(ActionManager::command(Constants::ZEN_MODE_ACTION_ID),
            &Command::keySequenceChanged,
            this, &ZenModePlugin::updateStateIcons);

    connect(ICore::instance(), &ICore::saveSettingsRequested, [this](ICore::SaveSettingsReason reason) {
        if (reason != ICore::MainWindowClosing)
            return;
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

    const Icon zenIcon({{":/zenmode/images/zenmode.png", Theme::IconsBaseColor}});
    const Icon dfIcon({{":/zenmode/images/distractionfree.png", Theme::IconsBaseColor}});

    m_zenModeStatusBarIcon.setObjectName("zenModeState");
    m_zenModeStatusBarIcon.setIcon(zenIcon.icon());
    m_zenModeStatusBarIcon.setAutoRaise(true);
    m_zenModeStatusBarIcon.setCheckable(true);
    connect(&m_zenModeStatusBarIcon, &QToolButton::clicked,
            m_toggleZenModeAction, &QAction::trigger);

    m_distractionModeStatusBarIcon.setObjectName("distractionModeState");
    m_distractionModeStatusBarIcon.setIcon(dfIcon.icon());
    m_distractionModeStatusBarIcon.setAutoRaise(true);
    m_distractionModeStatusBarIcon.setCheckable(true);
    connect(&m_distractionModeStatusBarIcon, &QToolButton::clicked,
            m_toggleDistractionAction, &QAction::trigger);
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
    m_zenModeStatusBarIcon.setChecked(m_zenModeActive);
    m_distractionModeStatusBarIcon.setChecked(m_distractionFreeModeActive);

    const QString dfmMsg = m_distractionFreeModeActive
            ? Tr::tr("Distraction free mode is active.")
            : Tr::tr("Distraction free mode is inactive.");
    m_distractionModeStatusBarIcon.setToolTip(Core::ActionManager::command(
        Constants::DISTRACTION_FREE_ACTION_ID)->stringWithAppendedShortcut(dfmMsg));

    const QString zenMsg = m_zenModeActive
             ? Tr::tr("Zen mode is active.")
             : Tr::tr("Zen mode is inactive.");
    m_zenModeStatusBarIcon.setToolTip(Core::ActionManager::command(
        Constants::ZEN_MODE_ACTION_ID)->stringWithAppendedShortcut(zenMsg));
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
    m_distractionModeStatusBarIcon.setEnabled(!m_zenModeActive);
}

void ZenModePlugin::toggleDistractionFreeMode()
{
    m_zenModeActive = false;
    m_distractionFreeModeActive = !m_distractionFreeModeActive;
    updateStateIcons();
    updateContentEditorWidth();

    setSidebarsAndModesVisible(!m_distractionFreeModeActive);
    m_toggleZenModeAction->setEnabled(!m_distractionFreeModeActive);
    m_zenModeStatusBarIcon.setEnabled(!m_distractionFreeModeActive);
}

} // namespace ZenMode::Internal

#include "zenmodeplugin.moc"
