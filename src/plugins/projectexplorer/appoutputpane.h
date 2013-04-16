/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef APPOUTPUTPANE_H
#define APPOUTPUTPANE_H

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

class AppOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    enum CloseTabMode {
        CloseTabNoPrompt,
        CloseTabWithPrompt
    };

    enum BehavivorOnOutput {
        Flash,
        Popup
    };

    AppOutputPane();
    virtual ~AppOutputPane();

    QWidget *outputWidget(QWidget *);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool);
    bool canFocus() const;
    bool hasFocus() const;
    void setFocus();

    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();
    bool canNavigate() const;

    void createNewOutputWindow(RunControl *rc);
    void showTabFor(RunControl *rc);
    void setBehaviorOnOutput(RunControl *rc, BehavivorOnOutput mode);

    bool aboutToClose() const;
    bool closeTabs(CloseTabMode mode);

    QList<RunControl *> runControls() const;

signals:
     void allRunControlsFinished();
     void runControlStarted(ProjectExplorer::RunControl *rc);
     void runControlFinished(ProjectExplorer::RunControl *rc);

public slots:
    // ApplicationOutput specifics
    void projectRemoved();

    void appendMessage(ProjectExplorer::RunControl *rc, const QString &out,
                       Utils::OutputFormat format);

private slots:
    void reRunRunControl();
    void stopRunControl();
    void attachToRunControl();
    bool closeTab(int index);
    void tabChanged(int);
    void contextMenuRequested(const QPoint &pos, int index);
    void slotRunControlStarted();
    void slotRunControlFinished();
    void slotRunControlFinished2(ProjectExplorer::RunControl *sender);

    void aboutToUnloadSession();
    void updateFromSettings();
    void enableButtons();

private:
    void enableButtons(const RunControl *rc, bool isRunning);

    struct RunControlTab {
        explicit RunControlTab(RunControl *runControl = 0,
                               Core::OutputWindow *window = 0);
        RunControl* runControl;
        Core::OutputWindow *window;
        // Is the run control stopping asynchronously, close the tab once it finishes
        bool asyncClosing;
        BehavivorOnOutput behavivorOnOutput;
    };

    bool isRunning() const;
    bool closeTab(int index, CloseTabMode cm);
    bool optionallyPromptToStop(RunControl *runControl);

    int indexOf(const RunControl *) const;
    int indexOf(const QWidget *outputWindow) const;
    int currentIndex() const;
    RunControl *currentRunControl() const;
    int tabWidgetIndexOf(int runControlIndex) const;
    void handleOldOutput(Core::OutputWindow *window) const;
    void updateCloseActions();

    QWidget *m_mainWidget;
    class TabWidget *m_tabWidget;
    QList<RunControlTab> m_runControlTabs;
    QAction *m_stopAction;
    QAction *m_closeCurrentTabAction;
    QAction *m_closeAllTabsAction;
    QAction *m_closeOtherTabsAction;
    QToolButton *m_reRunButton;
    QToolButton *m_stopButton;
    QToolButton *m_attachButton;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // APPOUTPUTPANE_H
