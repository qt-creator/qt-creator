/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
#include "lldb/lldbenginehost.h"
#include "debuggertooltipmanager.h"
#include "qml/qmlengine.h"

#ifdef Q_OS_WIN
#  include "peutils.h"
#  include <utils/winutils.h>
#endif

#include <projectexplorer/abi.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/applicationrunconfiguration.h> // For LocalApplication*

#include <utils/outputformat.h>
#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <utils/qtcprocess.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <utils/buildablehelperlibrary.h>

#include <QDir>
#include <QCheckBox>
#include <QSpinBox>
#include <QDebug>
#include <QErrorMessage>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Debugger::Internal;

enum { debug = 0 };

namespace Debugger {
namespace Internal {

bool isCdbEngineEnabled(); // Check the configuration page
bool checkCdbConfiguration(const DebuggerStartParameters &sp, ConfigurationCheck *check);
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &sp,
    DebuggerEngine *masterEngine, QString *error);

bool checkGdbConfiguration(const DebuggerStartParameters &sp, ConfigurationCheck *check);
DebuggerEngine *createGdbEngine(const DebuggerStartParameters &sp,
    DebuggerEngine *masterEngine);

DebuggerEngine *createScriptEngine(const DebuggerStartParameters &sp);
DebuggerEngine *createPdbEngine(const DebuggerStartParameters &sp);
QmlEngine *createQmlEngine(const DebuggerStartParameters &sp,
    DebuggerEngine *masterEngine);
DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &sp,
                                   DebuggerEngineType slaveEngineType,
                                   QString *errorMessage);
DebuggerEngine *createLldbEngine(const DebuggerStartParameters &sp);

extern QString msgNoBinaryForToolChain(const Abi &abi);

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

static inline QString engineTypeNames(const QList<DebuggerEngineType> &l)
{
    QString rc;
    foreach (DebuggerEngineType et, l) {
        if (!rc.isEmpty())
            rc.append(QLatin1String(", "));
        rc += QLatin1String(engineTypeName(et));
    }
    return rc;
}

static QString msgEngineNotAvailable(const char *engine)
{
    return DebuggerPlugin::tr("The application requires the debugger engine '%1', "
        "which is disabled.").arg(_(engine));
}

static inline QString msgEngineNotAvailable(DebuggerEngineType et)
{
    return msgEngineNotAvailable(engineTypeName(et));
}

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunConfigWidget
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunConfigWidget : public ProjectExplorer::RunConfigWidget
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

    DebuggerEngineType engineForExecutable(unsigned enabledEngineTypes,
        const QString &executable);
    DebuggerEngineType engineForMode(unsigned enabledEngineTypes,
        DebuggerStartMode mode);

public:
    DebuggerRunControl *q;
    DebuggerEngine *m_engine;
    const QWeakPointer<RunConfiguration> m_myRunConfiguration;
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
                                       const DebuggerStartParameters &sp,
                                       const QPair<DebuggerEngineType, DebuggerEngineType> &masterSlaveEngineTypes)
    : RunControl(runConfiguration, ProjectExplorer::DebugRunMode),
      d(new DebuggerRunControlPrivate(this, runConfiguration))
{
    connect(this, SIGNAL(finished()), SLOT(handleFinished()));
    // Create the engine. Could arguably be moved to the factory, but
    // we still have a derived S60DebugControl. Should rarely fail, though.
    QString errorMessage;
    d->m_engine = masterSlaveEngineTypes.first == QmlCppEngineType ?
            createQmlCppEngine(sp, masterSlaveEngineTypes.second, &errorMessage) :
            DebuggerRunControlFactory::createEngine(masterSlaveEngineTypes.first, sp,
                                                    0, &errorMessage);
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

void DebuggerRunControl::setCustomEnvironment(Utils::Environment env)
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
        appendMessage(tr("No executable specified.\n"), Utils::ErrorMessageFormat);
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
        appendMessage(tr("Debugging starts\n"), Utils::NormalMessageFormat);
}

void DebuggerRunControl::startFailed()
{
    appendMessage(tr("Debugging has failed\n"), Utils::NormalMessageFormat);
    d->m_running = false;
    emit finished();
    d->m_engine->handleStartFailed();
}

void DebuggerRunControl::handleFinished()
{
    appendMessage(tr("Debugging has finished\n"), Utils::NormalMessageFormat);
    if (d->m_engine)
        d->m_engine->handleFinished();
    debuggerCore()->runControlFinished(d->m_engine);
}

void DebuggerRunControl::showMessage(const QString &msg, int channel)
{
    switch (channel) {
        case AppOutput:
            appendMessage(msg, Utils::StdOutFormatSameLine);
            break;
        case AppError:
            appendMessage(msg, Utils::StdErrFormatSameLine);
            break;
        case AppStuff:
            appendMessage(msg, Utils::DebugFormat);
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
// Engine detection logic: Detection functions depending on tool chain, binary,
// etc. Return a list of possible engines (order of prefererence) without
// consideration of configuration, etc.
//
////////////////////////////////////////////////////////////////////////

static QList<DebuggerEngineType> enginesForToolChain(const Abi &toolChain,
                                                     DebuggerLanguages languages)
{
    QList<DebuggerEngineType> result;
    switch (toolChain.binaryFormat()) {
    case Abi::ElfFormat:
    case Abi::MachOFormat:
        result.push_back(LldbEngineType);
        result.push_back(GdbEngineType);
        if (languages & QmlLanguage)
            result.push_back(QmlEngineType);
        break;
   case Abi::PEFormat:
        if (toolChain.osFlavor() == Abi::WindowsMSysFlavor) {
            result.push_back(GdbEngineType);
            result.push_back(CdbEngineType);
        } else {
            result.push_back(CdbEngineType);
            //result.push_back(GdbEngineType);
        }
        if (languages & QmlLanguage)
            result.push_back(QmlEngineType);
        break;
    case Abi::RuntimeQmlFormat:
        result.push_back(QmlEngineType);
        break;
    default:
        break;
    }
    return result;
}

static inline QList<DebuggerEngineType> enginesForScriptExecutables(const QString &executable)
{
    QList<DebuggerEngineType> result;
    if (executable.endsWith(_(".js"))) {
        result.push_back(ScriptEngineType);
    } else if (executable.endsWith(_(".py"))) {
        result.push_back(PdbEngineType);
    }
    return result;
}

static QList<DebuggerEngineType> enginesForExecutable(const QString &executable)
{
    QList<DebuggerEngineType> result = enginesForScriptExecutables(executable);
    if (!result.isEmpty())
        return result;
#ifdef Q_OS_WIN
    // A remote executable?
    if (!executable.endsWith(_(".exe"), Qt::CaseInsensitive)) {
        result.push_back(GdbEngineType);
        return result;
    }

    // If a file has PDB files, it has been compiled by VS.
    QStringList pdbFiles;
    QString errorMessage;
    if (getPDBFiles(executable, &pdbFiles, &errorMessage) && !pdbFiles.isEmpty()) {
        result.push_back(CdbEngineType);
        result.push_back(GdbEngineType);
        return result;
    }
    // Fixme: Gdb should only be preferred if MinGW can positively be detected.
    result.push_back(GdbEngineType);
    result.push_back(CdbEngineType);
#else
    result.push_back(LldbEngineType);
    result.push_back(GdbEngineType);
#endif
    return result;
}

// Debugger type for mode.
static QList<DebuggerEngineType> enginesForMode(DebuggerStartMode startMode,
                                                DebuggerLanguages languages,
                                                bool hardConstraintsOnly)
{
    QList<DebuggerEngineType> result;

    if (languages == QmlLanguage) {
        QTC_ASSERT(startMode == StartInternal
                   || startMode == AttachToRemoteServer
                   || startMode == AttachToRemoteProcess,
                   qDebug() << "qml debugging not supported for mode"
                            << startMode);

        // Qml language only
        result.push_back(QmlEngineType);
        return result;
    }

    switch (startMode) {
    case NoStartMode:
        break;
    case StartInternal:
    case StartExternal:
    case AttachExternal:
        if (!hardConstraintsOnly) {
#ifdef Q_OS_WIN
            result.push_back(CdbEngineType); // Preferably Windows debugger for attaching locally.
#endif
            result.push_back(GdbEngineType);

            if (languages & QmlLanguage)
                result.push_back(QmlEngineType);
        }
        break;
    case LoadRemoteCore:
        result.push_back(GdbEngineType);
        break;
    case AttachCore:
#ifdef Q_OS_WIN
        result.push_back(CdbEngineType);
#endif
        result.push_back(GdbEngineType);
        break;
    case StartRemoteProcess:
    case StartRemoteGdb:
        result.push_back(GdbEngineType);
        if (languages & QmlLanguage)
            result.push_back(QmlEngineType);
        break;
    case AttachToRemoteProcess:
    case AttachToRemoteServer:
        if (!hardConstraintsOnly) {
#ifdef Q_OS_WIN
            result.push_back(CdbEngineType);
#endif
            result.push_back(GdbEngineType);

            if (languages & QmlLanguage)
                result.push_back(QmlEngineType);
        }
        break;
    case AttachCrashedExternal:
        result.push_back(CdbEngineType); // Only CDB can do this
        break;
    case StartRemoteEngine:
        // FIXME: Unclear IPC override. Someone please have a better idea.
        // For now thats the only supported IPC engine.
        result.push_back(LldbEngineType);
        break;
    }
    return result;
}

// Engine detection logic: Call all detection functions in order.

static QList<DebuggerEngineType> engineTypes(const DebuggerStartParameters &sp)
{
    // Script executables and certain start modes are 'hard constraints'.
    QList<DebuggerEngineType> result = enginesForScriptExecutables(sp.executable);
    if (!result.isEmpty())
        return result;

    result = enginesForMode(sp.startMode, sp.languages, true);
    if (!result.isEmpty())
        return result;

    //  'hard constraints' done (with the exception of QML ABI checked here),
    // further try to restrict available engines.
    if (sp.toolChainAbi.isValid()) {
        result = enginesForToolChain(sp.toolChainAbi, sp.languages);
        if (!result.isEmpty())
            return result;
    }

    // FIXME: 1 of 3 testing hacks.
    if (sp.processArgs.startsWith(QLatin1String("@tcf@ "))) {
        result.push_back(GdbEngineType);
        return result;
    }

    if (sp.startMode != AttachToRemoteServer
            && sp.startMode != AttachToRemoteProcess
            && sp.startMode != LoadRemoteCore
            && !sp.executable.isEmpty())
        result = enginesForExecutable(sp.executable);
    if (!result.isEmpty())
        return result;

    result = enginesForMode(sp.startMode, sp.languages, false);
    return result;
}

// Engine detection logic: ConfigurationCheck.
ConfigurationCheck::ConfigurationCheck() :
    masterSlaveEngineTypes(NoEngineType, NoEngineType)
{
}

ConfigurationCheck::operator bool() const
{
    return errorMessage.isEmpty()
        && errorDetails.isEmpty()
        && masterSlaveEngineTypes.first != NoEngineType;
}

QString ConfigurationCheck::errorDetailsString() const
{
    return errorDetails.join(QLatin1String("\n\n"));
}

// Convenience helper to check whether an engine is enabled and configured
// correctly.
static inline bool canUseEngine(DebuggerEngineType et,
                                const DebuggerStartParameters &sp,
                                unsigned cmdLineEnabledEngines,
                                ConfigurationCheck *result)
{
    // Enabled?
    if ((et & cmdLineEnabledEngines) == 0) {
        result->errorDetails.push_back(DebuggerPlugin::tr("The debugger engine '%1' is disabled.").
                                       arg(QLatin1String(engineTypeName(et))));
        return false;
    }
    // Configured.
    switch (et) {
    case CdbEngineType:
        return checkCdbConfiguration(sp, result);
    case GdbEngineType:
        return checkGdbConfiguration(sp, result);
    default:
        break;
    }
    return true;
}

/*!
    \fn Debugger::ConfigurationCheck Debugger::checkDebugConfiguration(const DebuggerStartParameters &sp)

    This is the master engine detection function that returns the
    engine types for a given set of start parameters and checks their
    configuration.
*/

DEBUGGER_EXPORT ConfigurationCheck checkDebugConfiguration(const DebuggerStartParameters &sp)
{
    ConfigurationCheck result;
    if (debug)
        qDebug().nospace() << "checkDebugConfiguration " << sp.toolChainAbi.toString()
                           << " Start mode=" << sp.startMode << " Executable=" << sp.executable
                           << " Debugger command=" << sp.debuggerCommand;
    // Get all applicable types.
    QList<DebuggerEngineType> requiredTypes = engineTypes(sp);
    if (requiredTypes.isEmpty()) {
        result.errorMessage = QLatin1String("Internal error: Unable to determine debugger engine type for this configuration");
        return result;
    }
    if (debug)
        qDebug() << " Required: " << engineTypeNames(requiredTypes);
    // Filter out disabled types, command line + current settings.
    unsigned cmdLineEnabledEngines = debuggerCore()->enabledEngines();
#ifdef WITH_LLDB
    if (!Core::ICore::settings()->value(QLatin1String("LLDB/enabled")).toBool())
        cmdLineEnabledEngines &= ~LldbEngineType;
#else
     cmdLineEnabledEngines &= ~LldbEngineType;
#endif

    DebuggerEngineType usableType = NoEngineType;
    QList<DebuggerEngineType> unavailableTypes;
    foreach (DebuggerEngineType et, requiredTypes) {
        if (canUseEngine(et, sp, cmdLineEnabledEngines, &result)) {
            result.errorDetails.clear();
            usableType = et;
            break;
        } else {
            unavailableTypes.push_back(et);
        }
    }
    if (usableType == NoEngineType) {
        if (requiredTypes.size() == 1) {
            result.errorMessage = DebuggerPlugin::tr(
                "The debugger engine '%1' required for debugging binaries of the type '%2'"
                " is not configured correctly.").
                arg(QLatin1String(engineTypeName(requiredTypes.front())), sp.toolChainAbi.toString());
        } else {
            result.errorMessage = DebuggerPlugin::tr(
                "None of the debugger engines '%1' capable of debugging binaries of the type '%2'"
                " is configured correctly.").
                arg(engineTypeNames(requiredTypes), sp.toolChainAbi.toString());
        }
        return result;
    }
    if (debug)
        qDebug() << "Configured engine: " << engineTypeName(usableType);
    // Inform verbosely about MinGW-gdb/CDB fallbacks. Do not complain about LLDB, for now.
    if (!result.errorDetails.isEmpty() && unavailableTypes.count(LldbEngineType) != unavailableTypes.size()) {
        const QString msg = DebuggerPlugin::tr(
            "The preferred debugger engine for debugging binaries of type '%1' is not available.\n"
            "The debugger engine '%2' will be used as a fallback.\nDetails: %3").
                arg(sp.toolChainAbi.toString(), QLatin1String(engineTypeName(usableType)),
                    result.errorDetails.join(QString(QLatin1Char('\n'))));
        debuggerCore()->showMessage(msg, LogWarning);
        showMessageBox(QMessageBox::Warning, DebuggerPlugin::tr("Warning"), msg);
    }
    // Anything left: Happy.
    result.errorMessage.clear();
    result.errorDetails.clear();


    // Could we actually use a combined qml/cpp-engine?
     if (usableType != QmlEngineType
             && requiredTypes.contains(QmlEngineType)) {
         result.masterSlaveEngineTypes.first = QmlCppEngineType;
         result.masterSlaveEngineTypes.second = usableType;
     } else {
         result.masterSlaveEngineTypes.first = usableType;
     }

    if (debug)
        qDebug() << engineTypeName(result.masterSlaveEngineTypes.first) << engineTypeName(result.masterSlaveEngineTypes.second);
    return result;
}

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

// A factory to create DebuggerRunControls
DebuggerRunControlFactory::DebuggerRunControlFactory(QObject *parent,
        unsigned enabledEngines)
    : IRunControlFactory(parent), m_enabledEngines(enabledEngines)
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

// Find Qt installation by running qmake
static inline QString findQtInstallPath(const Utils::FileName &qmakePath)
{
    QProcess proc;
    QStringList args;
    args.append(_("-query"));
    args.append(_("QT_INSTALL_HEADERS"));
    proc.start(qmakePath.toString(), args);
    if (!proc.waitForStarted()) {
        qWarning("%s: Cannot start '%s': %s", Q_FUNC_INFO, qPrintable(qmakePath.toString()),
           qPrintable(proc.errorString()));
        return QString();
    }
    proc.closeWriteChannel();
    if (!proc.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(proc);
        qWarning("%s: Timeout running '%s'.", Q_FUNC_INFO, qPrintable(qmakePath.toString()));
        return QString();
    }
    if (proc.exitStatus() != QProcess::NormalExit) {
        qWarning("%s: '%s' crashed.", Q_FUNC_INFO, qPrintable(qmakePath.toString()));
        return QString();
    }
    const QByteArray ba = proc.readAllStandardOutput().trimmed();
    QDir dir(QString::fromLocal8Bit(ba));
    if (dir.exists() && dir.cdUp())
        return dir.absolutePath();
    return QString();
}

static DebuggerStartParameters localStartParameters(RunConfiguration *runConfiguration)
{
    DebuggerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    LocalApplicationRunConfiguration *rc =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.environment = rc->environment();
    sp.workingDirectory = rc->workingDirectory();

#if defined(Q_OS_WIN)
    // Work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch' ...)
    sp.workingDirectory = Utils::normalizePathName(sp.workingDirectory);
#endif

    sp.executable = rc->executable();
    if (sp.executable.isEmpty())
        return sp;
    sp.startMode = StartInternal;
    sp.processArgs = rc->commandLineArguments();
    sp.toolChainAbi = rc->abi();
    if (!sp.toolChainAbi.isValid()) {
        QList<Abi> abis = Abi::abisOfBinary(Utils::FileName::fromString(sp.executable));
        if (!abis.isEmpty())
            sp.toolChainAbi = abis.at(0);
    }
    sp.useTerminal = rc->runMode() == LocalApplicationRunConfiguration::Console;
    sp.dumperLibrary = rc->dumperLibrary();
    sp.dumperLibraryLocations = rc->dumperLibraryLocations();

    if (const ProjectExplorer::Target *target = runConfiguration->target()) {
        if (QByteArray(target->metaObject()->className()).contains("Qt4")) {
            const Utils::FileName qmake = Utils::BuildableHelperLibrary::findSystemQt(sp.environment);
            if (!qmake.isEmpty())
                sp.qtInstallPath = findQtInstallPath(qmake);
        }
        if (const ProjectExplorer::Project *project = target->project()) {
            sp.projectSourceDirectory = project->projectDirectory();
            if (const ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration()) {
                sp.projectBuildDirectory = buildConfig->buildDirectory();
                if (const ProjectExplorer::ToolChain *tc = buildConfig->toolChain())
                    sp.debuggerCommand = tc->debuggerCommand().toString();
            }
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
        if (!sp.environment.hasKey(optimizerKey)) {
            sp.environment.set(optimizerKey, _("1"));
        }

        Utils::QtcProcess::addArg(&sp.processArgs,
                                  QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(sp.qmlServerPort));
    }

    // FIXME: If it's not yet build this will be empty and not filled
    // when rebuild as the runConfiguration is not stored and therefore
    // cannot be used to retrieve the dumper location.
    //qDebug() << "DUMPER: " << sp.dumperLibrary << sp.dumperLibraryLocations;
    sp.displayName = rc->displayName();

    return sp;
}

RunControl *DebuggerRunControlFactory::create
    (RunConfiguration *runConfiguration, RunMode mode)
{
    QTC_ASSERT(mode == DebugRunMode || mode == DebugRunModeWithBreakOnMain, return 0);
    DebuggerStartParameters sp = localStartParameters(runConfiguration);
    if (sp.startMode == NoStartMode)
        return 0;
    if (mode == DebugRunModeWithBreakOnMain)
        sp.breakOnMain = true;
    return create(sp, runConfiguration);
}

RunConfigWidget *DebuggerRunControlFactory::createConfigurationWidget
    (RunConfiguration *runConfiguration)
{
    return new DebuggerRunConfigWidget(runConfiguration);
}

DebuggerRunControl *DebuggerRunControlFactory::create
    (const DebuggerStartParameters &sp, RunConfiguration *runConfiguration)
{
    const ConfigurationCheck check = checkDebugConfiguration(sp);

    if (!check) {
        //appendMessage(errorMessage, true);
        Core::ICore::showWarningWithOptions(DebuggerPlugin::tr("Debugger"),
            check.errorMessage, check.errorDetailsString(), check.settingsCategory, check.settingsPage);
        return 0;
    }

    return new DebuggerRunControl(runConfiguration, sp, check.masterSlaveEngineTypes);
}

DebuggerEngine *DebuggerRunControlFactory::createEngine
    (DebuggerEngineType et,
     const DebuggerStartParameters &sp,
     DebuggerEngine *masterEngine,
     QString *errorMessage)
{
    switch (et) {
    case GdbEngineType:
        return createGdbEngine(sp, masterEngine);
    case ScriptEngineType:
        return createScriptEngine(sp);
    case CdbEngineType:
        return createCdbEngine(sp, masterEngine, errorMessage);
    case PdbEngineType:
        return createPdbEngine(sp);
    case QmlEngineType:
        return createQmlEngine(sp, masterEngine);
    case LldbEngineType:
        return createLldbEngine(sp);
    default:
        break;
    }
    *errorMessage = DebuggerPlugin::tr("Unable to create a debugger engine of the type '%1'").
                    arg(_(engineTypeName(et)));
    return 0;
}

} // namespace Debugger

#include "debuggerrunner.moc"
