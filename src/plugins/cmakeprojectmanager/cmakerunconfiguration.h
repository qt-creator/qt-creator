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

#ifndef CMAKERUNCONFIGURATION_H
#define CMAKERUNCONFIGURATION_H

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/persistentsettings.h>
#include <projectexplorer/environmenteditmodel.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace CMakeProjectManager {
namespace Internal {

class CMakeProject;

class CMakeRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    friend class CMakeRunConfigurationWidget;
    Q_OBJECT
public:
    CMakeRunConfiguration(CMakeProject *pro, const QString &target, const QString &workingDirectory, const QString &title);
    virtual ~CMakeRunConfiguration();
    virtual QString type() const;
    virtual QString executable() const;
    virtual RunMode runMode() const;
    virtual QString workingDirectory() const;
    virtual QStringList commandLineArguments() const;
    virtual ProjectExplorer::Environment environment() const;
    virtual QWidget *configurationWidget();

    void setExecutable(const QString &executable);
    void setWorkingDirectory(const QString &workingDirectory);

    void setUserWorkingDirectory(const QString &workingDirectory);

    QString title() const;

    virtual void save(ProjectExplorer::PersistentSettingsWriter &writer) const;
    virtual void restore(const ProjectExplorer::PersistentSettingsReader &reader);
    virtual QString dumperLibrary() const;
    virtual QStringList dumperLibraryLocations() const;
    virtual ProjectExplorer::ToolChain::ToolChainType toolChainType() const;

signals:
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<ProjectExplorer::EnvironmentItem> &diff);
    void workingDirectoryChanged(const QString&);

private slots:
    void setArguments(const QString &newText);
private:
    enum BaseEnvironmentBase { CleanEnvironmentBase = 0,
                               SystemEnvironmentBase = 1,
                               BuildEnvironmentBase = 2};
    void setBaseEnvironmentBase(BaseEnvironmentBase env);
    BaseEnvironmentBase baseEnvironmentBase() const;
    ProjectExplorer::Environment baseEnvironment() const;
    void setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff);
    QList<ProjectExplorer::EnvironmentItem> userEnvironmentChanges() const;

    RunMode m_runMode;
    QString m_target;
    QString m_workingDirectory;
    QString m_userWorkingDirectory;
    QString m_title;
    QString m_arguments;
    QList<ProjectExplorer::EnvironmentItem> m_userEnvironmentChanges;
    BaseEnvironmentBase m_baseEnvironmentBase;
};

class CMakeRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent = 0);
private slots:
    void setArguments(const QString &args);
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged();
    void userChangesUpdated();
    void setWorkingDirectory();
    void resetWorkingDirectory();
private slots:
    void baseEnvironmentComboBoxChanged(int index);
    void workingDirectoryChanged(const QString &workingDirectory);
private:
    void updateSummary();
    bool m_ignoreChange;
    CMakeRunConfiguration *m_cmakeRunConfiguration;
    Utils::PathChooser *m_workingDirectoryEdit;
    QComboBox *m_baseEnvironmentComboBox;
    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    Utils::DetailsWidget *m_detailsContainer;
};

class CMakeRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    CMakeRunConfigurationFactory();
    virtual ~CMakeRunConfigurationFactory();
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(const QString &type) const;
    // used to show the list of possible additons to a project, returns a list of types
    virtual QStringList availableCreationTypes(ProjectExplorer::Project *pro) const;
    // used to translate the types to names to display to the user
    virtual QString displayNameForType(const QString &type) const;
    virtual ProjectExplorer::RunConfiguration* create(ProjectExplorer::Project *project, const QString &type);
};


}
}

#endif // CMAKERUNCONFIGURATION_H
