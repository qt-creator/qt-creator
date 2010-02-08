/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "customexecutablerunconfiguration.h"
#include "environment.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/environmenteditmodel.h>
#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/target.h>
#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFormLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const char * const CUSTOM_EXECUTABLE_ID("ProjectExplorer.CustomExecutableRunConfiguration");

const char * const EXECUTABLE_KEY("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
const char * const ARGUMENTS_KEY("ProjectExplorer.CustomExecutableRunConfiguration.Arguments");
const char * const WORKING_DIRECTORY_KEY("ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory");
const char * const USE_TERMINAL_KEY("ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal");
const char * const USER_SET_NAME_KEY("ProjectExplorer.CustomExecutableRunConfiguration.UserSetName");
const char * const USER_NAME_KEY("ProjectExplorer.CustomExecutableRunConfiguration.UserName");
const char * const USER_ENVIRONMENT_CHANGES_KEY("ProjectExplorer.CustomExecutableRunConfiguration.UserEnvironmentChanges");
const char * const BASE_ENVIRONMENT_BASE_KEY("ProjectExplorer.CustomExecutableRunConfiguration.BaseEnvironmentBase");

const char * const DEFAULT_WORKING_DIR("$BUILDDIR");
}

class CustomDirectoryPathChooser : public Utils::PathChooser
{
public:
    CustomDirectoryPathChooser(QWidget *parent)
        : Utils::PathChooser(parent)
    {
    }
    virtual bool validatePath(const QString &path, QString *errorMessage = 0)
    {
        Q_UNUSED(path)
        Q_UNUSED(errorMessage)
        return true;
    }
};

CustomExecutableConfigurationWidget::CustomExecutableConfigurationWidget(CustomExecutableRunConfiguration *rc)
    : m_ignoreChange(false), m_runConfiguration(rc)
{
    QFormLayout *layout = new QFormLayout;
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setMargin(0);

    m_userName = new QLineEdit(this);
    layout->addRow(tr("Name:"), m_userName);

    m_executableChooser = new Utils::PathChooser(this);
    m_executableChooser->setExpectedKind(Utils::PathChooser::Command);
    layout->addRow(tr("Executable:"), m_executableChooser);

    m_commandLineArgumentsLineEdit = new QLineEdit(this);
    m_commandLineArgumentsLineEdit->setMinimumWidth(200); // this shouldn't be fixed here...
    layout->addRow(tr("Arguments:"), m_commandLineArgumentsLineEdit);

    m_workingDirectory = new CustomDirectoryPathChooser(this);
    m_workingDirectory->setExpectedKind(Utils::PathChooser::Directory);
    layout->addRow(tr("Working Directory:"), m_workingDirectory);

    m_useTerminalCheck = new QCheckBox(tr("Run in &Terminal"), this);
    layout->addRow(QString(), m_useTerminalCheck);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);

    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    detailsWidget->setLayout(layout);

    QLabel *environmentLabel = new QLabel(this);
    environmentLabel->setText(tr("Run Environment"));
    QFont f = environmentLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() *1.2);
    environmentLabel->setFont(f);
    vbox->addWidget(environmentLabel);

    QWidget *baseEnvironmentWidget = new QWidget;
    QHBoxLayout *baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    QLabel *label = new QLabel(tr("Base environment for this runconfiguration:"), this);
    baseEnvironmentLayout->addWidget(label);
    m_baseEnvironmentComboBox = new QComboBox(this);
    m_baseEnvironmentComboBox->addItems(QStringList()
                                        << tr("Clean Environment")
                                        << tr("System Environment")
                                        << tr("Build Environment"));
    m_baseEnvironmentComboBox->setCurrentIndex(rc->baseEnvironmentBase());
    connect(m_baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(baseEnvironmentSelected(int)));
    baseEnvironmentLayout->addWidget(m_baseEnvironmentComboBox);
    baseEnvironmentLayout->addStretch(10);

    m_environmentWidget = new EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(rc->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(rc->baseEnvironmentText());
    m_environmentWidget->setUserChanges(rc->userEnvironmentChanges());
    vbox->addWidget(m_environmentWidget);

    changed();

    connect(m_userName, SIGNAL(textEdited(QString)),
            this, SLOT(userNameEdited(QString)));
    connect(m_executableChooser, SIGNAL(changed(QString)),
            this, SLOT(executableEdited()));
    connect(m_commandLineArgumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(argumentsEdited(const QString&)));
    connect(m_workingDirectory, SIGNAL(changed(QString)),
            this, SLOT(workingDirectoryEdited()));
    connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
            this, SLOT(termToggled(bool)));

    connect(m_runConfiguration, SIGNAL(changed()), this, SLOT(changed()));

    connect(m_environmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(userChangesChanged()));

    connect(m_runConfiguration, SIGNAL(baseEnvironmentChanged()),
            this, SLOT(baseEnvironmentChanged()));
    connect(m_runConfiguration, SIGNAL(userEnvironmentChangesChanged(QList<ProjectExplorer::EnvironmentItem>)),
            this, SLOT(userEnvironmentChangesChanged()));
}

void CustomExecutableConfigurationWidget::userChangesChanged()
{
    m_runConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
}

void CustomExecutableConfigurationWidget::baseEnvironmentSelected(int index)
{
    m_ignoreChange = true;
    m_runConfiguration->setBaseEnvironmentBase(CustomExecutableRunConfiguration::BaseEnvironmentBase(index));

    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::baseEnvironmentChanged()
{
    if (m_ignoreChange)
        return;

    int index = CustomExecutableRunConfiguration::BaseEnvironmentBase(
            m_runConfiguration->baseEnvironmentBase());
    m_baseEnvironmentComboBox->setCurrentIndex(index);
    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
}

void CustomExecutableConfigurationWidget::userEnvironmentChangesChanged()
{
    m_environmentWidget->setUserChanges(m_runConfiguration->userEnvironmentChanges());
}


void CustomExecutableConfigurationWidget::executableEdited()
{
    m_ignoreChange = true;
    m_runConfiguration->setExecutable(m_executableChooser->path());
    m_ignoreChange = false;
}
void CustomExecutableConfigurationWidget::argumentsEdited(const QString &arguments)
{
    m_ignoreChange = true;
    m_runConfiguration->setCommandLineArguments(arguments);
    m_ignoreChange = false;
}
void CustomExecutableConfigurationWidget::workingDirectoryEdited()
{
    m_ignoreChange = true;
    m_runConfiguration->setWorkingDirectory(m_workingDirectory->path());
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::userNameEdited(const QString &name)
{
    m_ignoreChange = true;
    m_runConfiguration->setUserName(name);
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::termToggled(bool on)
{
    m_ignoreChange = true;
    m_runConfiguration->setRunMode(on ? LocalApplicationRunConfiguration::Console
                                      : LocalApplicationRunConfiguration::Gui);
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::changed()
{
    const QString &executable = m_runConfiguration->baseExecutable();
    QString text = tr("No Executable specified.");
    if (!executable.isEmpty())
        text = tr("Running executable: <b>%1</b> %2").
               arg(executable,
                   ProjectExplorer::Environment::joinArgumentList(m_runConfiguration->commandLineArguments()));

    // We triggered the change, don't update us
    if (m_ignoreChange)
        return;
    m_executableChooser->setPath(executable);
    m_commandLineArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(m_runConfiguration->commandLineArguments()));
    m_workingDirectory->setPath(m_runConfiguration->baseWorkingDirectory());
    m_useTerminalCheck->setChecked(m_runConfiguration->runMode() == LocalApplicationRunConfiguration::Console);
    m_userName->setText(m_runConfiguration->userName());
}

void CustomExecutableRunConfiguration::ctor()
{
    if (m_userSetName)
        setDisplayName(m_userName);
    else
        setDisplayName(tr("Custom Executable"));
    connect(target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(activeBuildConfigurationChanged()));

    m_lastActiveBuildConfiguration = activeBuildConfiguration();

    if (m_lastActiveBuildConfiguration) {
        connect(m_lastActiveBuildConfiguration, SIGNAL(environmentChanged()),
                this, SIGNAL(baseEnvironmentChanged()));
    }
}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *parent) :
    LocalApplicationRunConfiguration(parent, QLatin1String(CUSTOM_EXECUTABLE_ID)),
    m_runMode(Gui),
    m_userSetName(false),
    m_baseEnvironmentBase(CustomExecutableRunConfiguration::BuildEnvironmentBase)
{
    m_workingDirectory = QLatin1String(DEFAULT_WORKING_DIR);
    ctor();
}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *parent, CustomExecutableRunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_executable(source->m_executable),
    m_workingDirectory(source->m_workingDirectory),
    m_cmdArguments(source->m_cmdArguments),
    m_runMode(source->m_runMode),
    m_userSetName(source->m_userSetName),
    m_userName(source->m_userName),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_baseEnvironmentBase(source->m_baseEnvironmentBase)
{
    ctor();
}

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

QString CustomExecutableRunConfiguration::baseExecutable() const
{
    return m_executable;
}

QString CustomExecutableRunConfiguration::userName() const
{
    return m_userName;
}

QString CustomExecutableRunConfiguration::executable() const
{
    QString exec;
    if (QDir::isRelativePath(m_executable)) {
        Environment env = activeBuildConfiguration()->environment();
        exec = env.searchInPath(m_executable);
        if (exec.isEmpty())
            exec = QDir::cleanPath(workingDirectory() + QLatin1Char('/') + m_executable);
    } else {
        exec = m_executable;
    }

    if (!QFileInfo(exec).exists()) {
        // Oh the executable doesn't exists, ask the user.
        QWidget *confWidget = const_cast<CustomExecutableRunConfiguration *>(this)->configurationWidget();
        QDialog dialog(Core::ICore::instance()->mainWindow());
        dialog.setLayout(new QVBoxLayout());
        dialog.layout()->addWidget(new QLabel(tr("Could not find the executable, please specify one.")));
        dialog.layout()->addWidget(confWidget);
        QDialogButtonBox *dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(dbb, SIGNAL(accepted()), &dialog, SLOT(accept()));
        connect(dbb, SIGNAL(rejected()), &dialog, SLOT(reject()));
        dialog.layout()->addWidget(dbb);

        QString oldExecutable = m_executable;
        QString oldWorkingDirectory = m_workingDirectory;
        QStringList oldCmdArguments = m_cmdArguments;

        if (dialog.exec()) {
            return executable();
        } else {
            CustomExecutableRunConfiguration *that = const_cast<CustomExecutableRunConfiguration *>(this);
            that->m_executable = oldExecutable;
            that->m_workingDirectory = oldWorkingDirectory;
            that->m_cmdArguments = oldCmdArguments;
            emit that->changed();
            return QString();
        }
    }
    return exec;
}

LocalApplicationRunConfiguration::RunMode CustomExecutableRunConfiguration::runMode() const
{
    return m_runMode;
}

QString CustomExecutableRunConfiguration::baseWorkingDirectory() const
{
    return m_workingDirectory;
}

QString CustomExecutableRunConfiguration::workingDirectory() const
{
    QString wd = m_workingDirectory;
    QString bd = activeBuildConfiguration()->buildDirectory();
    return wd.replace("$BUILDDIR", QDir::cleanPath(bd));
}

QStringList CustomExecutableRunConfiguration::commandLineArguments() const
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

ProjectExplorer::Environment CustomExecutableRunConfiguration::baseEnvironment() const
{
    ProjectExplorer::Environment env;
    if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::CleanEnvironmentBase) {
        // Nothing
    } else  if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::SystemEnvironmentBase) {
        env = ProjectExplorer::Environment::systemEnvironment();
    } else  if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::BuildEnvironmentBase) {
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

ProjectExplorer::Environment CustomExecutableRunConfiguration::environment() const
{
    ProjectExplorer::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

QList<ProjectExplorer::EnvironmentItem> CustomExecutableRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void CustomExecutableRunConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

QVariantMap CustomExecutableRunConfiguration::toMap() const
{
    QVariantMap map(LocalApplicationRunConfiguration::toMap());
    map.insert(QLatin1String(EXECUTABLE_KEY), m_executable);
    map.insert(QLatin1String(ARGUMENTS_KEY), m_cmdArguments);
    map.insert(QLatin1String(WORKING_DIRECTORY_KEY), m_workingDirectory);
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == Console);
    map.insert(QLatin1String(USER_SET_NAME_KEY), m_userSetName);
    map.insert(QLatin1String(USER_NAME_KEY), m_userName);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), ProjectExplorer::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), static_cast<int>(m_baseEnvironmentBase));
    return map;
}

bool CustomExecutableRunConfiguration::fromMap(const QVariantMap &map)
{
    m_executable = map.value(QLatin1String(EXECUTABLE_KEY)).toString();
    m_cmdArguments = map.value(QLatin1String(ARGUMENTS_KEY)).toStringList();
    m_workingDirectory = map.value(QLatin1String(WORKING_DIRECTORY_KEY)).toString();
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY)).toBool() ? Console : Gui;
    m_userSetName = map.value(QLatin1String(USER_SET_NAME_KEY)).toBool();
    m_userName = map.value(QLatin1String(USER_NAME_KEY)).toString();
    m_userEnvironmentChanges = ProjectExplorer::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_baseEnvironmentBase = static_cast<BaseEnvironmentBase>(map.value(QLatin1String(BASE_ENVIRONMENT_BASE_KEY), static_cast<int>(CustomExecutableRunConfiguration::BuildEnvironmentBase)).toInt());

    return RunConfiguration::fromMap(map);
}

void CustomExecutableRunConfiguration::setExecutable(const QString &executable)
{
    m_executable = executable;
    if (!m_userSetName)
        setDisplayName(tr("Run %1").arg(m_executable));
    emit changed();
}

void CustomExecutableRunConfiguration::setCommandLineArguments(const QString &commandLineArguments)
{
    m_cmdArguments = ProjectExplorer::Environment::parseCombinedArgString(commandLineArguments);
    emit changed();
}

void CustomExecutableRunConfiguration::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
    emit changed();
}

void CustomExecutableRunConfiguration::setRunMode(RunMode runMode)
{
    m_runMode = runMode;
    emit changed();
}

QWidget *CustomExecutableRunConfiguration::configurationWidget()
{
    return new CustomExecutableConfigurationWidget(this);
}

void CustomExecutableRunConfiguration::setUserName(const QString &name)
{
    if (name.isEmpty()) {
        m_userName = name;
        m_userSetName = false;
        setDisplayName(tr("Run %1").arg(m_executable));
    } else {
        m_userName = name;
        m_userSetName = true;
        setDisplayName(name);
    }
    emit changed();
}

QString CustomExecutableRunConfiguration::dumperLibrary() const
{
    QString qmakePath = ProjectExplorer::DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = ProjectExplorer::DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    return ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
}

QStringList CustomExecutableRunConfiguration::dumperLibraryLocations() const
{
    QString qmakePath = ProjectExplorer::DebuggingHelperLibrary::findSystemQt(environment());
    QString qtInstallData = ProjectExplorer::DebuggingHelperLibrary::qtInstallDataDir(qmakePath);
    return ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryLocationsByInstallData(qtInstallData);
}

ProjectExplorer::ToolChain::ToolChainType CustomExecutableRunConfiguration::toolChainType() const
{
    return ProjectExplorer::ToolChain::UNKNOWN;
}

// Factory

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

CustomExecutableRunConfigurationFactory::~CustomExecutableRunConfigurationFactory()
{

}

bool CustomExecutableRunConfigurationFactory::canCreate(Target *parent, const QString &id) const
{
    Q_UNUSED(parent);
    return id == QLatin1String(CUSTOM_EXECUTABLE_ID);
}

RunConfiguration *CustomExecutableRunConfigurationFactory::create(Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    RunConfiguration *rc(new CustomExecutableRunConfiguration(parent));
    rc->setDisplayName(tr("Custom Executable"));
    return rc;
}

bool CustomExecutableRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    QString id(idFromMap(map));
    return canCreate(parent, id);
}

RunConfiguration *CustomExecutableRunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CustomExecutableRunConfiguration *rc(new CustomExecutableRunConfiguration(parent));
    if (rc->fromMap(map))
        return rc;
    delete rc;
    return 0;
}

bool CustomExecutableRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

RunConfiguration *CustomExecutableRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new CustomExecutableRunConfiguration(parent, static_cast<CustomExecutableRunConfiguration*>(source));
}

QStringList CustomExecutableRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    Q_UNUSED(parent)
    return QStringList() << QLatin1String(CUSTOM_EXECUTABLE_ID);
}

QString CustomExecutableRunConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(CUSTOM_EXECUTABLE_ID))
        return tr("Custom Executable");
    return QString();
}
