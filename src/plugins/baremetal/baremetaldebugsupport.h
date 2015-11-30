/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BAREMETALDEBUGSUPPORT_H
#define BAREMETALDEBUGSUPPORT_H

#include <QObject>
#include <QPointer>

#include <projectexplorer/devicesupport/idevice.h>

namespace Debugger { class DebuggerRunControl; }

namespace ProjectExplorer {
class DeviceApplicationRunner;
class IDevice;
}

namespace BareMetal {
namespace Internal {

class GdbServerProvider;
class BareMetalRunConfiguration;

class BareMetalDebugSupport : public QObject
{
    Q_OBJECT

public:
    explicit BareMetalDebugSupport(const ProjectExplorer::IDevice::ConstPtr device,
                                   Debugger::DebuggerRunControl *runControl);
    ~BareMetalDebugSupport();

private slots:
    void remoteSetupRequested();
    void debuggingFinished();
    void remoteOutputMessage(const QByteArray &output);
    void remoteErrorOutputMessage(const QByteArray &output);
    void remoteProcessStarted();
    void appRunnerFinished(bool success);
    void progressReport(const QString &progressOutput);
    void appRunnerError(const QString &error);

private:
    enum State { Inactive, StartingRunner, Running };

    void adapterSetupDone();
    void adapterSetupFailed(const QString &error);

    void startExecution();
    void setFinished();
    void reset();
    void showMessage(const QString &msg, int channel);

    QPointer<ProjectExplorer::DeviceApplicationRunner> m_appRunner;
    const QPointer<Debugger::DebuggerRunControl> m_runControl;
    const ProjectExplorer::IDevice::ConstPtr m_device;
    BareMetalDebugSupport::State m_state;
};

} // namespace Internal
} // namespace BareMetal

#endif // BAREMETALDEBUGSUPPORT_H
