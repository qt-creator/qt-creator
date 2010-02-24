/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
        environment.set("QML_DEBUG_SERVER_PORT", QString::number(runConfiguration->debugServerPort()));

    m_applicationLauncher.setEnvironment(environment.toStringList());
    m_applicationLauncher.setWorkingDirectory(runConfiguration->workingDirectory());

    m_executable = runConfiguration->viewerPath();
    m_commandLineArguments = runConfiguration->viewerArguments();

    connect(&m_applicationLauncher, SIGNAL(applicationError(QString)),
            this, SLOT(slotError(QString)));
    connect(&m_applicationLauncher, SIGNAL(appendOutput(QString)),
            this, SLOT(slotAddToOutputWindow(QString)));
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

    Debugger::DebuggerUISwitcher::instance()->setActiveLanguage(Qml::Constants::LANG_QML);

    emit started();
    emit addToOutputWindow(this, tr("Starting %1 %2").arg(QDir::toNativeSeparators(m_executable),
                           m_commandLineArguments.join(QLatin1String(" "))));
}

void QmlRunControl::stop()
{
    m_applicationLauncher.stop();
}

bool QmlRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

void QmlRunControl::slotBringApplicationToForeground(qint64 pid)
{
    bringApplicationToForeground(pid);
}

void QmlRunControl::slotError(const QString &err)
{
    emit error(this, err);
    emit finished();
}

void QmlRunControl::slotAddToOutputWindow(const QString &line)
{
    if (m_debugMode && line.startsWith("QmlDebugServer: Waiting for connection")) {
        Core::ICore *core = Core::ICore::instance();
        core->modeManager()->activateMode(Debugger::Constants::MODE_DEBUG);
    }

    emit addToOutputWindowInline(this, line);
}

void QmlRunControl::processExited(int exitCode)
{
    emit addToOutputWindow(this, tr("%1 exited with code %2").arg(QDir::toNativeSeparators(m_executable)).arg(exitCode));
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
    Q_UNUSED(mode);
    return (qobject_cast<QmlProjectRunConfiguration*>(runConfiguration) != 0);
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

QWidget *QmlRunControlFactory::configurationWidget(RunConfiguration *runConfiguration)
{
    Q_UNUSED(runConfiguration)
    return new QLabel("TODO add Configuration widget");
}

} // namespace Internal
} // namespace QmlProjectManager

