/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "runconfiguration.h"

#include "project.h"
#include "target.h"
#include "toolchain.h"
#include "abi.h"
#include "buildconfiguration.h"
#include "kitinformation.h"
#include <extensionsystem/pluginmanager.h>

#include <utils/outputformatter.h>
#include <utils/checkablemessagebox.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>

#include <QTimer>
#include <QPushButton>

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::ProcessHandle
    \brief The ProcessHandle class is a helper class to describe a process.

    Encapsulates parameters of a running process, local (PID) or remote (to be
    done, address, port, and so on).
*/

ProcessHandle::ProcessHandle(quint64 pid) :
    m_pid(pid)
{
}

bool ProcessHandle::isValid() const
{
    return m_pid != 0;
}

void ProcessHandle::setPid(quint64 pid)
{
    m_pid = pid;
}

quint64 ProcessHandle::pid() const
{
    return m_pid;
}

QString ProcessHandle::toString() const
{
    if (m_pid)
        return RunControl::tr("PID %1").arg(m_pid);
    //: Invalid process handle.
    return RunControl::tr("Invalid");
}

bool ProcessHandle::equals(const ProcessHandle &rhs) const
{
    return m_pid == rhs.m_pid;
}

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

IRunConfigurationAspect::IRunConfigurationAspect(RunConfiguration *runConfig)
{
    m_runConfiguration = runConfig;
    m_projectSettings = 0;
    m_globalSettings = 0;
    m_useGlobalSettings = false;
}

IRunConfigurationAspect::~IRunConfigurationAspect()
{
    delete m_projectSettings;
}

/*!
    Returns the widget used to configure this run configuration. Ownership is
    transferred to the caller.
*/

RunConfigWidget *IRunConfigurationAspect::createConfigurationWidget()
{
    return 0;
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
    ProjectConfiguration(target, id),
    m_aspectsInitialized(false)
{
    Q_ASSERT(target);
    ctor();
}

RunConfiguration::RunConfiguration(Target *target, RunConfiguration *source) :
    ProjectConfiguration(target, source),
    m_aspectsInitialized(true)
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
    if (m_aspectsInitialized)
        return;

    foreach (IRunControlFactory *factory, ExtensionSystem::PluginManager::getObjects<IRunControlFactory>())
        addExtraAspect(factory->createRunConfigurationAspect(this));
    m_aspectsInitialized = true;
}

void RunConfiguration::addExtraAspect(IRunConfigurationAspect *aspect)
{
    if (aspect)
        m_aspects += aspect;
}

void RunConfiguration::ctor()
{
    connect(this, SIGNAL(enabledChanged()), this, SIGNAL(requestRunActionsUpdate()));

    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Run Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([this]() -> Utils::MacroExpander * {
        BuildConfiguration *bc = target()->activeBuildConfiguration();
        return bc ? bc->macroExpander() : target()->macroExpander();
    });
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
        return 0;
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
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    if (!tc)
        return Abi::hostAbi();
    return tc->targetAbi();
}

bool RunConfiguration::fromMap(const QVariantMap &map)
{
    addExtraAspects();

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
    QTC_ASSERT(m_aspectsInitialized, return QList<IRunConfigurationAspect *>());
    return m_aspects;
}
IRunConfigurationAspect *RunConfiguration::extraAspect(Core::Id id) const
{
    QTC_ASSERT(m_aspectsInitialized, return 0);
    foreach (IRunConfigurationAspect *aspect, m_aspects)
        if (aspect->id() == id)
            return aspect;
    return 0;
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

IRunConfigurationFactory::~IRunConfigurationFactory()
{
}

RunConfiguration *IRunConfigurationFactory::create(Target *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    RunConfiguration *rc = doCreate(parent, id);
    if (!rc)
        return 0;
    rc->addExtraAspects();
    return rc;
}

RunConfiguration *IRunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    RunConfiguration *rc = doRestore(parent, map);
    if (!rc->fromMap(map)) {
        delete rc;
        rc = 0;
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

    Returns 0 if @p \a runConfiguration is not suitable for RunControls from this
    factory, or no user-accessible
    configuration is required.
*/

IRunControlFactory::IRunControlFactory(QObject *parent)
    : QObject(parent)
{
}

IRunControlFactory::~IRunControlFactory()
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
    return 0;
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

RunControl::RunControl(RunConfiguration *runConfiguration, Core::Id mode)
    : m_runMode(mode), m_runConfiguration(runConfiguration), m_outputFormatter(0)
{
    if (runConfiguration) {
        m_displayName  = runConfiguration->displayName();
        m_outputFormatter = runConfiguration->createOutputFormatter();

        if (runConfiguration->target())
            m_project = m_runConfiguration->target()->project();
    }

    // We need to ensure that there's always a OutputFormatter
    if (!m_outputFormatter)
        m_outputFormatter = new Utils::OutputFormatter();
}

RunControl::~RunControl()
{
    delete m_outputFormatter;
}

Utils::OutputFormatter *RunControl::outputFormatter()
{
    return m_outputFormatter;
}

Core::Id RunControl::runMode() const
{
    return m_runMode;
}

QString RunControl::displayName() const
{
    return m_displayName;
}

void RunControl::setIcon(const Utils::Icon &icon)
{
    m_icon = icon;
}

Utils::Icon RunControl::icon() const
{
    return m_icon;
}

Abi RunControl::abi() const
{
    if (const RunConfiguration *rc = m_runConfiguration.data())
        return rc->abi();
    return Abi();
}

RunConfiguration *RunControl::runConfiguration() const
{
    return m_runConfiguration.data();
}

Project *RunControl::project() const
{
    return m_project.data();
}

ProcessHandle RunControl::applicationProcessHandle() const
{
    return m_applicationProcessHandle;
}

void RunControl::setApplicationProcessHandle(const ProcessHandle &handle)
{
    if (m_applicationProcessHandle != handle) {
        m_applicationProcessHandle = handle;
        emit applicationProcessHandleChanged();
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
                                  tr("Force Quit"), tr("Keep Running"),
                                  optionalPrompt);
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

bool RunControl::sameRunConfiguration(const RunControl *other) const
{
    return other->m_runConfiguration.data() == m_runConfiguration.data();
}

void RunControl::bringApplicationToForeground(qint64 pid)
{
#ifdef Q_OS_MAC
    m_internalPid = pid;
    m_foregroundCount = 0;
    bringApplicationToForegroundInternal();
#else
    Q_UNUSED(pid)
#endif
}

void RunControl::bringApplicationToForegroundInternal()
{
#ifdef Q_OS_MAC
    ProcessSerialNumber psn;
    GetProcessForPID(m_internalPid, &psn);
    if (SetFrontProcess(&psn) == procNotFound && m_foregroundCount < 15) {
        // somehow the mac/carbon api says
        // "-600 no eligible process with specified process id"
        // if we call SetFrontProcess too early
        ++m_foregroundCount;
        QTimer::singleShot(200, this, SLOT(bringApplicationToForegroundInternal()));
        return;
    }
#endif
}

void RunControl::appendMessage(const QString &msg, Utils::OutputFormat format)
{
    emit appendMessage(this, msg, format);
}

} // namespace ProjectExplorer
