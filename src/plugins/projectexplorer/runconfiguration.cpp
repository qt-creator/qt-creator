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

using namespace ProjectExplorer;

namespace {

const char USE_CPP_DEBUGGER_KEY[] = "RunConfiguration.UseCppDebugger";
const char USE_QML_DEBUGGER_KEY[] = "RunConfiguration.UseQmlDebugger";
const char USE_QML_DEBUGGER_AUTO_KEY[] = "RunConfiguration.UseQmlDebuggerAuto";
const char QML_DEBUG_SERVER_PORT_KEY[] = "RunConfiguration.QmlDebugServerPort";
const char USE_MULTIPROCESS_KEY[] = "RunConfiguration.UseMultiProcess";

} // namespace

/*!
    \class ProjectExplorer::ProcessHandle
    \brief  Helper class to describe a process.

    Encapsulates parameters of a running process, local (PID) or remote (to be done,
    address, port, etc).
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


RunConfigWidget *IRunConfigurationAspect::createConfigurationWidget()
{
    return 0;
}


/*!
    \class ProjectExplorer::RunConfiguration
    \brief  Base class for a run configuration. A run configuration specifies how a
    target should be run, while the runner (see below) does the actual running.

    Note that all RunControls and the target hold a shared pointer to the RunConfiguration.
    That is the lifetime of the RunConfiguration might exceed the life of the target.
    The user might still have a RunControl running (or output tab of that RunControl open)
    and yet unloaded the target.

    Also, a RunConfiguration might be already removed from the list of RunConfigurations
    for a target, but still be runnable via the output tab.
*/

RunConfiguration::RunConfiguration(Target *target, const Core::Id id) :
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
}

/*!
    \brief Used to find out whether a runconfiguration is enabled
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

bool RunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (isConfigured())
        return true;
    if (errorMessage)
        *errorMessage = tr("Unknown error.");
    return false;
}


/*!
    \fn virtual QWidget *ProjectExplorer::RunConfiguration::createConfigurationWidget()

    \brief Returns the widget used to configure this run configuration. Ownership is transferred to the caller
*/

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
        map.unite(aspect->toMap());

    return map;
}

ProjectExplorer::Abi RunConfiguration::abi() const
{
    BuildConfiguration *bc = target()->activeBuildConfiguration();
    if (!bc)
        return Abi::hostAbi();
    ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
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

    \brief Extra configuration aspect.

    Aspects are a mechanism to add RunControl-specific options to a RunConfiguration without
    subclassing the RunConfiguration for every addition, preventing a combinatorical explosion
    of subclasses or the need to add all options to the base class.
*/

/*!
    \brief Return extra aspects.

    \sa ProjectExplorer::IRunConfigurationAspect
*/

QList<IRunConfigurationAspect *> RunConfiguration::extraAspects() const
{
    QTC_ASSERT(m_aspectsInitialized, return QList<IRunConfigurationAspect *>());
    return m_aspects;
}

Utils::OutputFormatter *RunConfiguration::createOutputFormatter() const
{
    return new Utils::OutputFormatter();
}


/*!
    \class ProjectExplorer::IRunConfigurationFactory

    \brief Restores RunConfigurations from settings.

    The run configuration factory is used for restoring run configurations from
    settings. And used to create new runconfigurations in the "Run Settings" Dialog.
    For the first case, bool canRestore(Target *parent, const QString &id) and
    RunConfiguration* create(Target *parent, const QString &id) are used.
    For the second type, the functions QStringList availableCreationIds(Target *parent) and
    QString displayNameForType(const QString&) are used to generate a list of creatable
    RunConfigurations, and create(..) is used to create it.
*/

/*!
    \fn QStringList ProjectExplorer::IRunConfigurationFactory::availableCreationIds(Target *parent) const

    \brief Used to show the list of possible additons to a target, returns a list of types.
*/

/*!
    \fn QString ProjectExplorer::IRunConfigurationFactory::displayNameForId(const QString &id) const
    \brief Used to translate the types to names to display to the user.
*/

IRunConfigurationFactory::IRunConfigurationFactory(QObject *parent) :
    QObject(parent)
{
}

IRunConfigurationFactory::~IRunConfigurationFactory()
{
}

RunConfiguration *IRunConfigurationFactory::create(Target *parent, const Core::Id id)
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
    QList<IRunConfigurationFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<IRunConfigurationFactory>();
    foreach (IRunConfigurationFactory *factory, factories) {
        if (factory->canRestore(parent, map))
            return factory;
    }
    return 0;
}

IRunConfigurationFactory *IRunConfigurationFactory::find(Target *parent, RunConfiguration *rc)
{
    QList<IRunConfigurationFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<IRunConfigurationFactory>();
    foreach (IRunConfigurationFactory *factory, factories) {
        if (factory->canClone(parent, rc))
            return factory;
    }
    return 0;
}

QList<IRunConfigurationFactory *> IRunConfigurationFactory::find(Target *parent)
{
    QList<IRunConfigurationFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<IRunConfigurationFactory>();
    QList<IRunConfigurationFactory *> result;
    foreach (IRunConfigurationFactory *factory, factories) {
        if (!factory->availableCreationIds(parent).isEmpty())
            result << factory;
    }
    return result;
}

/*!
    \class ProjectExplorer::IRunControlFactory

    \brief Creates RunControl objects matching a RunConfiguration
*/

/*!
    \fn IRunConfigurationAspect *ProjectExplorer::IRunControlFactory::createRunConfigurationAspect()
    \brief Return an IRunConfigurationAspect to carry options for RunControls this factory can create.

    If no extra options are required it is allowed to return null like the default implementation does.
    This is intended to be called from the RunConfiguration constructor, so passing a RunConfiguration
    pointer makes no sense because that object is under construction at the time.
*/

/*!
    \fn RunConfigWidget *ProjectExplorer::IRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)

    \brief Return a widget used to configure this runner. Ownership is transferred to the caller.

    Return 0 if @p runConfiguration is not suitable for RunControls from this factory, or no user-accessible
    configuration is required.
*/

IRunControlFactory::IRunControlFactory(QObject *parent)
    : QObject(parent)
{
}

IRunControlFactory::~IRunControlFactory()
{
}

IRunConfigurationAspect *IRunControlFactory::createRunConfigurationAspect(RunConfiguration *rc)
{
    Q_UNUSED(rc);
    return 0;
}

/*!
    \class ProjectExplorer::RunControl
    \brief Each instance of this class represents one item that is run.
*/

/*!
    \fn bool ProjectExplorer::RunControl::promptToStop(bool *optionalPrompt = 0) const

    \brief Prompt to stop. If 'optionalPrompt' is passed, a "Do not ask again"-
    checkbox will show and the result will be returned in '*optionalPrompt'.
*/

/*!
    \fn QIcon ProjectExplorer::RunControl::icon() const
    \brief Eeturns the icon to be shown in the Outputwindow.

    TODO the icon differs currently only per "mode", so this is more flexible then it needs to be.
*/

RunControl::RunControl(RunConfiguration *runConfiguration, RunMode mode)
    : m_runMode(mode), m_runConfiguration(runConfiguration), m_outputFormatter(0)
{
    if (runConfiguration) {
        m_displayName  = runConfiguration->displayName();
        m_outputFormatter = runConfiguration->createOutputFormatter();
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

RunMode RunControl::runMode() const
{
    return m_runMode;
}

QString RunControl::displayName() const
{
    return m_displayName;
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
    \brief Utility to prompt to terminate application with checkable box.
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
        messageBox.setCheckBoxText(tr("Do not ask again"));
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
