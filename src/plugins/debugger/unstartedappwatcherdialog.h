/****************************************************************************
**
** Copyright (C) 2016 Petar Perisin <petar.perisin@gmail.com>
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

#include <QDialog>
#include <QTimer>

#include <projectexplorer/devicesupport/deviceprocesslist.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QCheckBox;
QT_END_NAMESPACE

namespace ProjectExplorer {
class KitChooser;
class Kit;
}

namespace Utils { class PathChooser; }

namespace Debugger {
namespace Internal {

class UnstartedAppWatcherDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UnstartedAppWatcherDialog(QWidget *parent = 0);

    ProjectExplorer::Kit *currentKit() const;
    ProjectExplorer::DeviceProcessItem currentProcess() const;
    bool hideOnAttach() const;
    bool continueOnAttach() const;
    void startWatching();

    bool event(QEvent *) override;

signals:
    void processFound();

private:
    void selectExecutable();
    void pidFound(const ProjectExplorer::DeviceProcessItem &p);
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
    ProjectExplorer::DeviceProcessItem m_process;
    QTimer m_timer;
};

} // namespace Internal
} // namespace Debugger
