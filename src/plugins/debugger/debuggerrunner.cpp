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

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"

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
#include <QtCore/QTimer>

#include <QtGui/QAbstractItemView>
#include <QtGui/QTextDocument>
#include <QtGui/QTreeWidget>

using namespace ProjectExplorer;
using namespace Debugger::Internal;

namespace Debugger {
namespace Internal {

DebuggerEngine *createGdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createScriptEngine(const DebuggerStartParameters &);
DebuggerEngine *createPdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createTcfEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &);

#ifdef CDB_ENABLED
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &);
#else
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &) { return 0; }
#endif

// FIXME: Outdated?
// The createCdbEngine function takes a list of options pages it can add to.
// This allows for having a "enabled" toggle on the page independently
// of the engine. That's good for not enabling the related ActiveX control
// unnecessarily.

bool checkGdbConfiguration(int toolChain, QString *errorMsg, QString *settingsPage);
bool checkCdbConfiguration(int toolChain, QString *errorMsg, QString *settingsPage);

}


static QString toolChainName(int toolChainType)
{
    return ToolChain::toolChainName(ToolChain::ToolChainType(toolChainType));
}


////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

static QString msgEngineNotAvailable(const char *engine)
{
    return DebuggerPlugin::tr("The application requires the debugger engine '%1', "
        "which is disabled.").arg(QLatin1String(engine));
}

static DebuggerPlugin *plugin() { return DebuggerPlugin::instance(); }

// A factory to create DebuggerRunControls
DebuggerRunControlFactory::DebuggerRunControlFactory(QObject *parent,
        DebuggerEngineType enabledEngines)
    : IRunControlFactory(parent), m_enabledEngines(enabledEngines)
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

RunControl *DebuggerRunControlFactory::create
    (RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(mode == ProjectExplorer::Constants::DEBUGMODE, return 0);
    DebuggerStartParameters sp = localStartParameters(runConfiguration);
    return create(sp);
}

RunControl *DebuggerRunControlFactory::create(const DebuggerStartParameters &sp)
{
    DebuggerRunControl *runControl = new DebuggerRunControl;
    runControl->setEnabledEngines(m_enabledEngines);
    runControl->createEngine(sp);
    if (!runControl->engine()) {
        qDebug() << "FAILED TO CREATE ENGINE";
        delete runControl;
        return 0;
    }
    connect(runControl, SIGNAL(started()), runControl, SLOT(handleStarted()));
    connect(runControl, SIGNAL(finished()), runControl, SLOT(handleFinished()));
    return runControl;
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

DebuggerRunControl::DebuggerRunControl(QObject *parent)
    : RunControl(0, ProjectExplorer::Constants::DEBUGMODE)
{
    Q_UNUSED(parent);
    m_running = false;
    m_engine = 0;
    m_enabledEngines = AllEngineTypes;
}

static DebuggerEngineType engineForToolChain(int toolChainType)
{
    switch (toolChainType) {
        case ProjectExplorer::ToolChain::LINUX_ICC:
        case ProjectExplorer::ToolChain::MinGW:
        case ProjectExplorer::ToolChain::GCC:
        case ProjectExplorer::ToolChain::WINSCW: // S60
        case ProjectExplorer::ToolChain::GCCE:
        case ProjectExplorer::ToolChain::RVCT_ARMV5:
        case ProjectExplorer::ToolChain::RVCT_ARMV6:
        case ProjectExplorer::ToolChain::RVCT_ARMV5_GNUPOC:
        case ProjectExplorer::ToolChain::GCCE_GNUPOC:
            return GdbEngineType;

        case ProjectExplorer::ToolChain::MSVC:
        case ProjectExplorer::ToolChain::WINCE:
            return CdbEngineType;

        case ProjectExplorer::ToolChain::OTHER:
        case ProjectExplorer::ToolChain::UNKNOWN:
        case ProjectExplorer::ToolChain::INVALID:
        default:
            break;
    }
    return NoEngineType;
}


// Figure out the debugger type of an executable. Analyze executable
// unless the toolchain provides a hint.
DebuggerEngineType DebuggerRunControl::engineForExecutable(const QString &executable)
{
    if (executable.endsWith(_("qmlviewer"))) {
        if (m_enabledEngines & QmlEngineType)
            return QmlEngineType;
        m_errorMessage = msgEngineNotAvailable("Qml Engine");
    }

    if (executable.endsWith(_(".js"))) {
        if (m_enabledEngines & ScriptEngineType)
            return ScriptEngineType;
        m_errorMessage = msgEngineNotAvailable("Script Engine");
    }

    if (executable.endsWith(_(".py"))) {
        if (m_enabledEngines & PdbEngineType)
            return PdbEngineType;
        m_errorMessage = msgEngineNotAvailable("Pdb Engine");
    }

#ifdef Q_OS_WIN
    // A remote executable?
    if (!executable.endsWith(_(".exe")))
        return GdbEngineType

    // If a file has PDB files, it has been compiled by VS.
    QStringList pdbFiles;
    if (!getPDBFiles(executable, &pdbFiles, errorMessage)) {
        qWarning("Cannot determine type of executable %s: %s",
                 qPrintable(executable), qPrintable(m_errorMessage));
        return 0;
    }
    if (pdbFiles.empty())
        return GdbEngineType;

    // We need the CDB debugger in order to be able to debug VS
    // executables
    if (checkDebugConfiguration(ToolChain::MSVC, errorMessage, 0, &m_settingsIdHint))
        return CdbEngineType;
#else
    if (m_enabledEngines & GdbEngineType)
        return GdbEngineType;
    m_errorMessage = msgEngineNotAvailable("Gdb Engine");
#endif

    return NoEngineType;
}

// Debugger type for mode.
DebuggerEngineType DebuggerRunControl::engineForMode(DebuggerStartMode startMode)
{
    if (startMode == AttachTcf)
        return TcfEngineType;

#ifdef Q_OS_WIN
    // Preferably Windows debugger for attaching locally.
    if (startMode != AttachToRemote && cdbEngine)
        return CdbEngineType;
    if (gdbEngine)
        return GdbEngineType;
    m_errorMessage = msgEngineNotAvailable("Gdb Engine");
    return NoEngineType;
#else
    Q_UNUSED(startMode)
    //  m_errorMessage = msgEngineNotAvailable("Gdb Engine");
    return GdbEngineType;
#endif
}

void DebuggerRunControl::setEnabledEngines(DebuggerEngineType enabledEngines)
{
    m_enabledEngines = enabledEngines;
}

void DebuggerRunControl::createEngine(const DebuggerStartParameters &sp)
{
    // Figure out engine according to toolchain, executable, attach or default.

    DebuggerEngineType engineType = NoEngineType;
    QString errorMessage;
    QString settingsIdHint;

    if (sp.executable.endsWith(_("qmlviewer")))
        engineType = QmlEngineType;
    else if (sp.executable.endsWith(_(".js")))
        engineType = ScriptEngineType;
    else if (sp.executable.endsWith(_(".py")))
        engineType = PdbEngineType;
    else
        engineType = engineForToolChain(sp.toolChainType);

    if (engineType == NoEngineType
            && sp.startMode != AttachToRemote
            && !sp.executable.isEmpty())
        engineType = engineForExecutable(sp.executable);

    if (!engineType)
        engineType = engineForMode(sp.startMode);

    //qDebug() << "USING ENGINE : " << engineType;

    switch (engineType) {
        case GdbEngineType:
            m_engine = createGdbEngine(sp);
            break;
        case ScriptEngineType:
            m_engine = createScriptEngine(sp);
            break;
        case CdbEngineType:
            m_engine = createCdbEngine(sp);
            break;
        case PdbEngineType:
            m_engine = createPdbEngine(sp);
            break;
        case TcfEngineType:
            m_engine = createTcfEngine(sp);
            break;
        case QmlEngineType:
            m_engine = createQmlEngine(sp);
            break;
        default: {
            // Could not find anything suitable.
            emit debuggingFinished();
            // Create Message box with possibility to go to settings
            const QString msg = tr("Cannot debug '%1' (tool chain: '%2'): %3")
                .arg(sp.executable, toolChainName(sp.toolChainType), m_errorMessage);
            Core::ICore::instance()->showWarningWithOptions(tr("Warning"),
                msg, QString(), QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY),
                m_settingsIdHint);
            break;
        }
    }
}

DebuggerRunControl::~DebuggerRunControl()
{
    delete m_engine;
}

QString DebuggerRunControl::displayName() const
{
    QTC_ASSERT(m_engine, return QString());
    return m_engine->startParameters().displayName;
}

void DebuggerRunControl::setCustomEnvironment(ProjectExplorer::Environment env)
{
    m_engine->startParameters().environment = env.toStringList();
}

void DebuggerRunControl::init()
{
}

bool DebuggerRunControl::checkDebugConfiguration(int toolChain,
                                              QString *errorMessage,
                                              QString *settingsCategory /* = 0 */,
                                              QString *settingsPage /* = 0 */)
{
    errorMessage->clear();
    if (settingsCategory)
        settingsCategory->clear();
    if (settingsPage)
        settingsPage->clear();

    bool success = true;

    switch(toolChain) {
    case ProjectExplorer::ToolChain::GCC:
    case ProjectExplorer::ToolChain::LINUX_ICC:
    case ProjectExplorer::ToolChain::MinGW:
    case ProjectExplorer::ToolChain::WINCE: // S60
    case ProjectExplorer::ToolChain::WINSCW:
    case ProjectExplorer::ToolChain::GCCE:
    case ProjectExplorer::ToolChain::RVCT_ARMV5:
    case ProjectExplorer::ToolChain::RVCT_ARMV6:
        success = checkGdbConfiguration(toolChain, errorMessage, settingsPage);
        if (!success)
            *errorMessage = msgEngineNotAvailable("Gdb");
        break;
    case ProjectExplorer::ToolChain::MSVC:
        success = checkCdbConfiguration(toolChain, errorMessage, settingsPage);
        if (!success) {
            *errorMessage = msgEngineNotAvailable("Cdb");
            if (settingsPage)
                *settingsPage = QLatin1String("Cdb");
        }
        break;
    }
    if (!success && settingsCategory && settingsPage && !settingsPage->isEmpty())
        *settingsCategory = QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY);

    return success;
}

void DebuggerRunControl::start()
{
    QTC_ASSERT(m_engine, return);
    const DebuggerStartParameters &sp = m_engine->startParameters();

    QString errorMessage;
    QString settingsCategory;
    QString settingsPage;
    QString settingsIdHint;

    if (!checkDebugConfiguration(sp.toolChainType,
            &errorMessage, &settingsCategory, &settingsPage)) {
        emit appendMessage(this, errorMessage, true);
        emit finished();
        Core::ICore::instance()->showWarningWithOptions(tr("Debugger"),
            errorMessage, QString(), settingsCategory, settingsPage);
        return;
    }

    plugin()->activateDebugMode();

    showMessage(tr("Starting debugger for tool chain '%1'...")
        .arg(toolChainName(sp.toolChainType)), LogStatus);
    showMessage(DebuggerSettings::instance()->dump(), LogDebug);

    plugin()->setBusyCursor(false);
    engine()->startDebugger(this);
}

void DebuggerRunControl::startSuccessful()
{
    m_running = true;
    emit started();
}

void DebuggerRunControl::startFailed()
{
    m_running = false;
    emit finished();
}

void DebuggerRunControl::handleStarted()
{
    plugin()->runControlStarted(this);
}

void DebuggerRunControl::handleFinished()
{
    plugin()->runControlFinished(this);
}

void DebuggerRunControl::showMessage(const QString &msg, int channel)
{
    switch (channel) {
        case AppOutput:
            emit addToOutputWindowInline(this, msg, false);
            break;
        case AppError:
            emit addToOutputWindowInline(this, msg, true);
            break;
        case AppStuff:
            emit appendMessage(this, msg, true);
            break;
    }
}

void DebuggerRunControl::stop()
{
    m_running = false;
    QTC_ASSERT(m_engine, return);
    m_engine->exitDebugger();
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

Internal::DebuggerEngine *DebuggerRunControl::engine()
{
    QTC_ASSERT(m_engine, /**/);
    return m_engine;
}


} // namespace Debugger
