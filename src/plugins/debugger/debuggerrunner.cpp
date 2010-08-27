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
#include "debuggeruiswitcher.h"

#ifdef Q_OS_WIN
#  include "peutils.h"
#endif

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/applicationrunconfiguration.h> // For LocalApplication*

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QStringList>

#include <QtGui/QAbstractItemView>
#include <QtGui/QTextDocument>
#include <QtGui/QTreeWidget>
#include <QtGui/QMessageBox>

using namespace ProjectExplorer;
using namespace Debugger::Internal;

namespace Debugger {
namespace Internal {

DebuggerEngine *createGdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createScriptEngine(const DebuggerStartParameters &);
DebuggerEngine *createPdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createTcfEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &);

bool checkGdbConfiguration(int toolChain, QString *errorMsg, QString *settingsPage);

// FIXME: Outdated?
// The createCdbEngine function takes a list of options pages it can add to.
// This allows for having a "enabled" toggle on the page independently
// of the engine. That's good for not enabling the related ActiveX control
// unnecessarily.

#ifdef CDB_ENABLED
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &);
bool checkCdbConfiguration(int toolChain, QString *errorMsg, QString *settingsPage);
#else
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &) { return 0; }
bool checkCdbConfiguration(int, QString *, QString *) { return false; }
#endif

} // namespace Internal

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

    DebuggerLanguages activeLangs = DebuggerUISwitcher::instance()->activeDebugLanguages();
    if (activeLangs & QmlLanguage) {
        sp.qmlServerAddress = QLatin1String("127.0.0.1");
        sp.qmlServerPort = rc->environment().value("QML_DEBUG_SERVER_PORT").toUInt();
        if (sp.qmlServerPort == 0)
            sp.qmlServerPort = Constants::QML_DEFAULT_DEBUG_SERVER_PORT;

        sp.environment << QString(Constants::E_QML_DEBUG_SERVER_PORT)
                        + QLatin1Char('=') + QString::number(sp.qmlServerPort);
    }

    // FIXME: If it's not yet build this will be empty and not filled
    // when rebuild as the runConfiguration is not stored and therefore
    // cannot be used to retrieve the dumper location.
    //qDebug() << "DUMPER: " << sp.dumperLibrary << sp.dumperLibraryLocations;
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
    return create(sp, runConfiguration);
}

DebuggerRunControl *DebuggerRunControlFactory::create(
    const DebuggerStartParameters &sp,
    RunConfiguration *runConfiguration)
{
    DebuggerRunControl *runControl =
        new DebuggerRunControl(runConfiguration, m_enabledEngines, sp);
    if (!runControl->engine()) {
        qDebug() << "FAILED TO CREATE ENGINE";
        delete runControl;
        return 0;
    }
    return runControl;
}

QWidget *DebuggerRunControlFactory::createConfigurationWidget
    (RunConfiguration *runConfiguration)
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

DebuggerRunControl::DebuggerRunControl(RunConfiguration *runConfiguration,
        DebuggerEngineType enabledEngines, const DebuggerStartParameters &sp)
    : RunControl(runConfiguration, ProjectExplorer::Constants::DEBUGMODE)
    , m_myRunConfiguration(runConfiguration)
    , m_running(false)
    , m_started(false)
    , m_enabledEngines(enabledEngines)

{
    connect(this, SIGNAL(finished()), this, SLOT(handleFinished()));
    DebuggerStartParameters startParams = sp;
    startParams.m_isQmlProject = isQmlProject(runConfiguration);
    createEngine(startParams);
}

DebuggerRunControl::~DebuggerRunControl()
{
    disconnect();
    DebuggerEngine *engine = m_engine;
    m_engine = 0;
    engine->disconnect();
    delete engine;
}

const DebuggerStartParameters &DebuggerRunControl::startParameters() const
{
    QTC_ASSERT(m_engine, return *(new DebuggerStartParameters()));
    return m_engine->startParameters();
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
        return GdbEngineType;

    // If a file has PDB files, it has been compiled by VS.
    QStringList pdbFiles;
    if (!getPDBFiles(executable, &pdbFiles, &m_errorMessage)) {
        qWarning("Cannot determine type of executable %s: %s",
                 qPrintable(executable), qPrintable(m_errorMessage));
        return NoEngineType;
    }
    if (pdbFiles.empty())
        return GdbEngineType;

    // We need the CDB debugger in order to be able to debug VS
    // executables
    if (checkDebugConfiguration(ToolChain::MSVC, &m_errorMessage, 0, &m_settingsIdHint))
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
    if (startMode != AttachToRemote)
        return CdbEngineType;
    return GdbEngineType;
    m_errorMessage = msgEngineNotAvailable("Gdb Engine");
    return NoEngineType;
#else
    Q_UNUSED(startMode)
    //  m_errorMessage = msgEngineNotAvailable("Gdb Engine");
    return GdbEngineType;
#endif
}

void DebuggerRunControl::createEngine(const DebuggerStartParameters &startParams)
{
    DebuggerStartParameters sp = startParams;

    // Figure out engine according to toolchain, executable, attach or default.
    DebuggerEngineType engineType = NoEngineType;
    DebuggerLanguages activeLangs = DebuggerPlugin::instance()->activeLanguages();
    bool isQmlExecutable = sp.executable.endsWith(_("qmlviewer")) || sp.executable.endsWith(_("qmlobserver"));
#ifdef Q_OS_MAC
    isQmlExecutable = sp.executable.endsWith(_("QMLViewer.app")) || sp.executable.endsWith(_("QMLObserver.app"));
#endif
    if (isQmlExecutable)
        engineType = QmlEngineType;
    else if (sp.executable.endsWith(_(".js")))
        engineType = ScriptEngineType;
    else if (sp.executable.endsWith(_(".py")))
        engineType = PdbEngineType;
    else
        engineType = engineForToolChain(sp.toolChainType);

    // Fixme: 1 of 3 testing hacks.
    if (sp.processArgs.size() >= 5 && sp.processArgs.at(0) == _("@tcf@"))
        engineType = GdbEngineType;

    if (engineType == NoEngineType
            && sp.startMode != AttachToRemote
            && !sp.executable.isEmpty())
        engineType = engineForExecutable(sp.executable);

    if (!engineType)
        engineType = engineForMode(sp.startMode);

    if (engineType != QmlEngineType && (activeLangs & QmlLanguage)) {
        if (activeLangs & CppLanguage) {
            sp.cppEngineType = engineType;
            engineType = QmlCppEngineType;
        } else {
            engineType = QmlEngineType;
        }
    }

    // qDebug() << "USING ENGINE : " << engineType;

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
        case QmlCppEngineType:
            m_engine = createQmlCppEngine(sp);
            break;
        default: {
            // Could not find anything suitable.
            debuggingFinished();
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

QString DebuggerRunControl::displayName() const
{
    QTC_ASSERT(m_engine, return QString());
    return m_engine->startParameters().displayName;
}

void DebuggerRunControl::setCustomEnvironment(ProjectExplorer::Environment env)
{
    m_engine->startParameters().environment = env.toStringList();
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

    if (!checkDebugConfiguration(sp.toolChainType,
            &errorMessage, &settingsCategory, &settingsPage)) {
        emit appendMessage(this, errorMessage, true);
        emit finished();
        Core::ICore::instance()->showWarningWithOptions(tr("Debugger"),
            errorMessage, QString(), settingsCategory, settingsPage);
        return;
    }

    DebuggerUISwitcher::instance()->aboutToStartDebugger();

    const QString message = tr("Starting debugger '%1' for tool chain '%2'...").
                  arg(m_engine->objectName(), toolChainName(sp.toolChainType));
    plugin()->showMessage(message, StatusBar);
    plugin()->showMessage(DebuggerSettings::instance()->dump(), LogDebug);
    plugin()->runControlStarted(this);

    plugin()->activateDebugMode();

    engine()->startDebugger(this);
    m_running = true;
    emit addToOutputWindowInline(this, tr("Debugging starts"), false);
    emit addToOutputWindowInline(this, "\n", false);
    emit started();
}

void DebuggerRunControl::startFailed()
{
    emit addToOutputWindowInline(this, tr("Debugging has failed"), false);
    m_running = false;
    emit finished();
    engine()->handleStartFailed();
}

void DebuggerRunControl::handleFinished()
{
    emit addToOutputWindowInline(this, tr("Debugging has finished"), false);
    engine()->handleFinished();
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

bool DebuggerRunControl::aboutToStop() const
{
    QTC_ASSERT(isRunning(), return true;)

    const QString question = tr("A debugging session are still in progress. "
            "Terminating the session in the current"
            " state can leave the target in an inconsistent state."
            " Would you still like to terminate it?");

    const QMessageBox::StandardButton answer =
            QMessageBox::question(DebuggerUISwitcher::instance()->mainWindow(),
                                  tr("Close Debugging Session"), question,
                                  QMessageBox::Yes|QMessageBox::No);
    return answer == QMessageBox::Yes;
}

RunControl::StopResult DebuggerRunControl::stop()
{
    QTC_ASSERT(m_engine, return StoppedSynchronously);
    m_engine->quitDebugger();
    return AsynchronousStop;
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

DebuggerState DebuggerRunControl::state() const
{
    QTC_ASSERT(m_engine, return DebuggerNotReady);
    return m_engine->state();
}

Internal::DebuggerEngine *DebuggerRunControl::engine()
{
    QTC_ASSERT(m_engine, /**/);
    return m_engine;
}

bool DebuggerRunControl::isQmlProject(RunConfiguration *config)
{
    if (!config || !config->target() || !config->target()->project())
        return false;

    QStringList projectFiles = config->target()->project()->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
    foreach(const QString &filename, projectFiles) {
        if (filename.endsWith(".qml"))
            return true;
    }

    return false;
}

} // namespace Debugger
