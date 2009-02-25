/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "customexecutablerunconfiguration.h"
#include "environment.h"
#include "project.h"

#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QFileDialog>
#include <QDialogButtonBox>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

CustomExecutableConfigurationWidget::CustomExecutableConfigurationWidget(CustomExecutableRunConfiguration *rc)
    : m_ignoreChange(false)
{
    m_runConfiguration = rc;
    
    QFormLayout *layout = new QFormLayout();
    layout->setMargin(0);

    m_executableChooser = new Core::Utils::PathChooser();
    m_executableChooser->setExpectedKind(Core::Utils::PathChooser::File);
    layout->addRow("Executable:", m_executableChooser);

    m_commandLineArgumentsLineEdit = new QLineEdit;
    m_commandLineArgumentsLineEdit->setMinimumWidth(200); // this shouldn't be fixed here...
    layout->addRow("Arguments:", m_commandLineArgumentsLineEdit);

    m_workingDirectory = new Core::Utils::PathChooser();
    layout->addRow("Working Directory:", m_workingDirectory);

    setLayout(layout);
    changed();
    
    connect(m_executableChooser, SIGNAL(changed()),
            this, SLOT(setExecutable()));
    connect(m_commandLineArgumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(setCommandLineArguments(const QString&)));
    connect(m_workingDirectory, SIGNAL(changed()),
            this, SLOT(setWorkingDirectory()));
    
    connect(m_runConfiguration, SIGNAL(changed()), this, SLOT(changed()));
}

void CustomExecutableConfigurationWidget::setExecutable()
{
    m_ignoreChange = true;
    m_runConfiguration->setExecutable(m_executableChooser->path());
    m_ignoreChange = false;
}
void CustomExecutableConfigurationWidget::setCommandLineArguments(const QString &commandLineArguments)
{
    m_ignoreChange = true;
    m_runConfiguration->setCommandLineArguments(commandLineArguments);
    m_ignoreChange = false;
}
void CustomExecutableConfigurationWidget::setWorkingDirectory()
{
    m_ignoreChange = true;
    m_runConfiguration->setWorkingDirectory(m_workingDirectory->path());
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::changed()
{
    // We triggered the change, don't update us
    if (m_ignoreChange)
        return;
    m_executableChooser->setPath(m_runConfiguration->baseExecutable());
    m_commandLineArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(m_runConfiguration->commandLineArguments()));
    m_workingDirectory->setPath(m_runConfiguration->baseWorkingDirectory());
}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Project *pro)
    : ApplicationRunConfiguration(pro)
{
    m_workingDirectory = "$BUILDDIR";
    setName("Custom Executable");
}

CustomExecutableRunConfiguration::~CustomExecutableRunConfiguration()
{
}

QString CustomExecutableRunConfiguration::type() const
{
    return "ProjectExplorer.CustomExecutableRunConfiguration";
}

QString CustomExecutableRunConfiguration::baseExecutable() const
{
    return m_executable;
}

QString CustomExecutableRunConfiguration::executable() const
{
    QString exec;
    if (QDir::isRelativePath(m_executable)) {
        Environment env = project()->environment(project()->activeBuildConfiguration());
        exec = env.searchInPath(m_executable);
    } else {
        exec = m_executable;
    }

    if (!QFileInfo(exec).exists()) {
        // Oh the executable doesn't exists, ask the user.
        QWidget *confWidget = const_cast<CustomExecutableRunConfiguration *>(this)->configurationWidget();
        QDialog dialog;
        dialog.setLayout(new QVBoxLayout());
        dialog.layout()->addWidget(new QLabel("Could not find the executable, please specify one."));
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

ApplicationRunConfiguration::RunMode CustomExecutableRunConfiguration::runMode() const
{
    return ApplicationRunConfiguration::Gui;
}

QString CustomExecutableRunConfiguration::baseWorkingDirectory() const
{
    return m_workingDirectory;
}

QString CustomExecutableRunConfiguration::workingDirectory() const
{
    QString wd = m_workingDirectory;
    QString bd = project()->buildDirectory(project()->activeBuildConfiguration());
    return wd.replace("$BUILDDIR", QDir::cleanPath(bd));
}

QStringList CustomExecutableRunConfiguration::commandLineArguments() const
{
    return m_cmdArguments;
}

Environment CustomExecutableRunConfiguration::environment() const
{
    return project()->environment(project()->activeBuildConfiguration());
}


void CustomExecutableRunConfiguration::save(PersistentSettingsWriter &writer) const
{
    writer.saveValue("Executable", m_executable);
    writer.saveValue("Arguments", m_cmdArguments);
    writer.saveValue("WorkingDirectory", m_workingDirectory);
    ApplicationRunConfiguration::save(writer);
}

void CustomExecutableRunConfiguration::restore(const PersistentSettingsReader &reader)
{
    m_executable = reader.restoreValue("Executable").toString();
    m_cmdArguments = reader.restoreValue("Arguments").toStringList();
    m_workingDirectory = reader.restoreValue("WorkingDirectory").toString();
    ApplicationRunConfiguration::restore(reader);
}

void CustomExecutableRunConfiguration::setExecutable(const QString &executable)
{
    m_executable = executable;
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

QWidget *CustomExecutableRunConfiguration::configurationWidget()
{
    return new CustomExecutableConfigurationWidget(this);
}

// Factory

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory()
{
}

CustomExecutableRunConfigurationFactory::~CustomExecutableRunConfigurationFactory()
{

}

// used to recreate the runConfigurations when restoring settings
bool CustomExecutableRunConfigurationFactory::canCreate(const QString &type) const
{
    return type == "ProjectExplorer.CustomExecutableRunConfiguration";
}

QSharedPointer<RunConfiguration> CustomExecutableRunConfigurationFactory::create(Project *project, const QString &type)
{
    if (type == "ProjectExplorer.CustomExecutableRunConfiguration") {
        QSharedPointer<RunConfiguration> rc(new CustomExecutableRunConfiguration(project));
        rc->setName("Custom Executable");
        return rc;
    } else {
        return QSharedPointer<RunConfiguration>(0);
    }
}

QStringList CustomExecutableRunConfigurationFactory::canCreate(Project *pro) const
{
    Q_UNUSED(pro)
    return QStringList()<< "ProjectExplorer.CustomExecutableRunConfiguration";
}

QString CustomExecutableRunConfigurationFactory::nameForType(const QString &type) const
{
    if (type == "ProjectExplorer.CustomExecutableRunConfiguration")
        return "Custom Executable";
    else
        return QString::null;
}
