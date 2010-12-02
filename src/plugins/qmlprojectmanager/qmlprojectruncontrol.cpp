/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlprojectruncontrol.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojectconstants.h"
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/applicationlauncher.h>
#include <qt4projectmanager/qtversionmanager.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <debugger/debuggerrunner.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggerengine.h>
#include <qmljsinspector/qmljsinspectorconstants.h>
#include <qt4projectmanager/qtversionmanager.h>
#include <qt4projectmanager/qmlobservertool.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

using ProjectExplorer::RunConfiguration;
using ProjectExplorer::RunControl;

namespace QmlProjectManager {
namespace Internal {

QmlRunControl::QmlRunControl(QmlProjectRunConfiguration *runConfiguration, QString mode)
    : RunControl(runConfiguration, mode)
{
    m_applicationLauncher.setEnvironment(runConfiguration->environment());
    m_applicationLauncher.setWorkingDirectory(runConfiguration->workingDirectory());

    if (mode == ProjectExplorer::Constants::RUNMODE) {
        m_executable = runConfiguration->viewerPath();
    } else {
        m_executable = runConfiguration->observerPath();
    }
    m_commandLineArguments = runConfiguration->viewerArguments();

    connect(&m_applicationLauncher, SIGNAL(appendMessage(QString,bool)),
            this, SLOT(slotError(QString, bool)));
    connect(&m_applicationLauncher, SIGNAL(appendOutput(QString, bool)),
            this, SLOT(slotAddToOutputWindow(QString, bool)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(slotBringApplicationToForeground(qint64)));
}

QmlRunControl::~QmlRunControl()
{
    stop();
}

void QmlRunControl::start()
{
    m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui, m_executable,
                                m_commandLineArguments);

    emit started();
    emit appendMessage(this, tr("Starting %1 %2").arg(QDir::toNativeSeparators(m_executable),
                                                      m_commandLineArguments), false);
}

RunControl::StopResult QmlRunControl::stop()
{
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

bool QmlRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

void QmlRunControl::slotBringApplicationToForeground(qint64 pid)
{
    bringApplicationToForeground(pid);
}

void QmlRunControl::slotError(const QString &err, bool isError)
{
    emit appendMessage(this, err, isError);
    emit finished();
}

void QmlRunControl::slotAddToOutputWindow(const QString &line, bool onStdErr)
{
    emit addToOutputWindowInline(this, line, onStdErr);
}

void QmlRunControl::processExited(int exitCode)
{
    emit appendMessage(this, tr("%1 exited with code %2").arg(QDir::toNativeSeparators(m_executable)).arg(exitCode), exitCode != 0);
    emit finished();
}

QmlRunControlFactory::QmlRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

QmlRunControlFactory::~QmlRunControlFactory()
{
}

bool QmlRunControlFactory::canRun(RunConfiguration *runConfiguration,
                                  const QString &mode) const
{
    QmlProjectRunConfiguration *config =
        qobject_cast<QmlProjectRunConfiguration*>(runConfiguration);
    if (mode == ProjectExplorer::Constants::RUNMODE)
        return config != 0 && !config->viewerPath().isEmpty();

    bool qmlDebugSupportInstalled =
            Debugger::DebuggerPlugin::isActiveDebugLanguage(Debugger::QmlLanguage);

    if (config && qmlDebugSupportInstalled) {
        if (!config->observerPath().isEmpty())
            return true;

        if (config->qtVersion() && Qt4ProjectManager::QmlObserverTool::canBuild(config->qtVersion())) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

RunControl *QmlRunControlFactory::create(RunConfiguration *runConfiguration,
                                         const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    QmlProjectRunConfiguration *config = qobject_cast<QmlProjectRunConfiguration *>(runConfiguration);
    RunControl *runControl = 0;
    if (mode == ProjectExplorer::Constants::RUNMODE) {
       runControl = new QmlRunControl(config, mode);
    } else if (mode == Debugger::Constants::DEBUGMODE) {
        runControl = createDebugRunControl(config);
    }
    return runControl;
}

QString QmlRunControlFactory::displayName() const
{
    return tr("Run");
}

QWidget *QmlRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    Q_UNUSED(runConfiguration)
    return new QLabel("TODO add Configuration widget");
}

ProjectExplorer::RunControl *QmlRunControlFactory::createDebugRunControl(QmlProjectRunConfiguration *runConfig)
{
    Debugger::DebuggerStartParameters params;
    params.startMode = Debugger::StartInternal;
    params.executable = runConfig->observerPath();
    params.qmlServerAddress = "127.0.0.1";
    params.qmlServerPort = runConfig->qmlDebugServerPort();
    params.processArgs = runConfig->viewerArguments();
    Utils::QtcProcess::addArg(&params.processArgs,
            QLatin1String("-qmljsdebugger=port:") + QString::number(runConfig->qmlDebugServerPort()));
    params.workingDirectory = runConfig->workingDirectory();
    params.environment = runConfig->environment();
    params.displayName = runConfig->displayName();

    if (params.executable.isEmpty()) {
        showQmlObserverToolWarning();
        return 0;
    }

    return Debugger::DebuggerPlugin::createDebugger(params, runConfig);
}

void QmlRunControlFactory::showQmlObserverToolWarning() {
    QMessageBox dialog(QApplication::activeWindow());
    QPushButton *qtPref = dialog.addButton(tr("Open Qt4 Options"),
                                           QMessageBox::ActionRole);
    dialog.addButton(tr("Cancel"), QMessageBox::ActionRole);
    dialog.setDefaultButton(qtPref);
    dialog.setWindowTitle(tr("QML Observer Missing"));
    dialog.setText(tr("QML Observer could not be found."));
    dialog.setInformativeText(tr(
                                  "QML Observer is used to offer debugging features for "
                                  "QML applications, such as interactive debugging and inspection tools. "
                                  "It must be compiled for each used Qt version separately. "
                                  "On the Qt4 options page, select the current Qt installation "
                                  "and click Rebuild."));
    dialog.exec();
    if (dialog.clickedButton() == qtPref) {
        Core::ICore::instance()->showOptionsDialog(
                    Qt4ProjectManager::Constants::QT_SETTINGS_CATEGORY,
                    Qt4ProjectManager::Constants::QTVERSION_SETTINGS_PAGE_ID);
    }
}

} // namespace Internal
} // namespace QmlProjectManager

