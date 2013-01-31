/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmlprojectruncontrol.h"
#include "qmlprojectrunconfiguration.h"
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/qtcassert.h>

#include <debugger/debuggerrunner.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerruncontrolfactory.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qmlobservertool.h>

#include <qmlprojectmanager/qmlprojectplugin.h>

using namespace ProjectExplorer;

namespace QmlProjectManager {

namespace Internal {

QmlProjectRunControl::QmlProjectRunControl(QmlProjectRunConfiguration *runConfiguration, RunMode mode)
    : RunControl(runConfiguration, mode)
{
    m_applicationLauncher.setEnvironment(runConfiguration->environment());
    m_applicationLauncher.setWorkingDirectory(runConfiguration->workingDirectory());

    if (mode == NormalRunMode)
        m_executable = runConfiguration->viewerPath();
    else
        m_executable = runConfiguration->observerPath();
    m_commandLineArguments = runConfiguration->viewerArguments();
    m_mainQmlFile = runConfiguration->mainScript();

    connect(&m_applicationLauncher, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SLOT(slotAppendMessage(QString,Utils::OutputFormat)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(slotBringApplicationToForeground(qint64)));
}

QmlProjectRunControl::~QmlProjectRunControl()
{
    stop();
}

void QmlProjectRunControl::start()
{
    m_applicationLauncher.start(ApplicationLauncher::Gui, m_executable,
                                m_commandLineArguments);
    setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
    emit started();
    QString msg = tr("Starting %1 %2\n")
        .arg(QDir::toNativeSeparators(m_executable), m_commandLineArguments);
    appendMessage(msg, Utils::NormalMessageFormat);
}

RunControl::StopResult QmlProjectRunControl::stop()
{
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

bool QmlProjectRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

QIcon QmlProjectRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

void QmlProjectRunControl::slotBringApplicationToForeground(qint64 pid)
{
    bringApplicationToForeground(pid);
}

void QmlProjectRunControl::slotAppendMessage(const QString &line, Utils::OutputFormat format)
{
    appendMessage(line, format);
}

void QmlProjectRunControl::processExited(int exitCode)
{
    QString msg = tr("%1 exited with code %2\n")
        .arg(QDir::toNativeSeparators(m_executable)).arg(exitCode);
    appendMessage(msg, exitCode ? Utils::ErrorMessageFormat : Utils::NormalMessageFormat);
    emit finished();
}

QString QmlProjectRunControl::mainQmlFile() const
{
    return m_mainQmlFile;
}

QmlProjectRunControlFactory::QmlProjectRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

QmlProjectRunControlFactory::~QmlProjectRunControlFactory()
{
}

bool QmlProjectRunControlFactory::canRun(RunConfiguration *runConfiguration,
                                         RunMode mode) const
{
    QmlProjectRunConfiguration *config =
        qobject_cast<QmlProjectRunConfiguration*>(runConfiguration);
    if (!config)
        return false;
    if (mode == NormalRunMode)
        return !config->viewerPath().isEmpty();
    if (mode != DebugRunMode)
        return false;

    if (!Debugger::DebuggerPlugin::isActiveDebugLanguage(Debugger::QmlLanguage))
        return false;

    if (!config->observerPath().isEmpty())
        return true;
    if (!config->qtVersion())
        return false;
    if (!config->qtVersion()->needsQmlDebuggingLibrary())
        return true;
    if (QtSupport::QmlObserverTool::canBuild(config->qtVersion()))
        return true;
    return false;
}

RunControl *QmlProjectRunControlFactory::create(RunConfiguration *runConfiguration,
                                                RunMode mode, QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    QmlProjectRunConfiguration *config = qobject_cast<QmlProjectRunConfiguration *>(runConfiguration);

    QList<ProjectExplorer::RunControl *> runcontrols =
            ProjectExplorer::ProjectExplorerPlugin::instance()->runControls();
    foreach (ProjectExplorer::RunControl *rc, runcontrols) {
        if (QmlProjectRunControl *qrc = qobject_cast<QmlProjectRunControl *>(rc)) {
            if (qrc->mainQmlFile() == config->mainScript())
                // Asking the user defeats the purpose
                // Making it configureable might be worth it
                qrc->stop();
        }
    }

    RunControl *runControl = 0;
    if (mode == NormalRunMode)
        runControl = new QmlProjectRunControl(config, mode);
    else if (mode == DebugRunMode)
        runControl = createDebugRunControl(config, errorMessage);
    return runControl;
}

QString QmlProjectRunControlFactory::displayName() const
{
    return tr("Run");
}

RunControl *QmlProjectRunControlFactory::createDebugRunControl(QmlProjectRunConfiguration *runConfig, QString *errorMessage)
{
    Debugger::DebuggerStartParameters params;
    params.startMode = Debugger::StartInternal;
    params.executable = runConfig->observerPath();
    params.qmlServerAddress = QLatin1String("127.0.0.1");
    params.qmlServerPort = runConfig->debuggerAspect()->qmlDebugServerPort();
    params.processArgs = QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(params.qmlServerPort);
    params.processArgs += QLatin1Char(' ') + runConfig->viewerArguments();
    params.workingDirectory = runConfig->workingDirectory();
    params.environment = runConfig->environment();
    params.displayName = runConfig->displayName();
    params.projectSourceDirectory = runConfig->target()->project()->projectDirectory();
    params.projectSourceFiles = runConfig->target()->project()->files(Project::ExcludeGeneratedFiles);
    if (runConfig->debuggerAspect()->useQmlDebugger())
        params.languages |= Debugger::QmlLanguage;
    if (runConfig->debuggerAspect()->useCppDebugger())
        params.languages |= Debugger::CppLanguage;

    // Makes sure that all bindings go through the JavaScript engine, so that
    // breakpoints are actually hit!
    const QString optimizerKey = QLatin1String("QML_DISABLE_OPTIMIZER");
    if (!params.environment.hasKey(optimizerKey))
        params.environment.set(optimizerKey, QLatin1String("1"));

    if (params.executable.isEmpty()) {
        QmlProjectPlugin::showQmlObserverToolWarning();
        errorMessage->clear(); // hack, we already showed a error message
        return 0;
    }

    return Debugger::DebuggerPlugin::createDebugger(params, runConfig, errorMessage);
}

} // namespace Internal
} // namespace QmlProjectManager
