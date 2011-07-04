/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "analyzerplugin.h"

#include "analyzerconstants.h"
#include "analyzermanager.h"
#include "analyzerruncontrol.h"
#include "analyzersettings.h"
#include "analyzerstartparameters.h"
#include "analyzerrunconfigwidget.h"
#include "startremotedialog.h"
#include "ianalyzertool.h"

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <remotelinux/linuxdeviceconfiguration.h>
#include <remotelinux/remotelinuxrunconfiguration.h>

#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtGui/QAction>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QApplication>

using namespace Analyzer;
using namespace Analyzer::Internal;
using namespace ProjectExplorer;

static AnalyzerPlugin *m_instance = 0;

namespace Analyzer {
namespace Internal {

/////////////////////////////////////////////////////////////////////////////////
//
// AnalyzerRunControlFactory
//
/////////////////////////////////////////////////////////////////////////////////

static AnalyzerStartParameters localStartParameters(RunConfiguration *runConfiguration)
{
    AnalyzerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    LocalApplicationRunConfiguration *rc =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
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

static AnalyzerStartParameters remoteLinuxStartParameters(RunConfiguration *runConfiguration)
{
    AnalyzerStartParameters sp;
    RemoteLinux::RemoteLinuxRunConfiguration * const rc
        = qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.startMode = StartRemote;
    sp.debuggee = rc->remoteExecutableFilePath();
    sp.debuggeeArgs = rc->arguments();
    sp.connParams = rc->deviceConfig()->sshParameters();
    sp.analyzerCmdPrefix = rc->commandPrefix();
    sp.displayName = rc->displayName();
    return sp;
}


class AnalyzerRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    AnalyzerRunControlFactory(QObject *parent = 0);

    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    typedef ProjectExplorer::RunControl RunControl;

    // IRunControlFactory
    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
    QString displayName() const;

    ProjectExplorer::IRunConfigurationAspect *createRunConfigurationAspect();
    ProjectExplorer::RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration);
};

AnalyzerRunControlFactory::AnalyzerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
    setObjectName(QLatin1String("AnalyzerRunControlFactory"));
}

bool AnalyzerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    Q_UNUSED(runConfiguration);
    // FIXME: This is not generic.
    return mode.startsWith("Callgrind") || mode.startsWith("Memcheck") || mode == "QmlProfiler";
}

RunControl *AnalyzerRunControlFactory::create(RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    AnalyzerStartParameters sp;
    if (qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>(runConfiguration)) {
        sp = localStartParameters(runConfiguration);
    } else if (qobject_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(runConfiguration)) {
        sp = remoteLinuxStartParameters(runConfiguration);
    } else {
        // Might be S60DeviceRunfiguration, or something else ...
        //sp.startMode = StartRemote;
        sp.startMode = StartRemote;
    }

    IAnalyzerTool *tool = AnalyzerManager::toolFromId(mode.toLatin1());
    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, runConfiguration);
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));
    //m_isRunning = true;
    return rc;
}

QString AnalyzerRunControlFactory::displayName() const
{
    return tr("Analyzer");
}

IRunConfigurationAspect *AnalyzerRunControlFactory::createRunConfigurationAspect()
{
    return new AnalyzerProjectSettings;
}

RunConfigWidget *AnalyzerRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
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

////////////////////////////////////////////////////////////////////////
//
// AnalyzerPluginPrivate
//
////////////////////////////////////////////////////////////////////////

class AnalyzerPlugin::AnalyzerPluginPrivate
{
public:
    AnalyzerPluginPrivate(AnalyzerPlugin *qq):
        q(qq),
        m_manager(0)
    {}

    AnalyzerPlugin *q;
    AnalyzerManager *m_manager;
};

////////////////////////////////////////////////////////////////////////
//
// AnalyzerPlugin
//
////////////////////////////////////////////////////////////////////////

AnalyzerPlugin::AnalyzerPlugin()
    : d(new AnalyzerPluginPrivate(this))
{
    m_instance = this;
}

AnalyzerPlugin::~AnalyzerPlugin()
{
    delete d;
    m_instance = 0;
}

bool AnalyzerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d->m_manager = new AnalyzerManager(this);

    // Task integration.
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    ProjectExplorer::TaskHub *hub = pm->getObject<ProjectExplorer::TaskHub>();
    //: Category under which Analyzer tasks are listed in build issues view
    hub->addCategory(QLatin1String(Constants::ANALYZERTASK_ID), tr("Analyzer"));

    AnalyzerRunControlFactory *factory = new AnalyzerRunControlFactory();
    addAutoReleasedObject(factory);

    return true;
}

void AnalyzerPlugin::extensionsInitialized()
{
    d->m_manager->extensionsInitialized();
}

ExtensionSystem::IPlugin::ShutdownFlag AnalyzerPlugin::aboutToShutdown()
{
    d->m_manager->shutdown();
    return SynchronousShutdown;
}

AnalyzerPlugin *AnalyzerPlugin::instance()
{
    return m_instance;
}

} // namespace Internal
} // namespace Analyzer

Q_EXPORT_PLUGIN(AnalyzerPlugin)

#include "analyzerplugin.moc"
