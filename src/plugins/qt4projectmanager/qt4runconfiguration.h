/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QT4RUNCONFIGURATION_H
#define QT4RUNCONFIGURATION_H

#include <projectexplorer/applicationrunconfiguration.h>
#include <QtCore/QStringList>
#include <QtGui/QWidget>

class QWidget;
class QLabel;
class QLineEdit;

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {

class Qt4ProFileNode;



class Qt4RunConfiguration : public ProjectExplorer::ApplicationRunConfiguration
{
    Q_OBJECT
    // to change the name and arguments
    friend class Qt4RunConfigurationWidget;
public:
    Qt4RunConfiguration(Qt4Project *pro, QString proFilePath);
    virtual ~Qt4RunConfiguration();

    virtual QString type() const;
    virtual QWidget *configurationWidget();
    virtual void save(ProjectExplorer::PersistentSettingsWriter &writer) const;
    virtual void restore(const ProjectExplorer::PersistentSettingsReader &reader);

    virtual QString executable() const;
    virtual RunMode runMode() const;
    virtual QString workingDirectory() const;
    virtual QStringList commandLineArguments() const;
    virtual ProjectExplorer::Environment environment() const;

    QString proFilePath() const;

    // Should just be called from qt4project, since that knows that the file changed on disc
    void updateCachedValues();

signals:
    void nameChanged(const QString&);
    void commandLineArgumentsChanged(const QString&);

    // note those signals might not emited for every change
    void effectiveExecutableChanged();
    void effectiveWorkingDirectoryChanged();

private slots:
    void setCommandLineArguments(const QString &argumentsString);
    void nameEdited(const QString&);

private:
    void detectQtShadowBuild(const QString &buildConfig) const;
    QString resolveVariables(const QString &buildConfiguration, const QString& in) const;
    QString qmakeBuildConfigFromBuildConfiguration(const QString &buildConfigurationName) const;

    QStringList m_commandLineArguments;
    Qt4ProFileNode *m_proFileNode;
    QString m_proFilePath; // Full path to the Application Pro File

    // Cached startup sub project information
    QStringList m_targets;
    QString m_executable;
    QString m_srcDir;
    QString m_workingDir;
    ProjectExplorer::ApplicationRunConfiguration::RunMode m_runMode;
    bool m_userSetName;
    QWidget *m_configWidget;
    QLabel *m_executableLabel;
    QLabel *m_workingDirectoryLabel;
};

class Qt4RunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    Qt4RunConfigurationWidget(Qt4RunConfiguration *qt4runconfigration, QWidget *parent);
private slots:
    void setCommandLineArguments(const QString &arguments);
    void nameEdited(const QString &name);
    // TODO connect to signals from qt4runconfiguration for changed arguments and names
    void commandLineArgumentsChanged(const QString &args);
    void nameChanged(const QString &name);
    void effectiveExecutableChanged();
    void effectiveWorkingDirectoryChanged();
private:
    Qt4RunConfiguration *m_qt4RunConfiguration;
    bool m_ignoreChange;
    QLabel *m_executableLabel;
    QLabel *m_workingDirectoryLabel;
    QLineEdit *m_nameLineEdit;
    QLineEdit *m_argumentsLineEdit;
};

class Qt4RunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    Qt4RunConfigurationFactory();
    virtual ~Qt4RunConfigurationFactory();
    virtual bool canCreate(const QString &type) const;
    virtual QSharedPointer<ProjectExplorer::RunConfiguration> create(ProjectExplorer::Project *project, const QString &type);
    QStringList canCreate(ProjectExplorer::Project *pro) const;
    QString nameForType(const QString &type) const;
};

class Qt4RunConfigurationFactoryUser : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    Qt4RunConfigurationFactoryUser();
    virtual ~Qt4RunConfigurationFactoryUser();
    virtual bool canCreate(const QString &type) const;
    virtual QSharedPointer<ProjectExplorer::RunConfiguration> create(ProjectExplorer::Project *project, const QString &type);
    QStringList canCreate(ProjectExplorer::Project *pro) const;
    QString nameForType(const QString &type) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4RUNCONFIGURATION_H
