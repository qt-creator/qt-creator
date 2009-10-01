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

#ifndef QT4RUNCONFIGURATION_H
#define QT4RUNCONFIGURATION_H

#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/environmenteditmodel.h>
#include <QtCore/QStringList>
#include <QtGui/QWidget>
#include <QtGui/QToolButton>

QT_BEGIN_NAMESPACE
class QWidget;
class QCheckBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QComboBox;
QT_END_NAMESPACE

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {

class Qt4PriFileNode;

class Qt4RunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    // to change the name and arguments and set the userenvironmentchanges
    friend class Qt4RunConfigurationWidget;
public:
    Qt4RunConfiguration(Qt4Project *pro, const QString &proFilePath);
    virtual ~Qt4RunConfiguration();

    virtual QString type() const;
    virtual bool isEnabled() const;
    virtual QWidget *configurationWidget();
    virtual void save(ProjectExplorer::PersistentSettingsWriter &writer) const;
    virtual void restore(const ProjectExplorer::PersistentSettingsReader &reader);

    virtual QString executable() const;
    virtual RunMode runMode() const;
    virtual QString workingDirectory() const;
    virtual QStringList commandLineArguments() const;
    virtual ProjectExplorer::Environment environment() const;
    virtual QString dumperLibrary() const;
    virtual QStringList dumperLibraryLocations() const;
    virtual ProjectExplorer::ToolChain::ToolChainType toolChainType() const;

    bool isUsingDyldImageSuffix() const;
    void setUsingDyldImageSuffix(bool state);
    QString proFilePath() const;

    // TODO detectQtShadowBuild() ? how did this work ?

public slots:
    // This function is called if:
    // X the pro file changes
    // X the active buildconfiguration changes
    // X  the qmake parameters change
    // X  the build directory changes
    void invalidateCachedTargetInformation();

signals:
    void nameChanged(const QString&);
    void commandLineArgumentsChanged(const QString&);
    void workingDirectoryChanged(const QString&);
    void runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    void usingDyldImageSuffixChanged(bool);
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<ProjectExplorer::EnvironmentItem> &diff);

    // note those signals might not emited for every change
    void effectiveTargetInformationChanged();

private slots:
    void setCommandLineArguments(const QString &argumentsString);
    void setWorkingDirectory(const QString &workingDirectory);
    void nameEdited(const QString&);
    void setRunMode(RunMode runMode);

private:
    enum BaseEnvironmentBase { CleanEnvironmentBase = 0,
                               SystemEnvironmentBase = 1,
                               BuildEnvironmentBase  = 2 };
    void setBaseEnvironmentBase(BaseEnvironmentBase env);
    BaseEnvironmentBase baseEnvironmentBase() const;

    ProjectExplorer::Environment baseEnvironment() const;
    void setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff);
    QList<ProjectExplorer::EnvironmentItem> userEnvironmentChanges() const;

    void updateTarget();
    QStringList m_commandLineArguments;
    QString m_proFilePath; // Full path to the Application Pro File

    // Cached startup sub project information
    QStringList m_targets;
    QString m_executable;
    QString m_workingDir;
    ProjectExplorer::LocalApplicationRunConfiguration::RunMode m_runMode;
    bool m_userSetName;
    bool m_cachedTargetInformationValid;
    bool m_isUsingDyldImageSuffix;
    bool m_userSetWokingDirectory;
    QString m_userWorkingDirectory;
    QList<ProjectExplorer::EnvironmentItem> m_userEnvironmentChanges;
    BaseEnvironmentBase m_baseEnvironmentBase;
};

class Qt4RunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    Qt4RunConfigurationWidget(Qt4RunConfiguration *qt4runconfigration, QWidget *parent);
protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
private slots:
    void setWorkingDirectory();
    void resetWorkingDirectory();
    void setCommandLineArguments(const QString &arguments);
    void nameEdited(const QString &name);
    void userChangesUpdated();

    void workingDirectoryChanged(const QString &workingDirectory);
    void commandLineArgumentsChanged(const QString &args);
    void nameChanged(const QString &name);
    void runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    void userEnvironmentChangesChanged(const QList<ProjectExplorer::EnvironmentItem> &userChanges);
    void baseEnvironmentChanged();

    void effectiveTargetInformationChanged();
    void termToggled(bool);
    void usingDyldImageSuffixToggled(bool);
    void usingDyldImageSuffixChanged(bool);
    void baseEnvironmentComboBoxChanged(int index);

    void toggleDetails();
private:
    void updateSummary();
    Qt4RunConfiguration *m_qt4RunConfiguration;
    bool m_ignoreChange;
    QLabel *m_executableLabel;
    Core::Utils::PathChooser *m_workingDirectoryEdit;
    QLineEdit *m_nameLineEdit;
    QLineEdit *m_argumentsLineEdit;
    QCheckBox *m_useTerminalCheck;
    QCheckBox *m_usingDyldImageSuffix;

    QComboBox *m_baseEnvironmentComboBox;
    Utils::DetailsWidget *m_detailsContainer;

    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    bool m_isShown;
};

class Qt4RunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    Qt4RunConfigurationFactory();
    virtual ~Qt4RunConfigurationFactory();
    virtual bool canRestore(const QString &type) const;
    virtual QSharedPointer<ProjectExplorer::RunConfiguration> create(ProjectExplorer::Project *project, const QString &type);
    QStringList availableCreationTypes(ProjectExplorer::Project *pro) const;
    QString displayNameForType(const QString &type) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4RUNCONFIGURATION_H
