/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "customexecutablerunconfiguration.h"
#include "customexecutableconfigurationwidget.h"
#include "debugginghelper.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/abi.h>

#include <coreplugin/icore.h>

#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

#include <QDir>

using namespace QtSupport;
using namespace QtSupport::Internal;

namespace {
const char * const CUSTOM_EXECUTABLE_ID("ProjectExplorer.CustomExecutableRunConfiguration");

const char * const EXECUTABLE_KEY("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
const char * const ARGUMENTS_KEY("ProjectExplorer.CustomExecutableRunConfiguration.Arguments");
const char * const WORKING_DIRECTORY_KEY("ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory");
const char * const USE_TERMINAL_KEY("ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal");
const char * const USER_ENVIRONMENT_CHANGES_KEY("ProjectExplorer.CustomExecutableRunConfiguration.UserEnvironmentChanges");
const char * const BASE_ENVIRONMENT_BASE_KEY("ProjectExplorer.CustomExecutableRunConfiguration.BaseEnvironmentBase");
}

void CustomExecutableRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());

    connect(target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(activeBuildConfigurationChanged()));

    m_lastActiveBuildConfiguration = activeBuildConfiguration();

    if (m_lastActiveBuildConfiguration) {
        connect(m_lastActiveBuildConfiguration, SIGNAL(environmentChanged()),
                this, SIGNAL(baseEnvironmentChanged()));
    }
}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(ProjectExplorer::Target *parent) :
    ProjectExplorer::LocalApplicationRunConfiguration(parent, Core::Id(CUSTOM_EXECUTABLE_ID)),
    m_workingDirectory(QLatin1String(ProjectExplorer::Constants::DEFAULT_WORKING_DIR)),
    m_runMode(Gui),
    m_baseEnvironmentBase(CustomExecutableRunConfiguration::BuildEnvironmentBase)
{
    ctor();
}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(ProjectExplorer::Target *parent,
                                                                   CustomExecutableRunConfiguration *source) :
    ProjectExplorer::LocalApplicationRunConfiguration(parent, source),
    m_executable(source->m_executable),
    m_workingDirectory(source->m_workingDirectory),
    m_cmdArguments(source->m_cmdArguments),
    m_runMode(source->m_runMode),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_baseEnvironmentBase(source->m_baseEnvironmentBase)
{
    ctor();
}

// Note: Qt4Project deletes all empty customexecrunconfigs for which isConfigured() == false.
CustomExecutableRunConfiguration::~CustomExecutableRunConfiguration()
{
}

void CustomExecutableRunConfiguration::activeBuildConfigurationChanged()
{
    if (m_lastActiveBuildConfiguration) {
        disconnect(m_lastActiveBuildConfiguration, SIGNAL(environmentChanged()),
                   this, SIGNAL(baseEnvironmentChanged()));
    }
    m_lastActiveBuildConfiguration = activeBuildConfiguration();
    if (m_lastActiveBuildConfiguration) {
        connect(m_lastActiveBuildConfiguration, SIGNAL(environmentChanged()),
                this, SIGNAL(baseEnvironmentChanged()));
    }
}

QString CustomExecutableRunConfiguration::executable() const
{
    Utils::Environment env = environment();
    QString exec = env.searchInPath(Utils::expandMacros(m_executable, macroExpander()),
                                    QStringList() << workingDirectory());

    if (exec.isEmpty() || !QFileInfo(exec).exists()) {
        // Oh the executable doesn't exists, ask the user.
        CustomExecutableRunConfiguration *that = const_cast<CustomExecutableRunConfiguration *>(this);
        QWidget *confWidget = that->createConfigurationWidget();
        confWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        QDialog dialog(Core::ICore::mainWindow());
        dialog.setWindowTitle(displayName());
        dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
        dialog.setLayout(new QVBoxLayout());
        QLabel *label = new QLabel(tr("Could not find the executable, please specify one."));
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        dialog.layout()->addWidget(label);
        dialog.layout()->addWidget(confWidget);
        QDialogButtonBox *dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(dbb, SIGNAL(accepted()), &dialog, SLOT(accept()));
        connect(dbb, SIGNAL(rejected()), &dialog, SLOT(reject()));
        dialog.layout()->addWidget(dbb);
        dialog.layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);

        QString oldExecutable = m_executable;
        QString oldWorkingDirectory = m_workingDirectory;
        QString oldCmdArguments = m_cmdArguments;

        if (dialog.exec() == QDialog::Accepted) {
            return executable();
        } else {
            // Restore values changed by the configuration widget.
            if (that->m_executable != oldExecutable
                || that->m_workingDirectory != oldWorkingDirectory
                || that->m_cmdArguments != oldCmdArguments) {
                that->m_executable = oldExecutable;
                that->m_workingDirectory = oldWorkingDirectory;
                that->m_cmdArguments = oldCmdArguments;
                emit that->changed();
            }
            return QString();
        }
    }
    return QDir::cleanPath(exec);
}

QString CustomExecutableRunConfiguration::rawExecutable() const
{
    return m_executable;
}

bool CustomExecutableRunConfiguration::isConfigured() const
{
    return !m_executable.isEmpty();
}

ProjectExplorer::LocalApplicationRunConfiguration::RunMode CustomExecutableRunConfiguration::runMode() const
{
    return m_runMode;
}

QString CustomExecutableRunConfiguration::workingDirectory() const
{
    return QDir::cleanPath(environment().expandVariables(
                Utils::expandMacros(baseWorkingDirectory(), macroExpander())));
}

QString CustomExecutableRunConfiguration::baseWorkingDirectory() const
{
    return m_workingDirectory;
}


QString CustomExecutableRunConfiguration::commandLineArguments() const
{
    return Utils::QtcProcess::expandMacros(m_cmdArguments, macroExpander());
}

QString CustomExecutableRunConfiguration::rawCommandLineArguments() const
{
    return m_cmdArguments;
}

QString CustomExecutableRunConfiguration::baseEnvironmentText() const
{
    if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::CleanEnvironmentBase) {
        return tr("Clean Environment");
    } else  if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::SystemEnvironmentBase) {
        return tr("System Environment");
    } else  if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::BuildEnvironmentBase) {
        return tr("Build Environment");
    }
    return QString();
}

Utils::Environment CustomExecutableRunConfiguration::baseEnvironment() const
{
    Utils::Environment env;
    if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::CleanEnvironmentBase) {
        // Nothing
    } else  if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::SystemEnvironmentBase) {
        env = Utils::Environment::systemEnvironment();
    } else  if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::BuildEnvironmentBase) {
        if (activeBuildConfiguration())
            env = activeBuildConfiguration()->environment();
    }
    return env;
}

void CustomExecutableRunConfiguration::setBaseEnvironmentBase(BaseEnvironmentBase env)
{
    if (m_baseEnvironmentBase == env)
        return;
    m_baseEnvironmentBase = env;
    emit baseEnvironmentChanged();
}

CustomExecutableRunConfiguration::BaseEnvironmentBase CustomExecutableRunConfiguration::baseEnvironmentBase() const
{
    return m_baseEnvironmentBase;
}

Utils::Environment CustomExecutableRunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

QList<Utils::EnvironmentItem> CustomExecutableRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void CustomExecutableRunConfiguration::setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

QString CustomExecutableRunConfiguration::defaultDisplayName() const
{
    if (m_executable.isEmpty())
        return tr("Custom Executable");
    else
        return tr("Run %1").arg(QDir::toNativeSeparators(m_executable));
}

QVariantMap CustomExecutableRunConfiguration::toMap() const
{
    QVariantMap map(LocalApplicationRunConfiguration::toMap());
    map.insert(QLatin1String(EXECUTABLE_KEY), m_executable);
    map.insert(QLatin1String(ARGUMENTS_KEY), m_cmdArguments);
    map.insert(QLatin1String(WORKING_DIRECTORY_KEY), m_workingDirectory);
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == Console);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), static_cast<int>(m_baseEnvironmentBase));
    return map;
}

bool CustomExecutableRunConfiguration::fromMap(const QVariantMap &map)
{
    m_executable = map.value(QLatin1String(EXECUTABLE_KEY)).toString();
    m_cmdArguments = map.value(QLatin1String(ARGUMENTS_KEY)).toString();
    m_workingDirectory = map.value(QLatin1String(WORKING_DIRECTORY_KEY)).toString();
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY)).toBool() ? Console : Gui;
    m_userEnvironmentChanges = Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase>(map.value(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), static_cast<int>(CustomExecutableRunConfiguration::BuildEnvironmentBase)).toInt());

    setDefaultDisplayName(defaultDisplayName());
    return LocalApplicationRunConfiguration::fromMap(map);
}

void CustomExecutableRunConfiguration::setExecutable(const QString &executable)
{
    if (executable == m_executable)
        return;
    m_executable = executable;
    setDefaultDisplayName(defaultDisplayName());
    emit changed();
}

void CustomExecutableRunConfiguration::setCommandLineArguments(const QString &commandLineArguments)
{
    m_cmdArguments = commandLineArguments;
    emit changed();
}

void CustomExecutableRunConfiguration::setBaseWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
    emit changed();
}

void CustomExecutableRunConfiguration::setRunMode(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode)
{
    m_runMode = runMode;
    emit changed();
}

QWidget *CustomExecutableRunConfiguration::createConfigurationWidget()
{
    return new CustomExecutableConfigurationWidget(this);
}

QString CustomExecutableRunConfiguration::dumperLibrary() const
{
    Utils::FileName qmakePath = DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    return DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
}

QStringList CustomExecutableRunConfiguration::dumperLibraryLocations() const
{
    Utils::FileName qmakePath = DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    return DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);
}

ProjectExplorer::Abi CustomExecutableRunConfiguration::abi() const
{
    return ProjectExplorer::Abi(); // return an invalid ABI: We do not know what we will end up running!
}

// Factory

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("CustomExecutableRunConfigurationFactory")); }

CustomExecutableRunConfigurationFactory::~CustomExecutableRunConfigurationFactory()
{ }

bool CustomExecutableRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                                        const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == Core::Id(CUSTOM_EXECUTABLE_ID);
}

ProjectExplorer::RunConfiguration *
CustomExecutableRunConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    return new CustomExecutableRunConfiguration(parent);
}

bool CustomExecutableRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent,
                                                         const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    Core::Id id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

ProjectExplorer::RunConfiguration *
CustomExecutableRunConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CustomExecutableRunConfiguration *rc(new CustomExecutableRunConfiguration(parent));
    if (rc->fromMap(map))
        return rc;
    delete rc;
    return 0;
}

bool CustomExecutableRunConfigurationFactory::canClone(ProjectExplorer::Target *parent,
                                                       ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *
CustomExecutableRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                               ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new CustomExecutableRunConfiguration(parent, static_cast<CustomExecutableRunConfiguration*>(source));
}

bool CustomExecutableRunConfigurationFactory::canHandle(ProjectExplorer::Target *parent) const
{
    return parent->project()->supportsProfile(parent->profile());
}

QList<Core::Id> CustomExecutableRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(CUSTOM_EXECUTABLE_ID);
}

QString CustomExecutableRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(CUSTOM_EXECUTABLE_ID))
        return tr("Custom Executable");
    return QString();
}
