/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "killappstep.h"

#include "abstractremotelinuxdeployservice.h"
#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class KillAppService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    ~KillAppService() override;

    void setRemoteExecutable(const QString &filePath);

private:
    void handleStdErr();
    void handleProcessFinished();

    bool isDeploymentNecessary() const override;

    void doDeploy() override;
    void stopDeployment() override;

    void handleSignalOpFinished(const QString &errorMessage);
    void cleanup();
    void finishDeployment();

    QString m_remoteExecutable;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr m_signalOperation;
};

KillAppService::~KillAppService()
{
    cleanup();
}

void KillAppService::setRemoteExecutable(const QString &filePath)
{
    m_remoteExecutable = filePath;
}

bool KillAppService::isDeploymentNecessary() const
{
    return !m_remoteExecutable.isEmpty();
}

void KillAppService::doDeploy()
{
    m_signalOperation = deviceConfiguration()->signalOperation();
    if (!m_signalOperation) {
        handleDeploymentDone();
        return;
    }
    connect(m_signalOperation.data(), &ProjectExplorer::DeviceProcessSignalOperation::finished,
            this, &KillAppService::handleSignalOpFinished);
    emit progressMessage(tr("Trying to kill \"%1\" on remote device...").arg(m_remoteExecutable));
    m_signalOperation->killProcess(m_remoteExecutable);
}

void KillAppService::cleanup()
{
    if (m_signalOperation) {
        disconnect(m_signalOperation.data(), nullptr, this, nullptr);
        m_signalOperation.clear();
    }
}

void KillAppService::finishDeployment()
{
    cleanup();
    handleDeploymentDone();
}

void KillAppService::stopDeployment()
{
    finishDeployment();
}

void KillAppService::handleSignalOpFinished(const QString &errorMessage)
{
    if (errorMessage.isEmpty())
        emit progressMessage(tr("Remote application killed."));
    else
        emit progressMessage(tr("Failed to kill remote application. Assuming it was not running."));
    finishDeployment();
}

} // namespace Internal

KillAppStep::KillAppStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<Internal::KillAppService>();

    setWidgetExpandedByDefault(false);

    setInternalInitializer([this, service] {
        Target * const theTarget = target();
        QTC_ASSERT(theTarget, return CheckResult::failure());
        RunConfiguration * const rc = theTarget->activeRunConfiguration();
        const QString remoteExe = rc ? rc->runnable().command.executable().path() : QString();
        service->setRemoteExecutable(remoteExe);
        return CheckResult::success();
    });
}

Id KillAppStep::stepId()
{
    return Constants::KillAppStepId;
}

QString KillAppStep::displayName()
{
    return tr("Kill current application instance");
}

} // namespace RemoteLinux

#include "killappstep.moc"
