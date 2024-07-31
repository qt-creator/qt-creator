// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/ioutputpane.h>

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

namespace Internal {

class ShowOutputTaskHandler;
class TabWidget;

enum class AppOutputPaneMode { FlashOnOutput, PopupOnOutput, PopupOnFirstOutput };

class AppOutputSettings
{
public:
    AppOutputPaneMode runOutputMode = AppOutputPaneMode::PopupOnFirstOutput;
    AppOutputPaneMode debugOutputMode = AppOutputPaneMode::FlashOnOutput;
    bool cleanOldOutput = false;
    bool mergeChannels = false;
    bool wrapOutput = false;
    int maxCharCount = Core::Constants::DEFAULT_MAX_CHAR_COUNT;
};

class AppOutputPane final : public Core::IOutputPane
{
public:
    AppOutputPane();
    ~AppOutputPane() final;

    bool aboutToClose() const;

    QList<RunControl *> allRunControls() const;

    const AppOutputSettings &settings() const { return m_settings; }
    void setSettings(const AppOutputSettings &settings);

    void prepareRunControlStart(RunControl *runControl);
    void showOutputPaneForRunControl(RunControl *runControl);

    void closeTabsWithoutPrompt();

private:
    enum CloseTabMode {
        CloseTabNoPrompt,
        CloseTabWithPrompt
    };

    void closeTabs(CloseTabMode mode);
    void showTabFor(RunControl *rc);

    void setBehaviorOnOutput(RunControl *rc, AppOutputPaneMode mode);
    void projectRemoved();

    void createNewOutputWindow(RunControl *rc);
    void appendMessage(ProjectExplorer::RunControl *rc, const QString &out,
                       Utils::OutputFormat format);
    void reRunRunControl();
    void stopRunControl();
    void attachToRunControl();
    void tabChanged(int);
    void contextMenuRequested(const QPoint &pos);
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

    QWidget *outputWidget(QWidget *) final;
    QList<QWidget *> toolBarWidgets() const final;
    void clearContents() final;
    bool canFocus() const final;
    bool hasFocus() const final;
    void setFocus() final;

    bool canNext() const final;
    bool canPrevious() const final;
    void goToNext() final;
    void goToPrev() final;
    bool canNavigate() const final;

    bool hasFilterContext() const final;

    void updateFilter() final;
    const QList<Core::OutputWindow *> outputWindows() const final;
    void ensureWindowVisible(Core::OutputWindow *ow) final;

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

AppOutputPane &appOutputPane();

void setupAppOutputPane();
void destroyAppOutputPane();

} // namespace Internal
} // namespace ProjectExplorer
