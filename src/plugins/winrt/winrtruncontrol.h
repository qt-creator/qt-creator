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

#ifndef WINRTRUNCONTROL_H
#define WINRTRUNCONTROL_H

#include "winrtdevice.h"

#include <projectexplorer/runconfiguration.h>
#include <utils/qtcprocess.h>

namespace QtSupport {
class BaseQtVersion;
} // namespace QtSupport

namespace ProjectExplorer {
class Target;
} // namespace ProjectExplorer

namespace WinRt {
namespace Internal {

class WinRtRunConfiguration;
class WinRtRunnerHelper;

class WinRtRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    explicit WinRtRunControl(WinRtRunConfiguration *runConfiguration, Core::Id mode);

    void start() override;
    StopResult stop() override;
    bool isRunning() const override;

private slots:
    void onProcessStarted();
    void onProcessFinished();
    void onProcessError();

private:
    enum State { StartingState, StartedState, StoppedState };
    bool startWinRtRunner();

    WinRtRunConfiguration *m_runConfiguration;
    State m_state;
    Utils::QtcProcess *m_process;
    WinRtRunnerHelper *m_runner;
};

} // namespace Internal
} // namespace WinRt

#endif // WINRTRUNCONTROL_H
