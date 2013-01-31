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

#include "debuggerrunner.h"
#include "debuggerruncontrolfactory.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggermainwindow.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"
#include "debuggerstartparameters.h"
#include "debuggerkitinformation.h"
#include "lldb/lldbenginehost.h"
#include "debuggertooltipmanager.h"

#ifdef Q_OS_WIN
#  include "peutils.h"
#  include <utils/winutils.h>
#endif

#include <projectexplorer/abi.h>
#include <projectexplorer/localapplicationrunconfiguration.h> // For LocalApplication*
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/outputformat.h>
#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <utils/qtcprocess.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <QDir>
#include <QCheckBox>
#include <QSpinBox>
#include <QDebug>
#include <QErrorMessage>
#include <QFormLayout>
#include <QLabel>
#include <QPointer>

using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace Utils;

enum { debug = 0 };

namespace Debugger {
namespace Internal {

DebuggerEngine *createCdbEngine(const DebuggerStartParameters &sp, QString *error);
DebuggerEngine *createGdbEngine(const DebuggerStartParameters &sp);
DebuggerEngine *createScriptEngine(const DebuggerStartParameters &sp);
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
    case Debugger::ScriptEngineType:
        return "Script engine";
    case Debugger::CdbEngineType:
        return "Cdb engine";
    case Debugger::PdbEngineType:
        return "Pdb engine";
    case Debugger::QmlEngineType:
        return "QML engine";
    case Debugger::QmlCppEngineType:
        return "QML C++ engine";
    case Debugger::LldbEngineType:
        return "LLDB engine";
    case Debugger::AllEngineTypes:
        break;
    }
    return "No engine";
}

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunConfigWidget
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunConfigWidget : public RunConfigWidget
{
    Q_OBJECT

public:
    explicit DebuggerRunConfigWidget(RunConfiguration *runConfiguration);
    QString displayName() const { return tr("Debugger Settings"); }

private slots:
    void useCppDebuggerToggled(bool on);
    void useQmlDebuggerToggled(bool on);
    void qmlDebugServerPortChanged(int port);
    void useMultiProcessToggled(bool on);

public:
    DebuggerRunConfigurationAspect *m_aspect; // not owned

    QCheckBox *m_useCppDebugger;
    QCheckBox *m_useQmlDebugger;
    QSpinBox *m_debugServerPort;
    QLabel *m_debugServerPortLabel;
    QLabel *m_qmlDebuggerInfoLabel;
    QCheckBox *m_useMultiProcess;
};

DebuggerRunConfigWidget::DebuggerRunConfigWidget(RunConfiguration *runConfiguration)
{
    m_aspect = runConfiguration->debuggerAspect();

    m_useCppDebugger = new QCheckBox(tr("Enable C++"), this);
    m_useQmlDebugger = new QCheckBox(tr("Enable QML"), this);

    m_debugServerPort = new QSpinBox(this);
    m_debugServerPort->setMinimum(1);
    m_debugServerPort->setMaximum(65535);

    m_debugServerPortLabel = new QLabel(tr("Debug port:"), this);
    m_debugServerPortLabel->setBuddy(m_debugServerPort);

    m_qmlDebuggerInfoLabel = new QLabel(tr("<a href=\""
        "qthelp://com.nokia.qtcreator/doc/creator-debugging-qml.html"
        "\">What are the prerequisites?</a>"));

    m_useCppDebugger->setChecked(m_aspect->useCppDebugger());
    m_useQmlDebugger->setChecked(m_aspect->useQmlDebugger());

    m_debugServerPort->setValue(m_aspect->qmlDebugServerPort());

    static const QByteArray env = qgetenv("QTC_DEBUGGER_MULTIPROCESS");
    m_useMultiProcess =
        new QCheckBox(tr("Enable Debugging of Subprocesses"), this);
    m_useMultiProcess->setChecked(m_aspect->useMultiProcess());
    m_useMultiProcess->setVisible(env.toInt());

    connect(m_qmlDebuggerInfoLabel, SIGNAL(linkActivated(QString)),
            Core::HelpManager::instance(), SLOT(handleHelpRequest(QString)));
    connect(m_useQmlDebugger, SIGNAL(toggled(bool)),
            SLOT(useQmlDebuggerToggled(bool)));
    connect(m_useCppDebugger, SIGNAL(toggled(bool)),
            SLOT(useCppDebuggerToggled(bool)));
    connect(m_debugServerPort, SIGNAL(valueChanged(int)),
            SLOT(qmlDebugServerPortChanged(int)));
    connect(m_useMultiProcess, SIGNAL(toggled(bool)),
            SLOT(useMultiProcessToggled(bool)));

    if (m_aspect->isDisplaySuppressed())
        hide();

    if (m_aspect->areQmlDebuggingOptionsSuppressed()) {
        m_debugServerPortLabel->hide();
        m_debugServerPort->hide();
        m_useQmlDebugger->hide();
    }

    if (m_aspect->areCppDebuggingOptionsSuppressed())
        m_useCppDebugger->hide();

    if (m_aspect->isQmlDebuggingSpinboxSuppressed()) {
        m_debugServerPort->hide();
        m_debugServerPortLabel->hide();
    }

    QHBoxLayout *qmlLayout = new QHBoxLayout;
    qmlLayout->setMargin(0);
    qmlLayout->addWidget(m_useQmlDebugger);
    qmlLayout->addWidget(m_debugServerPortLabel);
    qmlLayout->addWidget(m_debugServerPort);
    qmlLayout->addWidget(m_qmlDebuggerInfoLabel);
    qmlLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_useCppDebugger);
    layout->addLayout(qmlLayout);
    layout->addWidget(m_useMultiProcess);
    setLayout(layout);
}

void DebuggerRunConfigWidget::qmlDebugServerPortChanged(int port)
{
    m_aspect->m_qmlDebugServerPort = port;
}

void DebuggerRunConfigWidget::useCppDebuggerToggled(bool on)
{
    m_aspect->m_useCppDebugger = on;
    if (!on && !m_useQmlDebugger->isChecked())
        m_useQmlDebugger->setChecked(true);
}

void DebuggerRunConfigWidget::useQmlDebuggerToggled(bool on)
{
    m_debugServerPort->setEnabled(on);
    m_debugServerPortLabel->setEnabled(on);

    m_aspect->m_useQmlDebugger = on
            ? DebuggerRunConfigurationAspect::EnableQmlDebugger
            : DebuggerRunConfigurationAspect::DisableQmlDebugger;
    if (!on && !m_useCppDebugger->isChecked())
        m_useCppDebugger->setChecked(true);
}

void DebuggerRunConfigWidget::useMultiProcessToggled(bool on)
{
    m_aspect->m_useMultiProcess = on;
}

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlPrivate
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunControlPrivate
{
public:
    explicit DebuggerRunControlPrivate(DebuggerRunControl *parent,
                                       RunConfiguration *runConfiguration);

public:
    DebuggerRunControl *q;
    DebuggerEngine *m_engine;
    const QPointer<RunConfiguration> m_myRunConfiguration;
    bool m_running;
};

DebuggerRunControlPrivate::DebuggerRunControlPrivate(DebuggerRunControl *parent,
                                                     RunConfiguration *runConfiguration)
    : q(parent)
    , m_engine(0)
    , m_myRunConfiguration(runConfiguration)
    , m_running(false)
{
}

} // namespace Internal

DebuggerRunControl::DebuggerRunControl(RunConfiguration *runConfiguration,
                                       const DebuggerStartParameters &sp)
    : RunControl(runConfiguration, DebugRunMode),
      d(new DebuggerRunControlPrivate(this, runConfiguration))
{
    connect(this, SIGNAL(finished()), SLOT(handleFinished()));
    // Create the engine. Could arguably be moved to the factory, but
    // we still have a derived S60DebugControl. Should rarely fail, though.
    QString errorMessage;
    d->m_engine = DebuggerRunControlFactory::createEngine(sp.masterEngineType, sp, &errorMessage);

    if (d->m_engine) {
        DebuggerToolTipManager::instance()->registerEngine(d->m_engine);
    } else {
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

const DebuggerStartParameters &DebuggerRunControl::startParameters() const
{
    QTC_ASSERT(d->m_engine, return *(new DebuggerStartParameters()));
    return d->m_engine->startParameters();
}

QString DebuggerRunControl::displayName() const
{
    QTC_ASSERT(d->m_engine, return QString());
    return d->m_engine->startParameters().displayName;
}

QIcon DebuggerRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_DEBUG_SMALL));
}

void DebuggerRunControl::setCustomEnvironment(Environment env)
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->startParameters().environment = env;
}

void DebuggerRunControl::start()
{
    QTC_ASSERT(d->m_engine, return);
    // User canceled input dialog asking for executable when working on library project.
    if (d->m_engine->startParameters().startMode == StartInternal
        && d->m_engine->startParameters().executable.isEmpty()) {
        appendMessage(tr("No executable specified.\n"), ErrorMessageFormat);
        emit started();
        emit finished();
        return;
    }

    if (d->m_engine->startParameters().startMode == StartInternal) {
        foreach (const BreakpointModelId &id, debuggerCore()->breakHandler()->allBreakpointIds()) {
            if (d->m_engine->breakHandler()->breakpointData(id).enabled
                    && !d->m_engine->acceptsBreakpoint(id)) {

                QString warningMessage =
                        DebuggerPlugin::tr("Some breakpoints cannot be handled by the debugger "
                                           "languages currently active, and will be ignored.");

                debuggerCore()->showMessage(warningMessage, LogWarning);

                QErrorMessage *msgBox = new QErrorMessage(debuggerCore()->mainWindow());
                msgBox->setAttribute(Qt::WA_DeleteOnClose);
                msgBox->showMessage(warningMessage);
                break;
            }
        }
    }

    debuggerCore()->runControlStarted(d->m_engine);

    // We might get a synchronous startFailed() notification on Windows,
    // when launching the process fails. Emit a proper finished() sequence.
    emit started();
    d->m_running = true;

    d->m_engine->startDebugger(this);

    if (d->m_running)
        appendMessage(tr("Debugging starts\n"), NormalMessageFormat);
}

void DebuggerRunControl::startFailed()
{
    appendMessage(tr("Debugging has failed\n"), NormalMessageFormat);
    d->m_running = false;
    emit finished();
    d->m_engine->handleStartFailed();
}

void DebuggerRunControl::handleFinished()
{
    appendMessage(tr("Debugging has finished\n"), NormalMessageFormat);
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

QString DebuggerRunControlFactory::displayName() const
{
    return DebuggerPlugin::tr("Debug");
}

static DebuggerStartParameters localStartParameters(RunConfiguration *runConfiguration, QString *errorMessage)
{
    DebuggerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    LocalApplicationRunConfiguration *rc =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);
    if (!rc->ensureConfigured(errorMessage))
        return sp;

    Target *target = runConfiguration->target();
    Kit *kit = target ? target->kit() : KitManager::instance()->defaultKit();
    if (!fillParameters(&sp, kit, errorMessage))
        return sp;
    sp.environment = rc->environment();
    sp.workingDirectory = rc->workingDirectory();

#if defined(Q_OS_WIN)
    // Work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch' ...)
    sp.workingDirectory = normalizePathName(sp.workingDirectory);
#endif

    sp.executable = rc->executable();
    if (sp.executable.isEmpty())
        return sp;
    sp.startMode = StartInternal;
    sp.processArgs = rc->commandLineArguments();
    sp.useTerminal = rc->runMode() == LocalApplicationRunConfiguration::Console;
    sp.dumperLibrary = rc->dumperLibrary();
    sp.dumperLibraryLocations = rc->dumperLibraryLocations();

    if (target) {
        if (const Project *project = target->project()) {
            sp.projectSourceDirectory = project->projectDirectory();
            if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
                sp.projectBuildDirectory = buildConfig->buildDirectory();
            sp.projectSourceFiles = project->files(Project::ExcludeGeneratedFiles);
        }
    }

    DebuggerRunConfigurationAspect *aspect = runConfiguration->debuggerAspect();
    sp.multiProcess = aspect->useMultiProcess();

    if (aspect->useCppDebugger())
        sp.languages |= CppLanguage;

    if (aspect->useQmlDebugger()) {
        sp.qmlServerAddress = _("127.0.0.1");
        sp.qmlServerPort = aspect->qmlDebugServerPort();
        sp.languages |= QmlLanguage;

        // Makes sure that all bindings go through the JavaScript engine, so that
        // breakpoints are actually hit!
        const QString optimizerKey = _("QML_DISABLE_OPTIMIZER");
        if (!sp.environment.hasKey(optimizerKey))
            sp.environment.set(optimizerKey, _("1"));

        QtcProcess::addArg(&sp.processArgs, QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(sp.qmlServerPort));
    }

    // FIXME: If it's not yet build this will be empty and not filled
    // when rebuild as the runConfiguration is not stored and therefore
    // cannot be used to retrieve the dumper location.
    //qDebug() << "DUMPER: " << sp.dumperLibrary << sp.dumperLibraryLocations;
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

    if (sp.executable.endsWith(_(".js"))) {
        sp.masterEngineType = ScriptEngineType;
        return true;
    }

    if (sp.executable.endsWith(_(".py"))) {
        sp.masterEngineType = PdbEngineType;
        return true;
    }

    if (rc) {
        DebuggerRunConfigurationAspect *aspect = rc->debuggerAspect();
        if (const Target *target = rc->target())
            if (!fillParameters(&sp, target->kit(), errorMessage))
                return false;
        const bool useCppDebugger = aspect->useCppDebugger();
        const bool useQmlDebugger = aspect->useQmlDebugger();
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
    TaskHub *th = ProjectExplorerPlugin::instance()->taskHub();
    th->clearTasks(Core::Id(Debugger::Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO));
    th->clearTasks(Core::Id(Debugger::Constants::TASK_CATEGORY_DEBUGGER_TEST));
    th->clearTasks(Core::Id(Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME));

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
            sp.debugSourceLocation.append(base + QLatin1String("qt5base/src/v8"));
            sp.debugSourceLocation.append(base + QLatin1String("qt5declarative/src/qml"));
        }
    }

    if (!fixupEngineTypes(sp, rc, errorMessage))
        return 0;

    return new DebuggerRunControl(rc, sp);
}

DebuggerRunControl *DebuggerRunControlFactory::createAndScheduleRun
    (const DebuggerStartParameters &sp, RunConfiguration *runConfiguration)
{
    QString errorMessage;
    if (runConfiguration && !runConfiguration->ensureConfigured(&errorMessage))
        ProjectExplorer::ProjectExplorerPlugin::showRunErrorMessage(errorMessage);

    DebuggerRunControl *rc = doCreate(sp, runConfiguration, &errorMessage);
    if (!rc) {
        ProjectExplorer::ProjectExplorerPlugin::showRunErrorMessage(errorMessage);
        return 0;
    }
    debuggerCore()->showMessage(sp.startMessage, 0);
    ProjectExplorerPlugin::instance()->startRunControl(rc, DebugRunMode);
    return rc;
}

RunConfigWidget *DebuggerRunControlFactory::createConfigurationWidget
    (RunConfiguration *runConfiguration)
{
    return new DebuggerRunConfigWidget(runConfiguration);
}

DebuggerEngine *DebuggerRunControlFactory::createEngine(DebuggerEngineType et,
    const DebuggerStartParameters &sp, QString *errorMessage)
{
    switch (et) {
    case GdbEngineType:
        return createGdbEngine(sp);
    case ScriptEngineType:
        return createScriptEngine(sp);
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
    *errorMessage = DebuggerPlugin::tr("Unable to create a debugger engine of the type '%1'").
                    arg(_(engineTypeName(et)));
    return 0;
}

} // namespace Debugger

#include "debuggerrunner.moc"
