/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "analyzerruncontrolfactory.h"
#include "analyzerruncontrol.h"
#include "analyzerconstants.h"
#include "analyzerrunconfigwidget.h"
#include "analyzersettings.h"
#include "analyzerstartparameters.h"

#include <utils/qtcassert.h>

#include <projectexplorer/applicationrunconfiguration.h>

#include <remotelinux/remotelinuxrunconfiguration.h>

#include <QtCore/QDebug>

using namespace Analyzer;
using namespace Analyzer::Internal;

AnalyzerStartParameters localStartParameters(ProjectExplorer::RunConfiguration *runConfiguration)
{
    AnalyzerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    ProjectExplorer::LocalApplicationRunConfiguration *rc =
            qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.startMode = StartLocal;
    sp.environment = rc->environment();
    sp.workingDirectory = rc->workingDirectory();
    sp.debuggee = rc->executable();
    sp.debuggeeArgs = rc->commandLineArguments();
    sp.displayName = rc->displayName();
    sp.connParams.host = QLatin1String("localhost");
    sp.connParams.port = rc->qmlDebugServerPort();
    return sp;
}

AnalyzerStartParameters remoteLinuxStartParameters(ProjectExplorer::RunConfiguration *runConfiguration)
{
    AnalyzerStartParameters sp;
    RemoteLinux::RemoteLinuxRunConfiguration * const rc
        = qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.debuggee = rc->remoteExecutableFilePath();
    sp.debuggeeArgs = rc->arguments();
    sp.connParams = rc->deviceConfig()->sshParameters();
    sp.analyzerCmdPrefix = rc->commandPrefix();
    sp.startMode = StartRemote;
    sp.displayName = rc->displayName();
    return sp;
}


// AnalyzerRunControlFactory ////////////////////////////////////////////////////
AnalyzerRunControlFactory::AnalyzerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool AnalyzerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    return runConfiguration->isEnabled() && mode == Constants::MODE_ANALYZE
            && (qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration)
                || qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration));
}

ProjectExplorer::RunControl *AnalyzerRunControlFactory::create(RunConfiguration *runConfiguration,
                                                               const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    const AnalyzerStartParameters sp
        = qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration)
            ? localStartParameters(runConfiguration) : remoteLinuxStartParameters(runConfiguration);
    return create(sp, runConfiguration);
}

AnalyzerRunControl *AnalyzerRunControlFactory::create(const AnalyzerStartParameters &sp,
                                                               RunConfiguration *runConfiguration)
{
    AnalyzerRunControl *rc = new AnalyzerRunControl(sp, runConfiguration);
    emit runControlCreated(rc);
    return rc;
}

QString AnalyzerRunControlFactory::displayName() const
{
    return tr("Analyzer");
}

ProjectExplorer::IRunConfigurationAspect *AnalyzerRunControlFactory::createRunConfigurationAspect()
{
    return new AnalyzerProjectSettings;
}

ProjectExplorer::RunConfigWidget *AnalyzerRunControlFactory::createConfigurationWidget(RunConfiguration
                                                                                       *runConfiguration)
{
    ProjectExplorer::LocalApplicationRunConfiguration *localRc =
        qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration);
    if (!localRc)
        return 0;
    AnalyzerProjectSettings *settings = runConfiguration->extraAspect<AnalyzerProjectSettings>();
    if (!settings)
        return 0;

    AnalyzerRunConfigWidget *ret = new AnalyzerRunConfigWidget;
    ret->setRunConfiguration(runConfiguration);
    return ret;
}
