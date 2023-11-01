// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorersettings.h"

#include <coreplugin/ioutputpane.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/outputformat.h>

#include <QPointer>
#include <QVector>

QT_BEGIN_NAMESPACE
class QToolButton;
class QAction;
class QPoint;
QT_END_NAMESPACE

namespace Core { class OutputWindow; }

namespace ProjectExplorer {

class RunControl;
class Project;

namespace Internal {

class ShowOutputTaskHandler;
class TabWidget;

class AppOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    enum CloseTabMode {
        CloseTabNoPrompt,
        CloseTabWithPrompt
    };

    AppOutputPane();
    ~AppOutputPane() override;

    QWidget *outputWidget(QWidget *) override;
    QList<QWidget *> toolBarWidgets() const override;
    void clearContents() override;
    bool canFocus() const override;
    bool hasFocus() const override;
    void setFocus() override;

    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;
    bool canNavigate() const override;

    void createNewOutputWindow(RunControl *rc);
    void showTabFor(RunControl *rc);
    void setBehaviorOnOutput(RunControl *rc, AppOutputPaneMode mode);

    bool aboutToClose() const;
    void closeTabs(CloseTabMode mode);

    QList<RunControl *> allRunControls() const;

    // ApplicationOutput specifics
    void projectRemoved();

    const AppOutputSettings &settings() const { return m_settings; }
    void setSettings(const AppOutputSettings &settings);

private:
    void appendMessage(ProjectExplorer::RunControl *rc, const QString &out,
                       Utils::OutputFormat format);
    void reRunRunControl();
    void stopRunControl();
    void attachToRunControl();
    void tabChanged(int);
    void contextMenuRequested(const QPoint &pos, int index);
    void runControlFinished(RunControl *runControl);

    void aboutToUnloadSession();
    void updateFromSettings();
    void enableDefaultButtons();

    void zoomIn(int range);
    void zoomOut(int range);
    void resetZoom();

    void enableButtons(const RunControl *rc);

    class RunControlTab {
    public:
        explicit RunControlTab(RunControl *runControl = nullptr,
                               Core::OutputWindow *window = nullptr);
        QPointer<RunControl> runControl;
        QPointer<Core::OutputWindow> window;
        AppOutputPaneMode behaviorOnOutput = AppOutputPaneMode::FlashOnOutput;
    };

    void closeTab(int index, CloseTabMode cm = CloseTabWithPrompt);
    bool optionallyPromptToStop(RunControl *runControl);

    RunControlTab *tabFor(const RunControl *rc);
    RunControlTab *tabFor(const QWidget *outputWindow);
    const RunControlTab *tabFor(const QWidget *outputWindow) const;
    RunControlTab *currentTab();
    const RunControlTab *currentTab() const;
    RunControl *currentRunControl() const;
    void handleOldOutput(Core::OutputWindow *window) const;
    void updateCloseActions();
    void updateFilter() override;
    const QList<Core::OutputWindow *> outputWindows() const override;
    void ensureWindowVisible(Core::OutputWindow *ow) override;

    void loadSettings();
    void storeSettings() const;

    TabWidget *m_tabWidget;
    QVector<RunControlTab> m_runControlTabs;
    int m_runControlCount = 0;
    QAction *m_stopAction;
    QAction *m_closeCurrentTabAction;
    QAction *m_closeAllTabsAction;
    QAction *m_closeOtherTabsAction;
    QToolButton *m_reRunButton;
    QToolButton *m_stopButton;
    QToolButton *m_attachButton;
    QToolButton * const m_settingsButton;
    QWidget *m_formatterWidget;
    ShowOutputTaskHandler * const m_handler;
    AppOutputSettings m_settings;
};

class AppOutputSettingsPage final : public Core::IOptionsPage
{
public:
    AppOutputSettingsPage();
};

} // namespace Internal
} // namespace ProjectExplorer
