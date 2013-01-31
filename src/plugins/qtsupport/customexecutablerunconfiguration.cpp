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
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

#include <QDir>

using namespace QtSupport;
using namespace QtSupport::Internal;

namespace {
const char CUSTOM_EXECUTABLE_ID[] = "ProjectExplorer.CustomExecutableRunConfiguration";

const char EXECUTABLE_KEY[] = "ProjectExplorer.CustomExecutableRunConfiguration.Executable";
const char ARGUMENTS_KEY[] = "ProjectExplorer.CustomExecutableRunConfiguration.Arguments";
const char WORKING_DIRECTORY_KEY[] = "ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory";
const char USE_TERMINAL_KEY[] = "ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal";
const char USER_ENVIRONMENT_CHANGES_KEY[] = "ProjectExplorer.CustomExecutableRunConfiguration.UserEnvironmentChanges";
const char BASE_ENVIRONMENT_BASE_KEY[] = "ProjectExplorer.CustomExecutableRunConfiguration.BaseEnvironmentBase";
}

void CustomExecutableRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());

    connect(target(), SIGNAL(environmentChanged()),
            this, SIGNAL(baseEnvironmentChanged()));
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

// Dialog embedding the CustomExecutableConfigurationWidget
// prompting the user to complete the configuration.
class CustomExecutableDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CustomExecutableDialog(CustomExecutableRunConfiguration *rc, QWidget *parent = 0);

private slots:
    void changed()
    {
        setOkButtonEnabled(m_runConfiguration->isConfigured());
    }

private:
    inline void setOkButtonEnabled(bool e)
    {
        m_dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(e);
    }

    QDialogButtonBox *m_dialogButtonBox;
    CustomExecutableRunConfiguration *m_runConfiguration;
};

CustomExecutableDialog::CustomExecutableDialog(CustomExecutableRunConfiguration *rc, QWidget *parent)
    : QDialog(parent)
    , m_dialogButtonBox(new  QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
    , m_runConfiguration(rc)
{
    connect(rc, SIGNAL(changed()), this, SLOT(changed()));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Could not find the executable, please specify one."));
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    layout->addWidget(label);
    QWidget *configWidget = rc->createConfigurationWidget();
    configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    layout->addWidget(configWidget);
    setOkButtonEnabled(false);
    connect(m_dialogButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(m_dialogButtonBox);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
}

bool CustomExecutableRunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (isConfigured())
        return validateExecutable(0, errorMessage);
    CustomExecutableDialog dialog(this, Core::ICore::mainWindow());
    dialog.setWindowTitle(displayName());
    const QString oldExecutable = m_executable;
    const QString oldWorkingDirectory = m_workingDirectory;
    const QString oldCmdArguments = m_cmdArguments;
    if (dialog.exec() == QDialog::Accepted)
        return validateExecutable(0, errorMessage);
    // User canceled: Hack: Silence the error dialog.
    if (errorMessage)
        *errorMessage = QLatin1String("");
    // Restore values changed by the configuration widget.
    if (m_executable != oldExecutable
        || m_workingDirectory != oldWorkingDirectory
        || m_cmdArguments != oldCmdArguments) {
        m_executable = oldExecutable;
        m_workingDirectory = oldWorkingDirectory;
        m_cmdArguments = oldCmdArguments;
        emit changed();
    }
    return false;
}

// Search the executable in the path.
bool CustomExecutableRunConfiguration::validateExecutable(QString *executable, QString *errorMessage) const
{
    if (executable)
        executable->clear();
    if (m_executable.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("No executable.");
        return false;
    }
    const Utils::Environment env = environment();
    const QString exec = env.searchInPath(Utils::expandMacros(m_executable, macroExpander()),
                                          QStringList(workingDirectory()));
    if (exec.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The executable\n%1\ncannot be found in the path.").
                            arg(QDir::toNativeSeparators(m_executable));
        return false;
    }
    if (executable)
        *executable = QDir::cleanPath(exec);
    return true;
}

QString CustomExecutableRunConfiguration::executable() const
{
    QString result;
    validateExecutable(&result);
    return result;
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
        else
            env = Utils::Environment::systemEnvironment(); // fall back
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
    return id == CUSTOM_EXECUTABLE_ID;
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
    return parent->project()->supportsKit(parent->kit());
}

QList<Core::Id> CustomExecutableRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(CUSTOM_EXECUTABLE_ID);
}

QString CustomExecutableRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == CUSTOM_EXECUTABLE_ID)
        return tr("Custom Executable");
    return QString();
}

#include "customexecutablerunconfiguration.moc"
