/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CMAKERUNCONFIGURATION_H
#define CMAKERUNCONFIGURATION_H

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/persistentsettings.h>
#include <projectexplorer/environmenteditmodel.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Utils {
class DebuggerLanguageChooser;
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class CMakeTarget;

class CMakeRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    friend class CMakeRunConfigurationWidget;
    friend class CMakeRunConfigurationFactory;

public:
    CMakeRunConfiguration(CMakeTarget *parent, const QString &target,
                          const QString &workingDirectory, const QString &title);
    ~CMakeRunConfiguration();

    CMakeTarget *cmakeTarget() const;
    CMakeBuildConfiguration *activeBuildConfiguration() const;

    QString executable() const;
    RunMode runMode() const;
    QString workingDirectory() const;
    QString commandLineArguments() const;
    Utils::Environment environment() const;
    QWidget *createConfigurationWidget();

    void setExecutable(const QString &executable);
    void setBaseWorkingDirectory(const QString &workingDirectory);

    QString title() const;

    QString dumperLibrary() const;
    QStringList dumperLibraryLocations() const;
    ProjectExplorer::ToolChainType toolChainType() const;

    QVariantMap toMap() const;

    void setEnabled(bool b);

    bool isEnabled(ProjectExplorer::BuildConfiguration *bc) const;
    using LocalApplicationRunConfiguration::isEnabled;

signals:
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);
    void baseWorkingDirectoryChanged(const QString&);

private slots:
    void setCommandLineArguments(const QString &newText);

protected:
    CMakeRunConfiguration(CMakeTarget *parent, CMakeRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName() const;

private:
    void setUserWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void ctor();

    enum BaseEnvironmentBase { CleanEnvironmentBase = 0,
                               SystemEnvironmentBase = 1,
                               BuildEnvironmentBase = 2};
    void setBaseEnvironmentBase(BaseEnvironmentBase env);
    BaseEnvironmentBase baseEnvironmentBase() const;
    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;

    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;

    RunMode m_runMode;
    QString m_buildTarget;
    QString m_workingDirectory;
    QString m_userWorkingDirectory;
    QString m_title;
    QString m_arguments;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    BaseEnvironmentBase m_baseEnvironmentBase;
    bool m_enabled;
};

class CMakeRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent = 0);

private slots:
    void setArguments(const QString &args);
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged();
    void userChangesChanged();
    void setWorkingDirectory();
    void resetWorkingDirectory();
    void useCppDebuggerToggled(bool toggled);
    void useQmlDebuggerToggled(bool toggled);
    void qmlDebugServerPortChanged(uint port);

private slots:
    void baseEnvironmentComboBoxChanged(int index);
    void workingDirectoryChanged(const QString &workingDirectory);

private:
    void ctor();
    bool m_ignoreChange;
    CMakeRunConfiguration *m_cmakeRunConfiguration;
    Utils::PathChooser *m_workingDirectoryEdit;
    QComboBox *m_baseEnvironmentComboBox;
    Utils::DebuggerLanguageChooser *m_debuggerLanguageChooser;
    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    Utils::DetailsWidget *m_detailsContainer;
};

class CMakeRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit CMakeRunConfigurationFactory(QObject *parent = 0);
    ~CMakeRunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product);

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const QString &id) const;
};

}
}

#endif // CMAKERUNCONFIGURATION_H
