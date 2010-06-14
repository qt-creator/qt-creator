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

#include "debuggerrunner.h"
#include "debuggermanager.h"
#include "debuggeroutputwindow.h"

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/applicationrunconfiguration.h> // For LocalApplication*

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QtGui/QTextDocument>

using namespace ProjectExplorer;
using namespace Debugger::Internal;


namespace Debugger {

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

// A factory to create DebuggerRunControls
DebuggerRunControlFactory::DebuggerRunControlFactory(DebuggerManager *manager)
    : m_manager(manager)
{}

bool DebuggerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
//    return mode == ProjectExplorer::Constants::DEBUGMODE;
    return mode == ProjectExplorer::Constants::DEBUGMODE
            && qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
}

QString DebuggerRunControlFactory::displayName() const
{
    return tr("Debug");
}

static DebuggerStartParameters localStartParameters(RunConfiguration *runConfiguration)
{
    DebuggerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    LocalApplicationRunConfiguration *rc =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.startMode = StartInternal;
    sp.executable = rc->executable();
    sp.environment = rc->environment().toStringList();
    sp.workingDirectory = rc->workingDirectory();
    sp.processArgs = rc->commandLineArguments();
    sp.toolChainType = rc->toolChainType();
    sp.useTerminal = rc->runMode() == LocalApplicationRunConfiguration::Console;
    sp.dumperLibrary = rc->dumperLibrary();
    sp.dumperLibraryLocations = rc->dumperLibraryLocations();
    sp.displayName = rc->displayName();

    // Find qtInstallPath.
    QString qmakePath = DebuggingHelperLibrary::findSystemQt(rc->environment());
    if (!qmakePath.isEmpty()) {
        QProcess proc;
        QStringList args;
        args.append(QLatin1String("-query"));
        args.append(QLatin1String("QT_INSTALL_HEADERS"));
        proc.start(qmakePath, args);
        proc.waitForFinished();
        QByteArray ba = proc.readAllStandardOutput().trimmed();
        QFileInfo fi(QString::fromLocal8Bit(ba) + "/..");
        sp.qtInstallPath = fi.absoluteFilePath();
    }
    return sp;
}

RunControl *DebuggerRunControlFactory::create(RunConfiguration *runConfiguration,
                                              const QString &mode)
{
    QTC_ASSERT(mode == ProjectExplorer::Constants::DEBUGMODE, return 0);
    DebuggerStartParameters sp = localStartParameters(runConfiguration);
    return new DebuggerRunControl(m_manager, sp);
}

RunControl *DebuggerRunControlFactory::create(const DebuggerStartParameters &sp)
{
    return new DebuggerRunControl(m_manager, sp);
}

QWidget *DebuggerRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    // NBS TODO: Add GDB-specific configuration widget
    Q_UNUSED(runConfiguration)
    return 0;
}



////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControl
//
////////////////////////////////////////////////////////////////////////

DebuggerRunControl::DebuggerRunControl(DebuggerManager *manager,
        const DebuggerStartParameters &startParameters)
    : RunControl(0, ProjectExplorer::Constants::DEBUGMODE),
      m_startParameters(startParameters),
      m_manager(manager),
      m_running(false)
{
    connect(m_manager, SIGNAL(debuggingFinished()),
            this, SLOT(debuggingFinished()),
            Qt::QueuedConnection);
    connect(m_manager, SIGNAL(messageAvailable(QString, bool)),
            this, SLOT(slotMessageAvailable(QString, bool)));
    connect(m_manager, SIGNAL(inferiorPidChanged(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(stopRequested()),
            m_manager, SLOT(exitDebugger()));

    if (m_startParameters.environment.empty())
        m_startParameters.environment = ProjectExplorer::Environment().toStringList();
    m_startParameters.useTerminal = false;

}

QString DebuggerRunControl::displayName() const
{
    return m_startParameters.displayName;
}

void DebuggerRunControl::setCustomEnvironment(ProjectExplorer::Environment env)
{
    m_startParameters.environment = env.toStringList();
}

void DebuggerRunControl::init()
{
}

void DebuggerRunControl::start()
{
    m_running = true;
    QString errorMessage;
    QString settingsCategory;
    QString settingsPage;
    if (m_manager->checkDebugConfiguration(m_startParameters.toolChainType,
            &errorMessage, &settingsCategory, &settingsPage)) {
        m_manager->startNewDebugger(this);
        emit started();
    } else {
        appendMessage(this, errorMessage, true);
        emit finished();
        Core::ICore::instance()->showWarningWithOptions(tr("Debugger"),
            errorMessage, QString(), settingsCategory, settingsPage);
    }
}

void DebuggerRunControl::showMessage(const QString &msg, int channel,
    int timeout)
{
    if (!m_manager)
        return;
    DebuggerOutputWindow *ow = m_manager->debuggerOutputWindow();
    QTC_ASSERT(ow, return);
    switch (channel) {
        case StatusBar:
            m_manager->showStatusMessage(msg, timeout);
            ow->showOutput(LogStatus, msg);
            break;
        case AppOutput:
            emit addToOutputWindowInline(this, msg, false);
            break;
        case AppError:
            emit addToOutputWindowInline(this, msg, true);
            break;
        case LogMiscInput:
            ow->showInput(LogMisc, msg);
            ow->showOutput(LogMisc, msg);
            break;
        case LogInput:
            ow->showInput(channel, msg);
            ow->showOutput(channel, msg);
            break;
        default:
            ow->showOutput(channel, msg);
            break;
    }
}

void DebuggerRunControl::slotMessageAvailable(const QString &data, bool isError)
{
    emit appendMessage(this, data, isError);
}


void DebuggerRunControl::stop()
{
    m_running = false;
    emit stopRequested();
}

void DebuggerRunControl::debuggingFinished()
{
    m_running = false;
    emit finished();
}

bool DebuggerRunControl::isRunning() const
{
    return m_running;
}

} // namespace Debugger
