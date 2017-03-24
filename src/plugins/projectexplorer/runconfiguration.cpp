/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "runconfiguration.h"

#include "project.h"
#include "target.h"
#include "toolchain.h"
#include "abi.h"
#include "buildconfiguration.h"
#include "environmentaspect.h"
#include "kitinformation.h"
#include "runnables.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>

#include <QDir>
#include <QPushButton>
#include <QTimer>

#ifdef Q_OS_OSX
#include <ApplicationServices/ApplicationServices.h>
#endif

#if defined (WITH_JOURNALD)
#include "journaldwatcher.h"
#endif

using namespace Utils;

namespace ProjectExplorer {

///////////////////////////////////////////////////////////////////////
//
// ISettingsAspect
//
///////////////////////////////////////////////////////////////////////

ISettingsAspect *ISettingsAspect::clone() const
{
    ISettingsAspect *other = create();
    QVariantMap data;
    toMap(data);
    other->fromMap(data);
    return other;
}

///////////////////////////////////////////////////////////////////////
//
// IRunConfigurationAspect
//
///////////////////////////////////////////////////////////////////////

IRunConfigurationAspect::IRunConfigurationAspect(RunConfiguration *runConfig) :
    m_runConfiguration(runConfig)
{ }

IRunConfigurationAspect::~IRunConfigurationAspect()
{
    delete m_projectSettings;
}

/*!
    Returns the widget used to configure this run configuration. Ownership is
    transferred to the caller.
*/

RunConfigWidget *IRunConfigurationAspect::createConfigurationWidget() const
{
    return m_runConfigWidgetCreator ? m_runConfigWidgetCreator() : nullptr;
}

void IRunConfigurationAspect::setProjectSettings(ISettingsAspect *settings)
{
    m_projectSettings = settings;
}

void IRunConfigurationAspect::setGlobalSettings(ISettingsAspect *settings)
{
    m_globalSettings = settings;
}

void IRunConfigurationAspect::setUsingGlobalSettings(bool value)
{
    m_useGlobalSettings = value;
}

ISettingsAspect *IRunConfigurationAspect::currentSettings() const
{
   return m_useGlobalSettings ? m_globalSettings : m_projectSettings;
}

void IRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    m_projectSettings->fromMap(map);
    m_useGlobalSettings = map.value(m_id.toString() + QLatin1String(".UseGlobalSettings"), true).toBool();
}

void IRunConfigurationAspect::toMap(QVariantMap &map) const
{
    m_projectSettings->toMap(map);
    map.insert(m_id.toString() + QLatin1String(".UseGlobalSettings"), m_useGlobalSettings);
}

void IRunConfigurationAspect::setRunConfigWidgetCreator(const RunConfigWidgetCreator &runConfigWidgetCreator)
{
    m_runConfigWidgetCreator = runConfigWidgetCreator;
}

IRunConfigurationAspect *IRunConfigurationAspect::clone(RunConfiguration *runConfig) const
{
    IRunConfigurationAspect *other = create(runConfig);
    if (m_projectSettings)
        other->m_projectSettings = m_projectSettings->clone();
    other->m_globalSettings = m_globalSettings;
    other->m_useGlobalSettings = m_useGlobalSettings;
    return other;
}

void IRunConfigurationAspect::resetProjectToGlobalSettings()
{
    QTC_ASSERT(m_globalSettings, return);
    QVariantMap map;
    m_globalSettings->toMap(map);
    m_projectSettings->fromMap(map);
}


/*!
    \class ProjectExplorer::RunConfiguration
    \brief The RunConfiguration class is the base class for a run configuration.

    A run configuration specifies how a target should be run, while a runner
    does the actual running.

    All RunControls and the target hold a shared pointer to the run
    configuration. That is, the lifetime of the run configuration might exceed
    the life of the target.
    The user might still have a RunControl running (or output tab of that RunControl open)
    and yet unloaded the target.

    Also, a run configuration might be already removed from the list of run
    configurations
    for a target, but still be runnable via the output tab.
*/

RunConfiguration::RunConfiguration(Target *target, Core::Id id) :
    ProjectConfiguration(target, id)
{
    Q_ASSERT(target);
    ctor();

    addExtraAspects();
}

RunConfiguration::RunConfiguration(Target *target, RunConfiguration *source) :
    ProjectConfiguration(target, source)
{
    Q_ASSERT(target);
    ctor();
    foreach (IRunConfigurationAspect *aspect, source->m_aspects) {
        IRunConfigurationAspect *clone = aspect->clone(this);
        if (clone)
            m_aspects.append(clone);
    }
}

RunConfiguration::~RunConfiguration()
{
    qDeleteAll(m_aspects);
}

void RunConfiguration::addExtraAspects()
{
    foreach (IRunControlFactory *factory, ExtensionSystem::PluginManager::getObjects<IRunControlFactory>())
        addExtraAspect(factory->createRunConfigurationAspect(this));
}

void RunConfiguration::addExtraAspect(IRunConfigurationAspect *aspect)
{
    if (aspect)
        m_aspects += aspect;
}

void RunConfiguration::ctor()
{
    connect(this, &RunConfiguration::enabledChanged,
            this, &RunConfiguration::requestRunActionsUpdate);

    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Run Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([this]() -> Utils::MacroExpander * {
        BuildConfiguration *bc = target()->activeBuildConfiguration();
        return bc ? bc->macroExpander() : target()->macroExpander();
    });
    expander->registerPrefix("CurrentRun:Env", tr("Variables in the current run environment"),
                             [this](const QString &var) {
        const auto envAspect = extraAspect<EnvironmentAspect>();
        return envAspect ? envAspect->environment().value(var) : QString();
    });
    expander->registerVariable(Constants::VAR_CURRENTRUN_NAME,
            QCoreApplication::translate("ProjectExplorer", "The currently active run configuration's name."),
            [this] { return displayName(); }, false);
}

/*!
    Checks whether a run configuration is enabled.
*/

bool RunConfiguration::isEnabled() const
{
    return true;
}

QString RunConfiguration::disabledReason() const
{
    return QString();
}

bool RunConfiguration::isConfigured() const
{
    return true;
}

RunConfiguration::ConfigurationState RunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (isConfigured())
        return Configured;
    if (errorMessage)
        *errorMessage = tr("Unknown error.");
    return UnConfigured;
}


BuildConfiguration *RunConfiguration::activeBuildConfiguration() const
{
    if (!target())
        return nullptr;
    return target()->activeBuildConfiguration();
}

Target *RunConfiguration::target() const
{
    return static_cast<Target *>(parent());
}

QVariantMap RunConfiguration::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();

    foreach (IRunConfigurationAspect *aspect, m_aspects)
        aspect->toMap(map);

    return map;
}

Abi RunConfiguration::abi() const
{
    BuildConfiguration *bc = target()->activeBuildConfiguration();
    if (!bc)
        return Abi::hostAbi();
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), Constants::CXX_LANGUAGE_ID);
    if (!tc)
        return Abi::hostAbi();
    return tc->targetAbi();
}

bool RunConfiguration::fromMap(const QVariantMap &map)
{
    foreach (IRunConfigurationAspect *aspect, m_aspects)
        aspect->fromMap(map);

    return ProjectConfiguration::fromMap(map);
}

/*!
    \class ProjectExplorer::IRunConfigurationAspect

    \brief The IRunConfigurationAspect class provides an additional
    configuration aspect.

    Aspects are a mechanism to add RunControl-specific options to a run
    configuration without subclassing the run configuration for every addition.
    This prevents a combinatorial explosion of subclasses and eliminates
    the need to add all options to the base class.
*/

/*!
    Returns extra aspects.

    \sa ProjectExplorer::IRunConfigurationAspect
*/

QList<IRunConfigurationAspect *> RunConfiguration::extraAspects() const
{
    return m_aspects;
}

IRunConfigurationAspect *RunConfiguration::extraAspect(Core::Id id) const
{
    return Utils::findOrDefault(m_aspects, Utils::equal(&IRunConfigurationAspect::id, id));
}

/*!
    \internal

    \class ProjectExplorer::Runnable

    \brief The ProjectExplorer::Runnable class wraps information needed
    to execute a process on a target device.

    A target specific \l RunConfiguration implementation can specify
    what information it considers necessary to execute a process
    on the target. Target specific) \n IRunControlFactory implementation
    can use that information either unmodified or tweak it or ignore
    it when setting up a RunControl.

    From Qt Creator's core perspective a Runnable object is opaque.
*/

/*!
    \internal

    \brief Returns a \l Runnable described by this RunConfiguration.
*/

Runnable RunConfiguration::runnable() const
{
    return Runnable();
}

Utils::OutputFormatter *RunConfiguration::createOutputFormatter() const
{
    return new Utils::OutputFormatter();
}


/*!
    \class ProjectExplorer::IRunConfigurationFactory

    \brief The IRunConfigurationFactory class restores run configurations from
    settings.

    The run configuration factory is used for restoring run configurations from
    settings and for creating new run configurations in the \gui {Run Settings}
    dialog.
    To restore run configurations, use the
    \c {bool canRestore(Target *parent, const QString &id)}
    and \c {RunConfiguration* create(Target *parent, const QString &id)}
    functions.

    To generate a list of creatable run configurations, use the
    \c {QStringList availableCreationIds(Target *parent)} and
    \c {QString displayNameForType(const QString&)} functions. To create a
    run configuration, use \c create().
*/

/*!
    \fn QStringList ProjectExplorer::IRunConfigurationFactory::availableCreationIds(Target *parent) const

    Shows the list of possible additions to a target. Returns a list of types.
*/

/*!
    \fn QString ProjectExplorer::IRunConfigurationFactory::displayNameForId(Core::Id id) const
    Translates the types to names to display to the user.
*/

IRunConfigurationFactory::IRunConfigurationFactory(QObject *parent) :
    QObject(parent)
{
}

RunConfiguration *IRunConfigurationFactory::create(Target *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return nullptr;
    RunConfiguration *rc = doCreate(parent, id);
    if (!rc)
        return nullptr;
    return rc;
}

RunConfiguration *IRunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return nullptr;
    RunConfiguration *rc = doRestore(parent, map);
    if (!rc->fromMap(map)) {
        delete rc;
        rc = nullptr;
    }
    return rc;
}

IRunConfigurationFactory *IRunConfigurationFactory::find(Target *parent, const QVariantMap &map)
{
    return ExtensionSystem::PluginManager::getObject<IRunConfigurationFactory>(
        [&parent, &map](IRunConfigurationFactory *factory) {
            return factory->canRestore(parent, map);
        });
}

IRunConfigurationFactory *IRunConfigurationFactory::find(Target *parent, RunConfiguration *rc)
{
    return ExtensionSystem::PluginManager::getObject<IRunConfigurationFactory>(
        [&parent, rc](IRunConfigurationFactory *factory) {
            return factory->canClone(parent, rc);
        });
}

QList<IRunConfigurationFactory *> IRunConfigurationFactory::find(Target *parent)
{
    return ExtensionSystem::PluginManager::getObjects<IRunConfigurationFactory>(
        [&parent](IRunConfigurationFactory *factory) {
            return !factory->availableCreationIds(parent).isEmpty();
        });
}

/*!
    \class ProjectExplorer::IRunControlFactory

    \brief The IRunControlFactory class creates RunControl objects matching a
    run configuration.
*/

/*!
    \fn RunConfigWidget *ProjectExplorer::IRunConfigurationAspect::createConfigurationWidget()

    Returns a widget used to configure this runner. Ownership is transferred to
    the caller.

    Returns null if @p \a runConfiguration is not suitable for RunControls from this
    factory, or no user-accessible
    configuration is required.
*/

IRunControlFactory::IRunControlFactory(QObject *parent)
    : QObject(parent)
{
}

/*!
    Returns an IRunConfigurationAspect to carry options for RunControls this
    factory can create.

    If no extra options are required, it is allowed to return null like the
    default implementation does. This function is intended to be called from the
    RunConfiguration constructor, so passing a RunConfiguration pointer makes
    no sense because that object is under construction at the time.
*/

IRunConfigurationAspect *IRunControlFactory::createRunConfigurationAspect(RunConfiguration *rc)
{
    Q_UNUSED(rc);
    return nullptr;
}

/*!
    \class ProjectExplorer::RunControl
    \brief The RunControl class instances represent one item that is run.
*/

/*!
    \fn QIcon ProjectExplorer::RunControl::icon() const
    Returns the icon to be shown in the Outputwindow.

    TODO the icon differs currently only per "mode", so this is more flexible
    than it needs to be.
*/

namespace Internal {

class RunControlPrivate
{
public:
    RunControlPrivate(RunConfiguration *runConfiguration, Core::Id mode)
        : runMode(mode), runConfiguration(runConfiguration)
    {
        if (runConfiguration) {
            displayName  = runConfiguration->displayName();
            outputFormatter = runConfiguration->createOutputFormatter();
            device = DeviceKitInformation::device(runConfiguration->target()->kit());
            project = runConfiguration->target()->project();
        }
    }

    ~RunControlPrivate()
    {
        delete toolRunner;
        delete outputFormatter;
    }

    QString displayName;
    Runnable runnable;
    IDevice::ConstPtr device;
    Connection connection;
    Core::Id runMode;
    Utils::Icon icon;
    const QPointer<RunConfiguration> runConfiguration; // Not owned.
    QPointer<Project> project; // Not owned.
    QPointer<ToolRunner> toolRunner; // Owned. QPointer as "extra safety" for now.
    Utils::OutputFormatter *outputFormatter = nullptr;

    // A handle to the actual application process.
    Utils::ProcessHandle applicationProcessHandle;

    RunControl::State state = RunControl::State::Initialized;

#ifdef Q_OS_OSX
    // This is used to bring apps in the foreground on Mac
    int foregroundCount;
#endif
};

} // Internal

RunControl::RunControl(RunConfiguration *runConfiguration, Core::Id mode) :
    d(new Internal::RunControlPrivate(runConfiguration, mode))
{
#ifdef WITH_JOURNALD
    JournaldWatcher::instance()->subscribe(this, [this](const JournaldWatcher::LogEntry &entry) {
        if (entry.value("_MACHINE_ID") != JournaldWatcher::instance()->machineId())
            return;

        const QByteArray pid = entry.value("_PID");
        if (pid.isEmpty())
            return;

        const qint64 pidNum = static_cast<qint64>(QString::fromLatin1(pid).toInt());
        if (pidNum != d->applicationProcessHandle.pid())
            return;

        const QString message = QString::fromUtf8(entry.value("MESSAGE")) + "\n";
        appendMessageRequested(this, message, Utils::OutputFormat::LogMessageFormat);
    });
#endif
}

RunControl::~RunControl()
{
#ifdef WITH_JOURNALD
    JournaldWatcher::instance()->unsubscribe(this);
#endif
    delete d;
}

void RunControl::initiateStart()
{
    setState(State::Starting);
    QTimer::singleShot(0, this, &RunControl::start);
}

void RunControl::initiateStop()
{
    setState(State::Stopping);
    QTimer::singleShot(0, this, &RunControl::stop);
}

Utils::OutputFormatter *RunControl::outputFormatter() const
{
    return d->outputFormatter;
}

Core::Id RunControl::runMode() const
{
    return d->runMode;
}

const Runnable &RunControl::runnable() const
{
    return d->runnable;
}

void RunControl::setRunnable(const Runnable &runnable)
{
    d->runnable = runnable;
}

const Connection &RunControl::connection() const
{
    return d->connection;
}

void RunControl::setConnection(const Connection &connection)
{
    d->connection = connection;
}

ToolRunner *RunControl::toolRunner() const
{
    return d->toolRunner;
}

void RunControl::setToolRunner(ToolRunner *tool)
{
    d->toolRunner = tool;
}

QString RunControl::displayName() const
{
    return d->displayName;
}

void RunControl::setDisplayName(const QString &displayName)
{
    d->displayName = displayName;
}

void RunControl::setIcon(const Utils::Icon &icon)
{
    d->icon = icon;
}

Utils::Icon RunControl::icon() const
{
    return d->icon;
}

Abi RunControl::abi() const
{
    if (const RunConfiguration *rc = d->runConfiguration.data())
        return rc->abi();
    return Abi();
}

IDevice::ConstPtr RunControl::device() const
{
   return d->device;
}

RunConfiguration *RunControl::runConfiguration() const
{
    return d->runConfiguration.data();
}

Project *RunControl::project() const
{
    return d->project.data();
}

bool RunControl::canReUseOutputPane(const RunControl *other) const
{
    if (other->isRunning())
        return false;

    return d->runnable.canReUseOutputPane(other->d->runnable);
}

/*!
    A handle to the application process.

    This is typically a process id, but should be treated as
    opaque handle to the process controled by this \c RunControl.
*/

ProcessHandle RunControl::applicationProcessHandle() const
{
    return d->applicationProcessHandle;
}

void RunControl::setApplicationProcessHandle(const ProcessHandle &handle)
{
    if (d->applicationProcessHandle != handle) {
        d->applicationProcessHandle = handle;
        emit applicationProcessHandleChanged(QPrivateSignal());
    }
}

/*!
    Prompts to stop. If \a optionalPrompt is passed, a \gui {Do not ask again}
    checkbox is displayed and the result is returned in \a *optionalPrompt.
*/

bool RunControl::promptToStop(bool *optionalPrompt) const
{
    QTC_ASSERT(isRunning(), return true);

    if (optionalPrompt && !*optionalPrompt)
        return true;

    const QString msg = tr("<html><head/><body><center><i>%1</i> is still running.<center/>"
                           "<center>Force it to quit?</center></body></html>").arg(displayName());
    return showPromptToStopDialog(tr("Application Still Running"), msg,
                                  tr("Force &Quit"), tr("&Keep Running"),
                                  optionalPrompt);
}

bool RunControl::isRunning() const
{
    return d->state == State::Running;
}

/*!
    Prompts to terminate the application with the \gui {Do not ask again}
    checkbox.
*/

bool RunControl::showPromptToStopDialog(const QString &title,
                                        const QString &text,
                                        const QString &stopButtonText,
                                        const QString &cancelButtonText,
                                        bool *prompt) const
{
    QTC_ASSERT(isRunning(), return true);
    // Show a question message box where user can uncheck this
    // question for this class.
    Utils::CheckableMessageBox messageBox(Core::ICore::mainWindow());
    messageBox.setWindowTitle(title);
    messageBox.setText(text);
    messageBox.setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
    if (!stopButtonText.isEmpty())
        messageBox.button(QDialogButtonBox::Yes)->setText(stopButtonText);
    if (!cancelButtonText.isEmpty())
        messageBox.button(QDialogButtonBox::Cancel)->setText(cancelButtonText);
    messageBox.setDefaultButton(QDialogButtonBox::Yes);
    if (prompt) {
        messageBox.setCheckBoxText(Utils::CheckableMessageBox::msgDoNotAskAgain());
        messageBox.setChecked(false);
    } else {
        messageBox.setCheckBoxVisible(false);
    }
    messageBox.exec();
    const bool close = messageBox.clickedStandardButton() == QDialogButtonBox::Yes;
    if (close && prompt && messageBox.isChecked())
        *prompt = false;
    return close;
}

static bool isAllowedTransition(RunControl::State from, RunControl::State to)
{
    switch (from) {
    case RunControl::State::Initialized:
        return to == RunControl::State::Starting;
    case RunControl::State::Starting:
        return to == RunControl::State::Running;
    case RunControl::State::Running:
        return to == RunControl::State::Stopping
            || to == RunControl::State::Stopped;
    case RunControl::State::Stopping:
        return to == RunControl::State::Stopped;
    case RunControl::State::Stopped:
        return false;
    }
    qDebug() << "UNKNOWN DEBUGGER STATE:" << from;
    return false;
}

void RunControl::setState(RunControl::State state)
{
    if (!isAllowedTransition(d->state, state)) {
        qDebug() << "Invalid run state transition from " << d->state << " to " << state;
    }
    d->state = state;
}

/*!
    Brings the application determined by this RunControl's \c applicationProcessHandle
    to the foreground.

    The default implementation raises the application on Mac, and does
    nothing elsewhere.
*/
void RunControl::bringApplicationToForeground()
{
#ifdef Q_OS_OSX
    d->foregroundCount = 0;
    bringApplicationToForegroundInternal();
#endif
}

void RunControl::reportApplicationStart()
{
    setState(State::Running);
    emit started(QPrivateSignal());
}

void RunControl::reportApplicationStop()
{
    if (d->state == State::Stopped) {
        // FIXME: Currently various tool implementations call reportApplicationStop()
        // multiple times. Fix it there and then add a soft assert here.
        return;
    }
    setState(State::Stopped);
    QTC_CHECK(d->applicationProcessHandle.isValid());
    setApplicationProcessHandle(Utils::ProcessHandle());
    emit finished(QPrivateSignal());
}

void RunControl::bringApplicationToForegroundInternal()
{
#ifdef Q_OS_OSX
    ProcessSerialNumber psn;
    GetProcessForPID(d->applicationProcessHandle.pid(), &psn);
    if (SetFrontProcess(&psn) == procNotFound && d->foregroundCount < 15) {
        // somehow the mac/carbon api says
        // "-600 no eligible process with specified process id"
        // if we call SetFrontProcess too early
        ++d->foregroundCount;
        QTimer::singleShot(200, this, &RunControl::bringApplicationToForegroundInternal);
    }
#endif
}

void RunControl::appendMessage(const QString &msg, Utils::OutputFormat format)
{
    emit appendMessageRequested(this, msg, format);
}

bool Runnable::canReUseOutputPane(const Runnable &other) const
{
    return d ? d->canReUseOutputPane(other.d) : (other.d.get() == 0);
}


// SimpleRunControlPrivate

namespace Internal {

class SimpleRunControlPrivate
{
public:
    ApplicationLauncher m_launcher;
};

} // Internal

// FIXME: Remove once ApplicationLauncher signalling does not depend on device.
static bool isSynchronousLauncher(RunControl *runControl)
{
    RunConfiguration *runConfig = runControl->runConfiguration();
    Target *target = runConfig ? runConfig->target() : nullptr;
    Kit *kit = target ? target->kit() : nullptr;
    Core::Id deviceId = DeviceTypeKitInformation::deviceTypeId(kit);
    return !deviceId.isValid() || deviceId == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

SimpleRunControl::SimpleRunControl(RunConfiguration *runConfiguration, Core::Id mode)
    : RunControl(runConfiguration, mode), d(new Internal::SimpleRunControlPrivate)
{
    setRunnable(runConfiguration->runnable());
    setIcon(Utils::Icons::RUN_SMALL_TOOLBAR);
}

SimpleRunControl::~SimpleRunControl()
{
    delete d;
}

ApplicationLauncher &SimpleRunControl::applicationLauncher()
{
    return d->m_launcher;
}

void SimpleRunControl::start()
{
    reportApplicationStart();
    d->m_launcher.disconnect(this);

    Runnable r = runnable();

    if (isSynchronousLauncher(this)) {

        connect(&d->m_launcher, &ApplicationLauncher::appendMessage,
                this, static_cast<void(RunControl::*)(const QString &, OutputFormat)>(&RunControl::appendMessage));
        connect(&d->m_launcher, &ApplicationLauncher::processStarted,
                this, &SimpleRunControl::onProcessStarted);
        connect(&d->m_launcher, &ApplicationLauncher::processExited,
                this, &SimpleRunControl::onProcessFinished);

        QTC_ASSERT(r.is<StandardRunnable>(), return);
        const QString executable = r.as<StandardRunnable>().executable;
        if (executable.isEmpty()) {
            appendMessage(RunControl::tr("No executable specified.") + '\n',
                          Utils::ErrorMessageFormat);
            reportApplicationStop();
        }  else if (!QFileInfo::exists(executable)) {
            appendMessage(RunControl::tr("Executable %1 does not exist.")
                          .arg(QDir::toNativeSeparators(executable)) + '\n',
                          Utils::ErrorMessageFormat);
            reportApplicationStop();
        } else {
            QString msg = RunControl::tr("Starting %1...").arg(QDir::toNativeSeparators(executable)) + '\n';
            appendMessage(msg, Utils::NormalMessageFormat);
            d->m_launcher.start(r);
            setApplicationProcessHandle(d->m_launcher.applicationPID());
        }

    } else {

        connect(&d->m_launcher, &ApplicationLauncher::reportError,
                this, [this](const QString &error) {
                    appendMessage(error, Utils::ErrorMessageFormat);
                });

        connect(&d->m_launcher, &ApplicationLauncher::remoteStderr,
                this, [this](const QByteArray &output) {
                    appendMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
                });

        connect(&d->m_launcher, &ApplicationLauncher::remoteStdout,
                this, [this](const QByteArray &output) {
                    appendMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
                });

        connect(&d->m_launcher, &ApplicationLauncher::finished,
                this, [this] {
                    d->m_launcher.disconnect(this);
                    reportApplicationStop();
                });

        connect(&d->m_launcher, &ApplicationLauncher::reportProgress,
                this, [this](const QString &progressString) {
                    appendMessage(progressString + '\n', Utils::NormalMessageFormat);
                });

        d->m_launcher.start(r, device());

    }
}

void SimpleRunControl::stop()
{
    d->m_launcher.stop();
}

void SimpleRunControl::onProcessStarted()
{
    // Console processes only know their pid after being started
    setApplicationProcessHandle(d->m_launcher.applicationPID());
    bringApplicationToForeground();
}

void SimpleRunControl::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QString msg;
    QString exe = runnable().as<StandardRunnable>().executable;
    if (status == QProcess::CrashExit)
        msg = tr("%1 crashed.").arg(QDir::toNativeSeparators(exe));
    else
        msg = tr("%1 exited with code %2").arg(QDir::toNativeSeparators(exe)).arg(exitCode);
    appendMessage(msg + QLatin1Char('\n'), Utils::NormalMessageFormat);
    reportApplicationStop();
}

// ToolRunner

ToolRunner::ToolRunner(RunControl *runControl)
    : m_runControl(runControl)
{
    runControl->setToolRunner(this);
}

RunControl *ToolRunner::runControl() const
{
    return m_runControl;
}

} // namespace ProjectExplorer
