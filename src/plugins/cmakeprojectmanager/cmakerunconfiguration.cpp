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
#include <projectexplorer/debugginghelper.h>
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

    connect(pro, SIGNAL(activeBuildConfigurationChanged()),
            this, SIGNAL(baseEnvironmentChanged()));

    connect(pro, SIGNAL(environmentChanged(QString)),
            this, SIGNAL(baseEnvironmentChanged()));
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
    writer.saveValue("CMakeRunConfiguration.UserEnvironmentChanges", ProjectExplorer::EnvironmentItem::toStringList(m_userEnvironmentChanges));
}

void CMakeRunConfiguration::restore(const ProjectExplorer::PersistentSettingsReader &reader)
{
    ProjectExplorer::ApplicationRunConfiguration::restore(reader);
    m_target = reader.restoreValue("CMakeRunConfiguration.Target").toString();
    m_workingDirectory = reader.restoreValue("CMakeRunConfiguration.WorkingDirectory").toString();
    m_runMode = reader.restoreValue("CMakeRunConfiguration.UseTerminal").toBool() ? Console : Gui;
    m_title = reader.restoreValue("CMakeRunConfiguation.Title").toString();
    m_arguments = reader.restoreValue("CMakeRunConfiguration.Arguments").toString();
    m_userEnvironmentChanges = ProjectExplorer::EnvironmentItem::fromStringList(reader.restoreValue("CMakeRunConfiguration.UserEnvironmentChanges").toStringList());
}

QWidget *CMakeRunConfiguration::configurationWidget()
{
    return new CMakeRunConfigurationWidget(this);
}

void CMakeRunConfiguration::setArguments(const QString &newText)
{
    m_arguments = newText;
}

QString CMakeRunConfiguration::dumperLibrary() const
{
    QString qmakePath = ProjectExplorer::DebuggingHelperLibrary::findSystemQt(environment());
    QString dhl = ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibrary(qmakePath);
    return dhl;
}

ProjectExplorer::Environment CMakeRunConfiguration::baseEnvironment() const
{
    // TODO use either System Environment
    // build environment
    // or empty
    //Environment env = Environment(QProcess::systemEnvironment());

    QString config = project()->activeBuildConfiguration();
    ProjectExplorer::Environment env = project()->environment(project()->activeBuildConfiguration());
    return env;
}

ProjectExplorer::Environment CMakeRunConfiguration::environment() const
{
    ProjectExplorer::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

QList<ProjectExplorer::EnvironmentItem> CMakeRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void CMakeRunConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        emit userEnvironmentChangesChanged(diff);
    }
}

// Configuration widget


CMakeRunConfigurationWidget::CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent)
    : QWidget(parent), m_cmakeRunConfiguration(cmakeRunConfiguration)
{

    QFormLayout *fl = new QFormLayout();
    QLineEdit *argumentsLineEdit = new QLineEdit();
    argumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(cmakeRunConfiguration->commandLineArguments()));
    connect(argumentsLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(setArguments(QString)));
    fl->addRow(tr("Arguments:"), argumentsLineEdit);

    QVBoxLayout *vbx = new QVBoxLayout(this);
    vbx->addLayout(fl);
    m_environmentWidget = new ProjectExplorer::EnvironmentWidget(this);
    vbx->addWidget(m_environmentWidget);
    m_environmentWidget->setBaseEnvironment(m_cmakeRunConfiguration->baseEnvironment());
    m_environmentWidget->setUserChanges(m_cmakeRunConfiguration->userEnvironmentChanges());

    connect(m_environmentWidget, SIGNAL(userChangesUpdated()),
            this, SLOT(userChangesUpdated()));

    connect(m_cmakeRunConfiguration, SIGNAL(baseEnvironmentChanged()),
            this, SLOT(baseEnvironmentChanged()));
    connect(m_cmakeRunConfiguration, SIGNAL(userEnvironmentChangesChanged(QList<ProjectExplorer::EnvironmentItem>)),
            this, SLOT(userEnvironmentChangesChanged()));
}

void CMakeRunConfigurationWidget::userChangesUpdated()
{
    m_cmakeRunConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
}

void CMakeRunConfigurationWidget::baseEnvironmentChanged()
{
    m_environmentWidget->setBaseEnvironment(m_cmakeRunConfiguration->baseEnvironment());
}

void CMakeRunConfigurationWidget::userEnvironmentChangesChanged()
{
    m_environmentWidget->setUserChanges(m_cmakeRunConfiguration->userEnvironmentChanges());
}

void CMakeRunConfigurationWidget::setArguments(const QString &args)
{
    m_cmakeRunConfiguration->setArguments(args);
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
    QStringList allTargets = pro->targets();
    for (int i=0; i<allTargets.size(); ++i) {
        allTargets[i] = Constants::CMAKERUNCONFIGURATION + allTargets[i];
    }
    return allTargets;
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
        const QString title = type.mid(QString(Constants::CMAKERUNCONFIGURATION).length());
        const CMakeTarget &ct = pro->targetForTitle(title);
        QSharedPointer<ProjectExplorer::RunConfiguration> rc(new CMakeRunConfiguration(pro, ct.executable, ct.workingDirectory, ct.title));
        return rc;
    }
}
