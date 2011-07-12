/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"
#include "valgrindsettings.h"

#include <analyzerbase/analyzerconstants.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerrunconfigwidget.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/startremotedialog.h>

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectexplorer.h>

#include <remotelinux/linuxdeviceconfiguration.h>
#include <remotelinux/remotelinuxrunconfiguration.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QtPlugin>
#include <QtGui/QAction>

using namespace Analyzer;
using namespace Valgrind::Internal;
using namespace ProjectExplorer;

/////////////////////////////////////////////////////////////////////////////////
//
// ValgrindRunControlFactory
//
/////////////////////////////////////////////////////////////////////////////////

namespace Valgrind {
namespace Internal {

class ValgrindRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    ValgrindRunControlFactory(QObject *parent = 0);

    // IRunControlFactory
    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
    QString displayName() const;

    IRunConfigurationAspect *createRunConfigurationAspect();
    RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration);
};

ValgrindRunControlFactory::ValgrindRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
    setObjectName(QLatin1String("ValgrindRunControlFactory"));
}

bool ValgrindRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    Q_UNUSED(runConfiguration);
    return mode.startsWith("Callgrind") || mode.startsWith("Memcheck");
}

RunControl *ValgrindRunControlFactory::create(RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    AnalyzerStartParameters sp;
    if (LocalApplicationRunConfiguration *rc1 =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration)) {
        sp.startMode = StartLocal;
        sp.environment = rc1->environment();
        sp.workingDirectory = rc1->workingDirectory();
        sp.debuggee = rc1->executable();
        sp.debuggeeArgs = rc1->commandLineArguments();
        sp.displayName = rc1->displayName();
        sp.connParams.host = QLatin1String("localhost");
        sp.connParams.port = rc1->qmlDebugServerPort();
    } else if (RemoteLinux::RemoteLinuxRunConfiguration *rc2 =
               qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration)) {
        sp.startMode = StartRemote;
        sp.debuggee = rc2->remoteExecutableFilePath();
        sp.debuggeeArgs = rc2->arguments();
        sp.connParams = rc2->deviceConfig()->sshParameters();
        sp.analyzerCmdPrefix = rc2->commandPrefix();
        sp.displayName = rc2->displayName();
    } else {
        // Might be S60DeviceRunfiguration, or something else ...
        //sp.startMode = StartRemote;
        sp.startMode = StartRemote;
    }

    IAnalyzerTool *tool = AnalyzerManager::toolFromId(mode.toLatin1());
    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, runConfiguration);
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));
    return rc;
}

QString ValgrindRunControlFactory::displayName() const
{
    return tr("Analyzer");
}

IRunConfigurationAspect *ValgrindRunControlFactory::createRunConfigurationAspect()
{
    return new AnalyzerProjectSettings;
}

RunConfigWidget *ValgrindRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    LocalApplicationRunConfiguration *localRc =
        qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    if (!localRc)
        return 0;
    AnalyzerProjectSettings *settings = runConfiguration->extraAspect<AnalyzerProjectSettings>();
    if (!settings)
        return 0;

    AnalyzerRunConfigWidget *ret = new AnalyzerRunConfigWidget;
    ret->setRunConfiguration(runConfiguration);
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////
//
// ValgrindPlugin
//
/////////////////////////////////////////////////////////////////////////////////

static void startRemoteTool(IAnalyzerTool *tool, StartMode mode)
{
    Q_UNUSED(tool);
    StartRemoteDialog dlg;
    if (dlg.exec() != QDialog::Accepted)
        return;

    AnalyzerStartParameters sp;
    sp.toolId = tool->id();
    sp.startMode = mode;
    sp.connParams = dlg.sshParams();
    sp.debuggee = dlg.executable();
    sp.debuggeeArgs = dlg.arguments();
    sp.displayName = dlg.executable();
    sp.workingDirectory = dlg.workingDirectory();

    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, 0);
    //m_currentRunControl = rc;
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));

    ProjectExplorerPlugin::instance()->startRunControl(rc, tool->id());
}

void ValgrindPlugin::startValgrindTool(IAnalyzerTool *tool, StartMode mode)
{
    if (mode == StartLocal)
        AnalyzerManager::startLocalTool(tool, mode);
    if (mode == StartRemote)
        startRemoteTool(tool, mode);
}

static AbstractAnalyzerSubConfig *globalValgrindFactory()
{
    return new ValgrindGlobalSettings();
}

static AbstractAnalyzerSubConfig *projectValgrindFactory()
{
    return new ValgrindProjectSettings();
}

bool ValgrindPlugin::initialize(const QStringList &, QString *)
{
    AnalyzerGlobalSettings::instance()->registerSubConfigs(&globalValgrindFactory, &projectValgrindFactory);

    StartModes modes;
#ifndef Q_OS_WIN
    modes.append(StartMode(StartLocal));
#endif
    modes.append(StartMode(StartRemote));

    AnalyzerManager::addTool(new MemcheckTool(this), modes);
    AnalyzerManager::addTool(new CallgrindTool(this), modes);

    ValgrindRunControlFactory *factory = new ValgrindRunControlFactory();
    addAutoReleasedObject(factory);

    return true;
}

} // namespace Internal
} // namespace Valgrind


Q_EXPORT_PLUGIN(Valgrind::Internal::ValgrindPlugin)

#include "valgrindplugin.moc"
