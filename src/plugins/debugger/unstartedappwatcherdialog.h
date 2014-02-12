/****************************************************************************
**
** Copyright (C) 2014 Petar Perisin <petar.perisin@gmail.com>
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

#ifndef UNSTARTEDAPPWATCHERDIALOG_H
#define UNSTARTEDAPPWATCHERDIALOG_H

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

public slots:
    void selectExecutable();
    void startWatching();
    void pidFound(const ProjectExplorer::DeviceProcessItem &p);
    void startStopWatching(bool start);
    void findProcess();
    void stopAndCheckExecutable();

signals:
    void processFound();

private:
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
    QPushButton *m_closePushButton;
    ProjectExplorer::DeviceProcessItem m_process;
    QTimer m_timer;
};

} // namespace Internal
} // namespace Debugger

#endif // UNSTARTEDAPPWATCHERDIALOG_H
