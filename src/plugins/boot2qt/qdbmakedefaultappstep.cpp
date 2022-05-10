/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qdbmakedefaultappstep.h"

#include "qdbconstants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <remotelinux/abstractremotelinuxdeploystep.h>

#include <utils/commandline.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb {
namespace Internal {

// QdbMakeDefaultAppService

class QdbMakeDefaultAppService : public RemoteLinux::AbstractRemoteLinuxDeployService
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbMakeDefaultAppService)

public:
    QdbMakeDefaultAppService()
    {
        connect(&m_process, &QtcProcess::done, this, [this] {
            if (m_process.error() != QProcess::UnknownError)
                emit errorMessage(tr("Remote process failed: %1").arg(m_process.errorString()));
            else if (m_makeDefault)
                emit progressMessage(tr("Application set as the default one."));
            else
                emit progressMessage(tr("Reset the default application."));

            stopDeployment();
        });
        connect(&m_process, &QtcProcess::readyReadStandardError, this, [this] {
            emit stdErrData(QString::fromUtf8(m_process.readAllStandardError()));
        });
    }

    void setMakeDefault(bool makeDefault)
    {
        m_makeDefault = makeDefault;
    }

private:
    bool isDeploymentNecessary() const final { return true; }

    void doDeploy() final
    {
        QString remoteExe;

        if (RunConfiguration *rc = target()->activeRunConfiguration()) {
            if (auto exeAspect = rc->aspect<ExecutableAspect>())
                remoteExe = exeAspect->executable().toString();
        }

        const QString args = m_makeDefault && !remoteExe.isEmpty()
                ? QStringLiteral("--make-default ") + remoteExe
                : QStringLiteral("--remove-default");
        m_process.setCommand(
                    {deviceConfiguration()->filePath(Constants::AppcontrollerFilepath), {args}});
        m_process.start();
    }

    void stopDeployment() final
    {
        m_process.close();
        handleDeploymentDone();
    }

    bool m_makeDefault = true;
    QtcProcess m_process;
};


// QdbMakeDefaultAppStep

class QdbMakeDefaultAppStep final : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_DECLARE_TR_FUNCTIONS(Qdb::Internal::QdbMakeDefaultAppStep)

public:
    QdbMakeDefaultAppStep(BuildStepList *bsl, Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
    {
        auto service = createDeployService<QdbMakeDefaultAppService>();

        auto selection = addAspect<SelectionAspect>();
        selection->setSettingsKey("QdbMakeDefaultDeployStep.MakeDefault");
        selection->addOption(tr("Set this application to start by default"));
        selection->addOption(tr("Reset default application"));

        setInternalInitializer([service, selection] {
            service->setMakeDefault(selection->value() == 0);
            return service->isDeploymentPossible();
        });
    }
};


// QdbMakeDefaultAppStepFactory

QdbMakeDefaultAppStepFactory::QdbMakeDefaultAppStepFactory()
{
    registerStep<QdbMakeDefaultAppStep>(Constants::QdbMakeDefaultAppStepId);
    setDisplayName(QdbMakeDefaultAppStep::tr("Change default application"));
    setSupportedDeviceType(Qdb::Constants::QdbLinuxOsType);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // namespace Internal
} // namespace Qdb
