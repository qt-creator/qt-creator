/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerrunner.h"
#include "debuggerruncontrolfactory.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerkitinformation.h"
#include "debuggerplugin.h"
#include "debuggerrunconfigurationaspect.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"
#include "breakhandler.h"
#include "shared/peutils.h"

#include <projectexplorer/localapplicationrunconfiguration.h> // For LocalApplication*
#include <projectexplorer/environmentaspect.h> // For the environment
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <coreplugin/icore.h>

#include <QTcpServer>

using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace Utils;

enum { debug = 0 };

namespace Debugger {
namespace Internal {

DebuggerEngine *createCdbEngine(const DebuggerStartParameters &sp, QString *error);
DebuggerEngine *createGdbEngine(const DebuggerStartParameters &sp);
DebuggerEngine *createPdbEngine(const DebuggerStartParameters &sp);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &sp);
DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &sp, QString *error);
DebuggerEngine *createLldbEngine(const DebuggerStartParameters &sp);

static const char *engineTypeName(DebuggerEngineType et)
{
    switch (et) {
    case Debugger::NoEngineType:
        break;
    case Debugger::GdbEngineType:
        return "Gdb engine";
    case Debugger::CdbEngineType:
        return "Cdb engine";
    case Debugger::PdbEngineType:
        return "Pdb engine";
    case Debugger::QmlEngineType:
        return "QML engine";
    case Debugger::QmlCppEngineType:
        return "QML C++ engine";
    case Debugger::LldbEngineType:
        return "LLDB command line engine";
    case Debugger::AllEngineTypes:
        break;
    }
    return "No engine";
}

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlPrivate
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunControlPrivate
{
public:
    explicit DebuggerRunControlPrivate(RunConfiguration *runConfiguration);

public:
    DebuggerEngine *m_engine;
    const QPointer<RunConfiguration> m_myRunConfiguration;
    bool m_running;
};

DebuggerRunControlPrivate::DebuggerRunControlPrivate(RunConfiguration *runConfiguration)
    : m_engine(0)
    , m_myRunConfiguration(runConfiguration)
    , m_running(false)
{
}

} // namespace Internal

DebuggerRunControl::DebuggerRunControl(RunConfiguration *runConfiguration,
                                       const DebuggerStartParameters &sp)
    : RunControl(runConfiguration, DebugRunMode),
      d(new DebuggerRunControlPrivate(runConfiguration))
{
    setIcon(QLatin1String(ProjectExplorer::Constants::ICON_DEBUG_SMALL));

    connect(this, SIGNAL(finished()), SLOT(handleFinished()));
    // Create the engine. Could arguably be moved to the factory, but
    // we still have a derived S60DebugControl. Should rarely fail, though.
    QString errorMessage;
    d->m_engine = DebuggerRunControlFactory::createEngine(sp.masterEngineType, sp, &errorMessage);

    if (!d->m_engine) {
        debuggingFinished();
        Core::ICore::showWarningWithOptions(DebuggerRunControl::tr("Debugger"), errorMessage);
    }
}

DebuggerRunControl::~DebuggerRunControl()
{
    disconnect();
    if (DebuggerEngine *engine = d->m_engine) {
        d->m_engine = 0;
        engine->disconnect();
        delete engine;
    }
    delete d;
}

QString DebuggerRunControl::displayName() const
{
    QTC_ASSERT(d->m_engine, return QString());
    return d->m_engine->startParameters().displayName;
}

void DebuggerRunControl::start()
{
    QTC_ASSERT(d->m_engine, return);
    // User canceled input dialog asking for executable when working on library project.
    if (d->m_engine->startParameters().startMode == StartInternal
        && d->m_engine->startParameters().executable.isEmpty()) {
        appendMessage(tr("No executable specified.") + QLatin1Char('\n'), ErrorMessageFormat);
        emit started();
        emit finished();
        return;
    }

    if (d->m_engine->startParameters().startMode == StartInternal) {
        QStringList unhandledIds;
        foreach (const BreakpointModelId &id, debuggerCore()->breakHandler()->allBreakpointIds()) {
            if (d->m_engine->breakHandler()->breakpointData(id).enabled
                    && !d->m_engine->acceptsBreakpoint(id))
                unhandledIds.append(id.toString());
        }
        if (!unhandledIds.isEmpty()) {
            QString warningMessage =
                    DebuggerPlugin::tr("Some breakpoints cannot be handled by the debugger "
                                       "languages currently active, and will be ignored.\n"
                                       "Affected are breakpoints %1")
                    .arg(unhandledIds.join(QLatin1String(", ")));

            debuggerCore()->showMessage(warningMessage, LogWarning);

            static bool checked = true;
            if (checked)
                CheckableMessageBox::information(Core::ICore::mainWindow(),
                                                 tr("Debugger"),
                                                 warningMessage,
                                                 tr("&Show this message again."),
                                                 &checked, QDialogButtonBox::Ok);
        }
    }

    debuggerCore()->runControlStarted(d->m_engine);

    // We might get a synchronous startFailed() notification on Windows,
    // when launching the process fails. Emit a proper finished() sequence.
    emit started();
    d->m_running = true;

    d->m_engine->startDebugger(this);

    if (d->m_running)
        appendMessage(tr("Debugging starts") + QLatin1Char('\n'), NormalMessageFormat);
}

void DebuggerRunControl::startFailed()
{
    appendMessage(tr("Debugging has failed") + QLatin1Char('\n'), NormalMessageFormat);
    d->m_running = false;
    emit finished();
    d->m_engine->handleStartFailed();
}

void DebuggerRunControl::handleFinished()
{
    appendMessage(tr("Debugging has finished") + QLatin1Char('\n'), NormalMessageFormat);
    if (d->m_engine)
        d->m_engine->handleFinished();
    debuggerCore()->runControlFinished(d->m_engine);
}

void DebuggerRunControl::showMessage(const QString &msg, int channel)
{
    switch (channel) {
        case AppOutput:
            appendMessage(msg, StdOutFormatSameLine);
            break;
        case AppError:
            appendMessage(msg, StdErrFormatSameLine);
            break;
        case AppStuff:
            appendMessage(msg, DebugFormat);
            break;
    }
}

bool DebuggerRunControl::promptToStop(bool *optionalPrompt) const
{
    QTC_ASSERT(isRunning(), return true);

    if (optionalPrompt && !*optionalPrompt)
        return true;

    const QString question = tr("A debugging session is still in progress. "
            "Terminating the session in the current"
            " state can leave the target in an inconsistent state."
            " Would you still like to terminate it?");
    return showPromptToStopDialog(tr("Close Debugging Session"), question,
                                  QString(), QString(), optionalPrompt);
}

RunControl::StopResult DebuggerRunControl::stop()
{
    QTC_ASSERT(d->m_engine, return StoppedSynchronously);
    d->m_engine->quitDebugger();
    return AsynchronousStop;
}

void DebuggerRunControl::debuggingFinished()
{
    d->m_running = false;
    emit finished();
}

bool DebuggerRunControl::isRunning() const
{
    return d->m_running;
}

DebuggerEngine *DebuggerRunControl::engine()
{
    QTC_CHECK(d->m_engine);
    return d->m_engine;
}

RunConfiguration *DebuggerRunControl::runConfiguration() const
{
    return d->m_myRunConfiguration.data();
}


////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

DebuggerRunControlFactory::DebuggerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{}

bool DebuggerRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    return (mode == DebugRunMode || mode == DebugRunModeWithBreakOnMain)
            && qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
}

static DebuggerStartParameters localStartParameters(RunConfiguration *runConfiguration, QString *errorMessage)
{
    DebuggerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    LocalApplicationRunConfiguration *rc =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);
    EnvironmentAspect *environment = rc->extraAspect<ProjectExplorer::EnvironmentAspect>();
    QTC_ASSERT(environment, return sp);

    Target *target = runConfiguration->target();
    Kit *kit = target ? target->kit() : KitManager::defaultKit();
    if (!fillParameters(&sp, kit, errorMessage))
        return sp;
    sp.environment = environment->environment();

    // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
    sp.workingDirectory = FileUtils::normalizePathName(rc->workingDirectory());

    sp.executable = rc->executable();
    if (sp.executable.isEmpty())
        return sp;

    sp.processArgs = rc->commandLineArguments();
    sp.useTerminal = rc->runMode() == ApplicationLauncher::Console;

    if (target) {
        if (const Project *project = target->project()) {
            sp.projectSourceDirectory = project->projectDirectory().toString();
            if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
                sp.projectBuildDirectory = buildConfig->buildDirectory().toString();
            sp.projectSourceFiles = project->files(Project::ExcludeGeneratedFiles);
        }
    }

    DebuggerRunConfigurationAspect *debugger
            = runConfiguration->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    sp.multiProcess = debugger->useMultiProcess();

    if (debugger->useCppDebugger())
        sp.languages |= CppLanguage;

    if (debugger->useQmlDebugger()) {
        const ProjectExplorer::IDevice::ConstPtr device =
                DeviceKitInformation::device(runConfiguration->target()->kit());
        QTC_ASSERT(device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE, return sp);
        QTcpServer server;
        const bool canListen = server.listen(QHostAddress::LocalHost)
                || server.listen(QHostAddress::LocalHostIPv6);
        if (!canListen) {
            if (errorMessage)
                *errorMessage = DebuggerPlugin::tr("Not enough free ports for QML debugging.") + QLatin1Char(' ');
            return sp;
        }
        sp.qmlServerAddress = server.serverAddress().toString();
        sp.qmlServerPort = server.serverPort();
        sp.languages |= QmlLanguage;

        // Makes sure that all bindings go through the JavaScript engine, so that
        // breakpoints are actually hit!
        const QString optimizerKey = _("QML_DISABLE_OPTIMIZER");
        if (!sp.environment.hasKey(optimizerKey))
            sp.environment.set(optimizerKey, _("1"));

        QtcProcess::addArg(&sp.processArgs, QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(sp.qmlServerPort));
    }

    sp.startMode = StartInternal;
    sp.displayName = rc->displayName();

    return sp;
}

RunControl *DebuggerRunControlFactory::create
    (RunConfiguration *runConfiguration, RunMode mode, QString *errorMessage)
{
    QTC_ASSERT(mode == DebugRunMode || mode == DebugRunModeWithBreakOnMain, return 0);
    DebuggerStartParameters sp = localStartParameters(runConfiguration, errorMessage);
    if (sp.startMode == NoStartMode)
        return 0;
    if (mode == DebugRunModeWithBreakOnMain)
        sp.breakOnMain = true;

    return doCreate(sp, runConfiguration, errorMessage);
}

static bool fixupEngineTypes(DebuggerStartParameters &sp, RunConfiguration *rc, QString *errorMessage)
{
    if (sp.masterEngineType != NoEngineType)
        return true;

    if (sp.executable.endsWith(_(".py"))) {
        sp.masterEngineType = PdbEngineType;
        return true;
    }

    if (rc) {
        DebuggerRunConfigurationAspect *aspect
                = rc->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
        if (const Target *target = rc->target())
            if (!fillParameters(&sp, target->kit(), errorMessage))
                return false;
        const bool useCppDebugger = aspect->useCppDebugger() && (sp.languages & CppLanguage);
        const bool useQmlDebugger = aspect->useQmlDebugger() && (sp.languages & QmlLanguage);
        if (useQmlDebugger) {
            if (useCppDebugger) {
                sp.masterEngineType = QmlCppEngineType;
                sp.firstSlaveEngineType = sp.cppEngineType;
                sp.secondSlaveEngineType = QmlCppEngineType;
            } else {
                sp.masterEngineType = QmlEngineType;
            }
        } else {
            sp.masterEngineType = sp.cppEngineType;
        }
        return true;
    }
    sp.masterEngineType = sp.cppEngineType;
    return true;
}

DebuggerRunControl *DebuggerRunControlFactory::doCreate
    (const DebuggerStartParameters &sp0, RunConfiguration *rc, QString *errorMessage)
{
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

    DebuggerStartParameters sp = sp0;
    if (!debuggerCore()->boolSetting(AutoEnrichParameters)) {
        const QString sysroot = sp.sysRoot;
        if (sp.debugInfoLocation.isEmpty())
            sp.debugInfoLocation = sysroot + QLatin1String("/usr/lib/debug");
        if (sp.debugSourceLocation.isEmpty()) {
            QString base = sysroot + QLatin1String("/usr/src/debug/");
            sp.debugSourceLocation.append(base + QLatin1String("qt5base/src/corelib"));
            sp.debugSourceLocation.append(base + QLatin1String("qt5base/src/gui"));
            sp.debugSourceLocation.append(base + QLatin1String("qt5base/src/network"));
        }
    }

    if (!fixupEngineTypes(sp, rc, errorMessage))
        return 0;

    return new DebuggerRunControl(rc, sp);
}

IRunConfigurationAspect *DebuggerRunControlFactory::createRunConfigurationAspect(RunConfiguration *rc)
{
    return new DebuggerRunConfigurationAspect(rc);
}

DebuggerRunControl *DebuggerRunControlFactory::createAndScheduleRun(const DebuggerStartParameters &sp)
{
    QString errorMessage;
    DebuggerRunControl *rc = doCreate(sp, 0, &errorMessage);
    if (!rc) {
        ProjectExplorer::ProjectExplorerPlugin::showRunErrorMessage(errorMessage);
        return 0;
    }
    debuggerCore()->showMessage(sp.startMessage, 0);
    ProjectExplorerPlugin::instance()->startRunControl(rc, DebugRunMode);
    return rc;
}

DebuggerEngine *DebuggerRunControlFactory::createEngine(DebuggerEngineType et,
    const DebuggerStartParameters &sp, QString *errorMessage)
{
    switch (et) {
    case GdbEngineType:
        return createGdbEngine(sp);
    case CdbEngineType:
        return createCdbEngine(sp, errorMessage);
    case PdbEngineType:
        return createPdbEngine(sp);
    case QmlEngineType:
        return createQmlEngine(sp);
    case LldbEngineType:
        return createLldbEngine(sp);
    case QmlCppEngineType:
        return createQmlCppEngine(sp, errorMessage);
    default:
        break;
    }
    *errorMessage = DebuggerPlugin::tr("Unable to create a debugger engine of the type \"%1\"").
                    arg(_(engineTypeName(et)));
    return 0;
}

} // namespace Debugger
