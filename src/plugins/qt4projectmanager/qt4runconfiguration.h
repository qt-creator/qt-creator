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
class Qt4ProFileNode;
class Qt4RunConfigurationFactory;
class Qt4Target;

class Qt4RunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    // to change the display name and arguments and set the userenvironmentchanges
    friend class Qt4RunConfigurationWidget;
    friend class Qt4RunConfigurationFactory;

public:
    Qt4RunConfiguration(Qt4Target *parent, const QString &proFilePath);
    virtual ~Qt4RunConfiguration();

    Qt4Target *qt4Target() const;

    virtual bool isEnabled(ProjectExplorer::BuildConfiguration *configuration) const;
    virtual QWidget *configurationWidget();

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
    QVariantMap toMap() const;

public slots:
    // This function is called if:
    // X the pro file changes
    // X the active buildconfiguration changes
    // X  the qmake parameters change
    // X  the build directory changes
    void invalidateCachedTargetInformation();

signals:
    void commandLineArgumentsChanged(const QString&);
    void workingDirectoryChanged(const QString&);
    void runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    void usingDyldImageSuffixChanged(bool);
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<ProjectExplorer::EnvironmentItem> &diff);

    // Note: These signals might not get emitted for every change!
    void effectiveTargetInformationChanged();

private slots:
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);
    void setArguments(const QString &argumentsString);
    void setWorkingDirectory(const QString &workingDirectory);
    void setUserName(const QString&);
    void setRunMode(RunMode runMode);

protected:
    Qt4RunConfiguration(Qt4Target *parent, Qt4RunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private:
    enum BaseEnvironmentBase { CleanEnvironmentBase = 0,
                               SystemEnvironmentBase = 1,
                               BuildEnvironmentBase  = 2 };
    void setDefaultDisplayName();
    void setBaseEnvironmentBase(BaseEnvironmentBase env);
    BaseEnvironmentBase baseEnvironmentBase() const;

    void ctor();

    ProjectExplorer::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    void setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff);
    QList<ProjectExplorer::EnvironmentItem> userEnvironmentChanges() const;

    void updateTarget();
    QStringList m_commandLineArguments;
    QString m_proFilePath; // Full path to the Application Pro File

    // Cached startup sub project information
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
    ~Qt4RunConfigurationWidget();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
private slots:
    void workDirectoryEdited();
    void workingDirectoryReseted();
    void argumentsEdited(const QString &arguments);
    void displayNameEdited(const QString &name);
    void userChangesEdited();

    void workingDirectoryChanged(const QString &workingDirectory);
    void commandLineArgumentsChanged(const QString &args);
    void displayNameChanged();
    void runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    void userEnvironmentChangesChanged(const QList<ProjectExplorer::EnvironmentItem> &userChanges);
    void baseEnvironmentChanged();

    void effectiveTargetInformationChanged();
    void termToggled(bool);
    void usingDyldImageSuffixToggled(bool);
    void usingDyldImageSuffixChanged(bool);
    void baseEnvironmentSelected(int index);

private:
    Qt4RunConfiguration *m_qt4RunConfiguration;
    bool m_ignoreChange;
    QLabel *m_executableLabel;
    Utils::PathChooser *m_workingDirectoryEdit;
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
    explicit Qt4RunConfigurationFactory(QObject *parent = 0);
    virtual ~Qt4RunConfigurationFactory();

    virtual bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    virtual ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    virtual bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    virtual ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    virtual bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    virtual ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const QString &id) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4RUNCONFIGURATION_H
