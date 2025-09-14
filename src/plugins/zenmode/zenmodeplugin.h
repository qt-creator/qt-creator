// Copyright (C) 2026 Sebastian Mosiej (sebastian.mosiej@wp.pl)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>

QT_BEGIN_NAMESPACE
class QMainWindow;
QT_END_NAMESPACE

namespace ZenMode::Internal {

class ZenModeSettings;

class ZenModePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ZenModePlugin.json")

public:
    ZenModePlugin() = default;
    ~ZenModePlugin() final;

    void initialize() final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;
    bool delayedInitialize() override;

public:
    enum ModeStyle {
        Hidden = 0,
        IconsOnly = 1,
        IconsAndText = 2
    };
    Q_ENUM(ModeStyle);

protected slots:
    void settingsChanged();

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
    ModeStyle activeModeSidebar();
    void updateStateIcons();
    void updateContentEditorWidth();
    void readUserSettings();
    void restoreUserSettings();

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

    std::vector<QPointer<QAction>> m_toggleModesStatesActions;
    ModeStyle m_prevModesSidebarState = Hidden;
    ModeStyle m_modesStateWhenZenActive = Hidden;

    QPointer<QAction> m_toggleFullscreenAction;
    QPointer<QMainWindow> m_window;
    QPointer<QMenuBar> m_menuBar;


    QLabel m_zenModeStatusBarIcon;
    QLabel m_distractionModeStatusBarIcon;
    QString m_activeModeStyleSheet;
    QString m_inactiveModeStyleSheet;

    struct UserSettings {
        bool fullScreenState = false;
        bool leftSidebarState = false;
        bool rightSidebarState = false;
        bool menuBarState = false;
        unsigned int modesSidebarState = 0;
        unsigned int editorContentWidth = 100;
    } m_userSettings;
};

} // namespace ZenMode::Internal
