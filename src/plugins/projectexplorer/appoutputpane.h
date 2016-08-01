/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QVector>

#include <coreplugin/ioutputpane.h>

#include <utils/outputformat.h>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QToolButton;
class QAction;
class QPoint;
QT_END_NAMESPACE

namespace Core { class OutputWindow; }

namespace ProjectExplorer {

class RunControl;
class Project;

namespace Internal {

class TabWidget;
class AppOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    enum CloseTabMode {
        CloseTabNoPrompt,
        CloseTabWithPrompt
    };

    enum BehaviorOnOutput {
        Flash,
        Popup
    };

    AppOutputPane();
    ~AppOutputPane() override;

    QWidget *outputWidget(QWidget *) override;
    QList<QWidget *> toolBarWidgets() const override;
    QString displayName() const override;
    int priorityInStatusBar() const override;
    void clearContents() override;
    void visibilityChanged(bool) override;
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
    void setBehaviorOnOutput(RunControl *rc, BehaviorOnOutput mode);

    bool aboutToClose() const;
    bool closeTabs(CloseTabMode mode);

    QList<RunControl *> allRunControls() const;

signals:
     void allRunControlsFinished();
     void runControlStarted(ProjectExplorer::RunControl *rc);
     void runControlFinished(ProjectExplorer::RunControl *rc);

public:
    // ApplicationOutput specifics
    void projectRemoved();

    void appendMessage(ProjectExplorer::RunControl *rc, const QString &out,
                       Utils::OutputFormat format);

private:
    void reRunRunControl();
    void stopRunControl();
    void attachToRunControl();
    void tabChanged(int);
    void contextMenuRequested(const QPoint &pos, int index);
    void slotRunControlStarted();
    void slotRunControlFinished();
    void slotRunControlFinished2(ProjectExplorer::RunControl *sender);

    void aboutToUnloadSession();
    void updateFromSettings();
    void enableDefaultButtons();

    void zoomIn();
    void zoomOut();

    void enableButtons(const RunControl *rc, bool isRunning);

    class RunControlTab {
    public:
        explicit RunControlTab(RunControl *runControl = nullptr,
                               Core::OutputWindow *window = nullptr);
        RunControl *runControl;
        Core::OutputWindow *window;
        // Is the run control stopping asynchronously, close the tab once it finishes
        bool asyncClosing = false;
        BehaviorOnOutput behaviorOnOutput = Flash;
    };

    bool isRunning() const;
    bool closeTab(int index, CloseTabMode cm = CloseTabWithPrompt);
    bool optionallyPromptToStop(RunControl *runControl);

    int indexOf(const RunControl *) const;
    int indexOf(const QWidget *outputWindow) const;
    int currentIndex() const;
    RunControl *currentRunControl() const;
    int tabWidgetIndexOf(int runControlIndex) const;
    void handleOldOutput(Core::OutputWindow *window) const;
    void updateCloseActions();
    void updateFontSettings();
    void saveSettings();
    void updateBehaviorSettings();

    QWidget *m_mainWidget;
    TabWidget *m_tabWidget;
    QVector<RunControlTab> m_runControlTabs;
    QAction *m_stopAction;
    QAction *m_closeCurrentTabAction;
    QAction *m_closeAllTabsAction;
    QAction *m_closeOtherTabsAction;
    QToolButton *m_reRunButton;
    QToolButton *m_stopButton;
    QToolButton *m_attachButton;
    QToolButton *m_zoomInButton;
    QToolButton *m_zoomOutButton;
    float m_zoom;
};

} // namespace Internal
} // namespace ProjectExplorer
