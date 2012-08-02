/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QNX_INTERNAL_QNXDEBUGSUPPORT_H
#define QNX_INTERNAL_QNXDEBUGSUPPORT_H

#include <projectexplorer/devicesupport/idevice.h>

#include <QObject>
#include <QString>

namespace Debugger { class DebuggerEngine; }
namespace ProjectExplorer {
class DeviceApplicationRunner;
class DeviceUsedPortsGatherer;
}

namespace Qnx {
namespace Internal {

class QnxRunConfiguration;

class QnxDebugSupport : public QObject
{
    Q_OBJECT
public:
    explicit QnxDebugSupport(QnxRunConfiguration *runConfig,
                             Debugger::DebuggerEngine *engine);

public slots:
    void handleDebuggingFinished();

private slots:
    void handleAdapterSetupRequested();

    void handleRemoteProcessStarted();
    void handleRemoteProcessFinished(bool success);
    void handleProgressReport(const QString &progressOutput);
    void handleRemoteOutput(const QByteArray &output);
    void handleError(const QString &error);
    void handlePortListReady();

private:
    void startExecution();
    void setFinished();

    enum State {
        Inactive,
        GatheringPorts,
        StartingRemoteProcess,
        Debugging
    };

    const QString m_executable;
    const QString m_commandPrefix;
    const QString m_arguments;
    ProjectExplorer::IDevice::ConstPtr m_device;
    ProjectExplorer::DeviceApplicationRunner *m_runner;
    ProjectExplorer::DeviceUsedPortsGatherer * m_portsGatherer;
    Debugger::DebuggerEngine *m_engine;
    int m_port;

    State m_state;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_QNXDEBUGSUPPORT_H
