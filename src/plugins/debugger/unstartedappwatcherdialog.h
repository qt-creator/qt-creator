// Copyright (C) 2016 Petar Perisin <petar.perisin@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>
#include <QSet>
#include <QTimer>

#include <utils/processinfo.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QCheckBox;
QT_END_NAMESPACE

namespace ProjectExplorer {
class KitChooser;
class Kit;
}

namespace Utils { class PathChooser; }

namespace Debugger::Internal {

class UnstartedAppWatcherDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UnstartedAppWatcherDialog(QWidget *parent = nullptr);

    ProjectExplorer::Kit *currentKit() const;
    Utils::ProcessInfo currentProcess() const;
    bool hideOnAttach() const;
    bool continueOnAttach() const;
    void startWatching();

    bool event(QEvent *) override;

signals:
    void processFound();

private:
    void selectExecutable();
    void pidFound(const Utils::ProcessInfo &p);
    void startStopWatching(bool start);
    void findProcess();
    void stopAndCheckExecutable();
    void kitChanged();

    enum UnstartedAppWacherState
    {
        InvalidWacherState,
        NotWatchingState,
        WatchingState,
        FoundState
    };

    void startStopTimer(bool start);
    bool checkExecutableString() const;
    void setWaitingState(UnstartedAppWacherState state);

    ProjectExplorer::KitChooser *m_kitChooser;
    Utils::PathChooser *m_pathChooser;
    QLabel *m_waitingLabel;
    QCheckBox *m_hideOnAttachCheckBox;
    QCheckBox *m_continueOnAttachCheckBox;
    QPushButton *m_watchingPushButton;
    Utils::ProcessInfo m_process;
    QSet<int> m_excluded;
    QTimer m_timer;
};

} // Debugger::Internal
