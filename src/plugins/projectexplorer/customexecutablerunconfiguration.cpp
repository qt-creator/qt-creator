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
#include "project.h"
#include "persistentsettings.h"
#include "environmenteditmodel.h"

#include <coreplugin/icore.h>
#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/buildconfiguration.h>
#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QtGui/QCheckBox>
#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QFileDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>
#include <QDialogButtonBox>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

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

    m_detailsContainer->setSummaryText(text);
    // We triggered the change, don't update us
    if (m_ignoreChange)
        return;
    m_executableChooser->setPath(executable);
    m_commandLineArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(m_runConfiguration->commandLineArguments()));
    m_workingDirectory->setPath(m_runConfiguration->baseWorkingDirectory());
    m_useTerminalCheck->setChecked(m_runConfiguration->runMode() == LocalApplicationRunConfiguration::Console);
    m_userName->setText(m_runConfiguration->userName());
}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Project *pro)
    : LocalApplicationRunConfiguration(pro),
      m_runMode(Gui),
      m_userSetName(false),
      m_baseEnvironmentBase(CustomExecutableRunConfiguration::BuildEnvironmentBase)
{
    m_workingDirectory = "$BUILDDIR";
    setName(tr("Custom Executable"));

    connect(pro, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(activeBuildConfigurationChanged()));

    m_lastActiveBuildConfiguration = pro->activeBuildConfiguration();

    if (m_lastActiveBuildConfiguration) {
        connect(m_lastActiveBuildConfiguration, SIGNAL(environmentChanged()),
                this, SIGNAL(baseEnvironmentChanged()));
    }
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
    m_lastActiveBuildConfiguration = project()->activeBuildConfiguration();
    if (m_lastActiveBuildConfiguration) {
        connect(m_lastActiveBuildConfiguration, SIGNAL(environmentChanged()),
                this, SIGNAL(baseEnvironmentChanged()));
    }
}

QString CustomExecutableRunConfiguration::type() const
{
    return "ProjectExplorer.CustomExecutableRunConfiguration";
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
        Environment env = project()->activeBuildConfiguration()->environment();
        exec = env.searchInPath(m_executable);
        if (exec.isEmpty())
            exec = QDir::cleanPath(workingDirectory() + "/" + m_executable);
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
            return QString::null;
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
    QString bd = project()->activeBuildConfiguration()->buildDirectory();
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
    return QString::null;
}

ProjectExplorer::Environment CustomExecutableRunConfiguration::baseEnvironment() const
{
    ProjectExplorer::Environment env;
    if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::CleanEnvironmentBase) {
        // Nothing
    } else  if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::SystemEnvironmentBase) {
        env = ProjectExplorer::Environment::systemEnvironment();
    } else  if (m_baseEnvironmentBase == CustomExecutableRunConfiguration::BuildEnvironmentBase) {
        env = project()->activeBuildConfiguration()->environment();
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

void CustomExecutableRunConfiguration::save(PersistentSettingsWriter &writer) const
{
    writer.saveValue("Executable", m_executable);
    writer.saveValue("Arguments", m_cmdArguments);
    writer.saveValue("WorkingDirectory", m_workingDirectory);
    writer.saveValue("UseTerminal", m_runMode == Console);
    writer.saveValue("UserSetName", m_userSetName);
    writer.saveValue("UserName", m_userName);
    writer.saveValue("UserEnvironmentChanges", ProjectExplorer::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    writer.saveValue("BaseEnvironmentBase", m_baseEnvironmentBase);
    LocalApplicationRunConfiguration::save(writer);
}

void CustomExecutableRunConfiguration::restore(const PersistentSettingsReader &reader)
{
    m_executable = reader.restoreValue("Executable").toString();
    m_cmdArguments = reader.restoreValue("Arguments").toStringList();
    m_workingDirectory = reader.restoreValue("WorkingDirectory").toString();
    m_runMode = reader.restoreValue("UseTerminal").toBool() ? Console : Gui;
    m_userSetName = reader.restoreValue("UserSetName").toBool();
    m_userName = reader.restoreValue("UserName").toString();
    m_userEnvironmentChanges = ProjectExplorer::EnvironmentItem::fromStringList(reader.restoreValue("UserEnvironmentChanges").toStringList());
    LocalApplicationRunConfiguration::restore(reader);
    QVariant tmp = reader.restoreValue("BaseEnvironmentBase");
    m_baseEnvironmentBase = tmp.isValid() ? BaseEnvironmentBase(tmp.toInt()) : CustomExecutableRunConfiguration::BuildEnvironmentBase;
}

void CustomExecutableRunConfiguration::setExecutable(const QString &executable)
{
    m_executable = executable;
    if (!m_userSetName)
        setName(tr("Run %1").arg(m_executable));
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
        setName(tr("Run %1").arg(m_executable));
    } else {
        m_userName = name;
        m_userSetName = true;
        setName(name);
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

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory()
{
}

CustomExecutableRunConfigurationFactory::~CustomExecutableRunConfigurationFactory()
{

}

// used to recreate the runConfigurations when restoring settings
bool CustomExecutableRunConfigurationFactory::canRestore(const QString &type) const
{
    return type == "ProjectExplorer.CustomExecutableRunConfiguration";
}

RunConfiguration* CustomExecutableRunConfigurationFactory::create(Project *project, const QString &type)
{
    if (type == "ProjectExplorer.CustomExecutableRunConfiguration") {
        RunConfiguration* rc = new CustomExecutableRunConfiguration(project);
        rc->setName(tr("Custom Executable"));
        return rc;
    } else {
        return 0;
    }
}

QStringList CustomExecutableRunConfigurationFactory::availableCreationTypes(Project *pro) const
{
    Q_UNUSED(pro)
    return QStringList()<< "ProjectExplorer.CustomExecutableRunConfiguration";
}

QString CustomExecutableRunConfigurationFactory::displayNameForType(const QString &type) const
{
    if (type == "ProjectExplorer.CustomExecutableRunConfiguration")
        return tr("Custom Executable");
    else
        return QString();
}
