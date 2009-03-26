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

#include "cmakerunconfiguration.h"

#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <projectexplorer/environment.h>
#include <utils/qtcassert.h>
#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeRunConfiguration::CMakeRunConfiguration(CMakeProject *pro, const QString &target, const QString &workingDirectory, const QString &title)
    : ProjectExplorer::ApplicationRunConfiguration(pro)
    , m_runMode(Gui)
    , m_target(target)
    , m_workingDirectory(workingDirectory)
    , m_title(title)
{
    setName(title);
}

CMakeRunConfiguration::~CMakeRunConfiguration()
{
}

QString CMakeRunConfiguration::type() const
{
    return Constants::CMAKERUNCONFIGURATION;
}

QString CMakeRunConfiguration::executable() const
{
    return m_target;
}

ProjectExplorer::ApplicationRunConfiguration::RunMode CMakeRunConfiguration::runMode() const
{
    return m_runMode;
}

QString CMakeRunConfiguration::workingDirectory() const
{
    return m_workingDirectory;
}

QStringList CMakeRunConfiguration::commandLineArguments() const
{
    return ProjectExplorer::Environment::parseCombinedArgString(m_arguments);
}

ProjectExplorer::Environment CMakeRunConfiguration::environment() const
{
    // TODO
    return ProjectExplorer::Environment::systemEnvironment();
}

QString CMakeRunConfiguration::title() const
{
    return m_title;
}

void CMakeRunConfiguration::setExecutable(const QString &executable)
{
    m_target = executable;
}

void CMakeRunConfiguration::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

void CMakeRunConfiguration::save(ProjectExplorer::PersistentSettingsWriter &writer) const
{
    ProjectExplorer::ApplicationRunConfiguration::save(writer);
    writer.saveValue("CMakeRunConfiguration.Target", m_target);
    writer.saveValue("CMakeRunConfiguration.WorkingDirectory", m_workingDirectory);
    writer.saveValue("CMakeRunConfiguration.UseTerminal", m_runMode == Console);
    writer.saveValue("CMakeRunConfiguation.Title", m_title);
    writer.saveValue("CMakeRunConfiguration.Arguments", m_arguments);
}

void CMakeRunConfiguration::restore(const ProjectExplorer::PersistentSettingsReader &reader)
{
    ProjectExplorer::ApplicationRunConfiguration::restore(reader);
    m_target = reader.restoreValue("CMakeRunConfiguration.Target").toString();
    m_workingDirectory = reader.restoreValue("CMakeRunConfiguration.WorkingDirectory").toString();
    m_runMode = reader.restoreValue("CMakeRunConfiguration.UseTerminal").toBool() ? Console : Gui;
    m_title = reader.restoreValue("CMakeRunConfiguation.Title").toString();
    m_arguments = reader.restoreValue("CMakeRunConfiguration.Arguments").toString();
}

QWidget *CMakeRunConfiguration::configurationWidget()
{
    QWidget *widget = new QWidget();
    QFormLayout *fl = new QFormLayout();
    widget->setLayout(fl);
    QLineEdit *argumentsLineEdit = new QLineEdit(widget);
    argumentsLineEdit->setText(m_arguments);
    connect(argumentsLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(setArguments(QString)));
    fl->addRow(tr("Arguments:"), argumentsLineEdit);
    return widget;
}

void CMakeRunConfiguration::setArguments(const QString &newText)
{
    m_arguments = newText;
}

QString CMakeRunConfiguration::dumperLibrary() const
{
    return CMakeManager::findDumperLibrary(environment());
}

// Factory
CMakeRunConfigurationFactory::CMakeRunConfigurationFactory()
{

}

CMakeRunConfigurationFactory::~CMakeRunConfigurationFactory()
{

}

// used to recreate the runConfigurations when restoring settings
bool CMakeRunConfigurationFactory::canCreate(const QString &type) const
{
    if (type.startsWith(Constants::CMAKERUNCONFIGURATION))
        return true;
    return false;
}

// used to show the list of possible additons to a project, returns a list of types
QStringList CMakeRunConfigurationFactory::canCreate(ProjectExplorer::Project *project) const
{
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    if (!pro)
        return QStringList();
    // TODO gather all targets and return them here
    return QStringList();
}

// used to translate the types to names to display to the user
QString CMakeRunConfigurationFactory::nameForType(const QString &type) const
{
    Q_ASSERT(type.startsWith(Constants::CMAKERUNCONFIGURATION));

    if (type == Constants::CMAKERUNCONFIGURATION)
        return "CMake"; // Doesn't happen
    else
        return type.mid(QString(Constants::CMAKERUNCONFIGURATION).length());
}

QSharedPointer<ProjectExplorer::RunConfiguration> CMakeRunConfigurationFactory::create(ProjectExplorer::Project *project, const QString &type)
{
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    Q_ASSERT(pro);
    if (type == Constants::CMAKERUNCONFIGURATION) {
        // Restoring, filename will be added by restoreSettings
        QSharedPointer<ProjectExplorer::RunConfiguration> rc(new CMakeRunConfiguration(pro, QString::null, QString::null, QString::null));
        return rc;
    } else {
        // Adding new
        // TODO extract target from type
        QString file = type.mid(QString(Constants::CMAKERUNCONFIGURATION).length());
        QSharedPointer<ProjectExplorer::RunConfiguration> rc(new CMakeRunConfiguration(pro, file, QString::null, QString::null));
        return rc;
    }
}
