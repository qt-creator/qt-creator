// Copyright (C) 2026 Sebastian Mosiej (sebastian.mosiej@wp.pl)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#include <coreplugin/modemanager.h>

#include <QLabel>

QT_BEGIN_NAMESPACE
class QAction;
class QMainWindow;
class QMenuBar;
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
    bool delayedInitialize() override;

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

    QLabel m_zenModeStatusBarIcon;
    QLabel m_distractionModeStatusBarIcon;
};

} // namespace ZenMode::Internal
