/****************************************************************************
**
** Copyright (C) 2014 Kläralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#include "valgrindruncontrolfactory.h"
#include "valgrindsettings.h"
#include "valgrindplugin.h"

#include <analyzerbase/ianalyzertool.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerrunconfigwidget.h>

#include <remotelinux/abstractremotelinuxrunconfiguration.h>

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

#include <QTcpServer>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace Valgrind {
namespace Internal {

ValgrindRunControlFactory::ValgrindRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

bool ValgrindRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    Q_UNUSED(runConfiguration);
    return mode == CallgrindRunMode || mode == MemcheckRunMode;
}

RunControl *ValgrindRunControlFactory::create(RunConfiguration *runConfiguration, RunMode mode, QString *errorMessage)
{
    Q_UNUSED(errorMessage);

    AnalyzerStartParameters sp;
    sp.displayName = runConfiguration->displayName();
    sp.runMode = mode;
    if (LocalApplicationRunConfiguration *rc1 =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration)) {
        EnvironmentAspect *aspect = runConfiguration->extraAspect<EnvironmentAspect>();
        if (aspect)
            sp.environment = aspect->environment();
        sp.workingDirectory = rc1->workingDirectory();
        sp.debuggee = rc1->executable();
        sp.debuggeeArgs = rc1->commandLineArguments();
        const IDevice::ConstPtr device =
                DeviceKitInformation::device(runConfiguration->target()->kit());
        QTC_ASSERT(device, return 0);
        QTC_ASSERT(device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE, return 0);
        QTcpServer server;
        if (!server.listen(QHostAddress::LocalHost) && !server.listen(QHostAddress::LocalHostIPv6)) {
            qWarning() << "Cannot open port on host for profiling.";
            return 0;
        }
        sp.connParams.host = server.serverAddress().toString();
        sp.connParams.port = server.serverPort();
        sp.startMode = StartLocal;
        sp.localRunMode = static_cast<ProjectExplorer::ApplicationLauncher::Mode>(rc1->runMode());
    } else if (RemoteLinux::AbstractRemoteLinuxRunConfiguration *rc2 =
               qobject_cast<RemoteLinux::AbstractRemoteLinuxRunConfiguration *>(runConfiguration)) {
        sp.startMode = StartRemote;
        sp.debuggee = rc2->remoteExecutableFilePath();
        sp.connParams = DeviceKitInformation::device(rc2->target()->kit())->sshParameters();
        sp.debuggeeArgs = rc2->arguments().join(QLatin1String(" "));
    } else {
        QTC_ASSERT(false, return 0);
    }

    return AnalyzerManager::createRunControl(sp, runConfiguration);
}


class ValgrindRunConfigurationAspect : public IRunConfigurationAspect
{
public:
    ValgrindRunConfigurationAspect(RunConfiguration *parent)
        : IRunConfigurationAspect(parent)
    {
        setProjectSettings(new ValgrindProjectSettings());
        setGlobalSettings(ValgrindPlugin::globalSettings());
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(QCoreApplication::translate("Valgrind::Internal::ValgrindRunConfigurationAspect", "Valgrind Settings"));
        setUsingGlobalSettings(true);
        resetProjectToGlobalSettings();
    }

    IRunConfigurationAspect *create(RunConfiguration *parent) const
    {
        return new ValgrindRunConfigurationAspect(parent);
    }

    RunConfigWidget *createConfigurationWidget()
    {
        return new Analyzer::AnalyzerRunConfigWidget(this);

    }
};

IRunConfigurationAspect *ValgrindRunControlFactory::createRunConfigurationAspect(RunConfiguration *rc)
{
    return new ValgrindRunConfigurationAspect(rc);
}

} // namespace Internal
} // namespace Valgrind
