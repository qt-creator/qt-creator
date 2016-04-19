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

#include "iosrunconfiguration.h"
#include <QProcess>

namespace Debugger { class DebuggerRunControl; }
namespace ProjectExplorer { class RunControl; }

namespace Ios {
namespace Internal {

class IosRunConfiguration;
class IosRunner;

class IosDebugSupport : public QObject
{
    Q_OBJECT

public:
    static ProjectExplorer::RunControl *createDebugRunControl(IosRunConfiguration *runConfig,
                                                              QString *errorMessage);

    IosDebugSupport(IosRunConfiguration *runConfig,
        Debugger::DebuggerRunControl *runControl, bool cppDebug, bool qmlDebug);
    ~IosDebugSupport();

private:
    void handleServerPorts(Utils::Port gdbServerPort, Utils::Port qmlPort);
    void handleGotInferiorPid(qint64, Utils::Port qmlPort);
    void handleRemoteProcessFinished(bool cleanEnd);

    void handleRemoteOutput(const QString &output);
    void handleRemoteErrorOutput(const QString &output);

    Debugger::DebuggerRunControl *m_runControl;
    IosRunner * const m_runner;
    const QString m_dumperLib;
};

} // namespace Internal
} // namespace Ios
