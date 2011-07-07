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

#include "qmlprofilerruncontrolfactory.h"
#include "qmlprojectmanager/qmlprojectrunconfiguration.h"

#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzersettings.h>
#include <analyzerbase/analyzerrunconfigwidget.h>

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectexplorer.h>

#include <remotelinux/linuxdeviceconfiguration.h>
#include <remotelinux/remotelinuxrunconfiguration.h>
#include <qt4projectmanager/qt-s60/s60devicedebugruncontrol.h>
#include <qt4projectmanager/qt-s60/s60devicerunconfiguration.h>

#include <utils/qtcassert.h>

#include <QtGui/QAction>

using namespace Analyzer;
using namespace ProjectExplorer;
using namespace QmlProfiler::Internal;
using namespace QmlProjectManager;

QmlProfilerRunControlFactory::QmlProfilerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
    setObjectName(QLatin1String("QmlProfilerRunControlFactory"));
}

bool QmlProfilerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    // FIXME: Should this just accept all  mode == QLatin1String("QmlProfiler"); ?
    if (qobject_cast<QmlProjectRunConfiguration *>(runConfiguration))
        return mode == QLatin1String("QmlProfiler");
    if (qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration))
        return mode == QLatin1String("QmlProfiler");
    if (qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration))
        return mode == QLatin1String("QmlProfiler");
    if (qobject_cast<Qt4ProjectManager::S60DeviceRunConfiguration *>(runConfiguration))
        return mode == QLatin1String("QmlProfiler");
    return false;
}

RunControl *QmlProfilerRunControlFactory::create(RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    AnalyzerStartParameters sp;
    sp.toolId = "QmlProfiler";
    sp.startMode = StartQml; // FIXME: The parameter struct is not needed/not used.


    // FIXME: This is only used to communicate the connParams settings.
    if (QmlProjectRunConfiguration *rc1 =
            qobject_cast<QmlProjectRunConfiguration *>(runConfiguration)) {
        // This is a "plain" .qmlproject.
        sp.environment = rc1->environment();
        sp.workingDirectory = rc1->workingDirectory();
        sp.debuggee = rc1->observerPath();
        sp.debuggeeArgs = rc1->viewerArguments();
        sp.displayName = rc1->displayName();
        sp.connParams.host = QLatin1String("localhost");
        sp.connParams.port = rc1->qmlDebugServerPort();
    } else if (LocalApplicationRunConfiguration *rc2 =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration)) {
        sp.environment = rc2->environment();
        sp.workingDirectory = rc2->workingDirectory();
        sp.debuggee = rc2->executable();
        sp.debuggeeArgs = rc2->commandLineArguments();
        sp.displayName = rc2->displayName();
        sp.connParams.host = QLatin1String("localhost");
        sp.connParams.port = rc2->qmlDebugServerPort();
    } else if (RemoteLinux::RemoteLinuxRunConfiguration *rc3 =
            qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration)) {
        sp.debuggee = rc3->remoteExecutableFilePath();
        sp.debuggeeArgs = rc3->arguments();
        sp.connParams = rc3->deviceConfig()->sshParameters();
        sp.analyzerCmdPrefix = rc3->commandPrefix();
        sp.displayName = rc3->displayName();
    } else if (Qt4ProjectManager::S60DeviceRunConfiguration *rc4 =
        qobject_cast<Qt4ProjectManager::S60DeviceRunConfiguration *>(runConfiguration)) {
        //sp.environment = rc4->environment();
        //sp.workingDirectory = rc4->workingDirectory();
        //sp.debuggee = rc4->executable();
        sp.debuggeeArgs = rc4->commandLineArguments();
        sp.displayName = rc4->displayName();
        sp.connParams.host = QLatin1String("localhost");
        sp.connParams.port = rc4->qmlDebugServerPort();
    } else {
        // What could that be?
        QTC_ASSERT(false, return 0);
    }

    IAnalyzerTool *tool = AnalyzerManager::toolFromId(mode.toLatin1());
    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, runConfiguration);
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));
    return rc;
}

QString QmlProfilerRunControlFactory::displayName() const
{
    return tr("QML Profiler");
}

IRunConfigurationAspect *QmlProfilerRunControlFactory::createRunConfigurationAspect()
{
    return new AnalyzerProjectSettings;
}

RunConfigWidget *QmlProfilerRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    QmlProjectManager::QmlProjectRunConfiguration *localRc =
        qobject_cast<QmlProjectManager::QmlProjectRunConfiguration *>(runConfiguration);
    if (!localRc)
        return 0;

    AnalyzerProjectSettings *settings = runConfiguration->extraAspect<AnalyzerProjectSettings>();
    if (!settings)
        return 0;

    Analyzer::AnalyzerRunConfigWidget *ret = new Analyzer::AnalyzerRunConfigWidget;

    ret->setRunConfiguration(runConfiguration);
    return ret;
}
