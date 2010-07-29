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

#include "qmloutputformatter.h"
#include "qmlprojectruncontrol.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojectconstants.h"
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/applicationlauncher.h>
#include <utils/qtcassert.h>

#include <debugger/debuggerconstants.h>
#include <debugger/debuggeruiswitcher.h>
#include <qmlinspector/qmlinspectorconstants.h>

#include <QDir>
#include <QLabel>

using ProjectExplorer::RunConfiguration;
using ProjectExplorer::RunControl;

namespace QmlProjectManager {
namespace Internal {

QmlRunControl::QmlRunControl(QmlProjectRunConfiguration *runConfiguration, bool debugMode)
    : RunControl(runConfiguration), m_debugMode(debugMode)
{
    ProjectExplorer::Environment environment = ProjectExplorer::Environment::systemEnvironment();
    if (debugMode)
        environment.set(QmlProjectManager::Constants::E_QML_DEBUG_SERVER_PORT, QString::number(runConfiguration->debugServerPort()));

    m_applicationLauncher.setEnvironment(environment.toStringList());
    m_applicationLauncher.setWorkingDirectory(runConfiguration->workingDirectory());

    m_executable = runConfiguration->viewerPath();
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
}

void QmlRunControl::start()
{
    m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui, m_executable,
                                m_commandLineArguments);

    // FIXME this line should be refactored out in order to remove the dependency between
    // debugger and qmlprojectmanager, because debugger also relies on cpptools.
    Debugger::DebuggerUISwitcher::instance()->setActiveLanguage(Qml::Constants::LANG_QML);

    emit started();
    emit appendMessage(this, tr("Starting %1 %2").arg(QDir::toNativeSeparators(m_executable),
                                                      m_commandLineArguments.join(QLatin1String(" "))), false);
}

void QmlRunControl::stop()
{
    m_applicationLauncher.stop();
}

bool QmlRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

ProjectExplorer::OutputFormatter *QmlRunControl::createOutputFormatter(QObject *parent)
{
    return new QmlOutputFormatter(parent);
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
    if (m_debugMode && line.startsWith("QDeclarativeDebugServer: Waiting for connection")) {
        Core::ICore *core = Core::ICore::instance();
        core->modeManager()->activateMode(Debugger::Constants::MODE_DEBUG);
    }

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
    QmlProjectRunConfiguration *config = qobject_cast<QmlProjectRunConfiguration*>(runConfiguration);
    if (mode == ProjectExplorer::Constants::RUNMODE) {
        return config != 0;
    } else {
        return (config != 0) && Debugger::DebuggerUISwitcher::instance()->supportedLanguages().contains(Qml::Constants::LANG_QML);
    }
}

RunControl *QmlRunControlFactory::create(RunConfiguration *runConfiguration,
                                         const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    return new QmlRunControl(qobject_cast<QmlProjectRunConfiguration *>(runConfiguration),
                             mode == ProjectExplorer::Constants::DEBUGMODE);
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

} // namespace Internal
} // namespace QmlProjectManager

