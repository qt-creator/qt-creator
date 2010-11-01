/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CUSTOMEXECUTABLERUNCONFIGURATION_H
#define CUSTOMEXECUTABLERUNCONFIGURATION_H

#include "applicationrunconfiguration.h"

#include <utils/environment.h>

#include <QtCore/QVariantMap>

namespace ProjectExplorer {
class Target;

namespace Internal {
class CustomExecutableConfigurationWidget;
}

class CustomExecutableRunConfigurationFactory;

class PROJECTEXPLORER_EXPORT CustomExecutableRunConfiguration : public LocalApplicationRunConfiguration
{
    Q_OBJECT
    // the configuration widget needs to setExecutable setWorkingDirectory and setCommandLineArguments
    friend class Internal::CustomExecutableConfigurationWidget;
    friend class CustomExecutableRunConfigurationFactory;

public:
    explicit CustomExecutableRunConfiguration(Target *parent);
    ~CustomExecutableRunConfiguration();

    /**
     * Returns the executable, looks in the environment for it and might even
     * ask the user if none is specified
     */
    QString executable() const;
    QString rawExecutable() const;

    /** Returns whether this runconfiguration ever was configured with a executable
     */
    bool isConfigured() const;

    LocalApplicationRunConfiguration::RunMode runMode() const;
    QString workingDirectory() const;
    QStringList commandLineArguments() const;
    Utils::Environment environment() const;

    QWidget *createConfigurationWidget();
    QString dumperLibrary() const;
    QStringList dumperLibraryLocations() const;

    ProjectExplorer::ToolChainType toolChainType() const;

    QVariantMap toMap() const;

signals:
    void changed();

    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

private slots:
    void activeBuildConfigurationChanged();

protected:
    CustomExecutableRunConfiguration(Target *parent, CustomExecutableRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    QString defaultDisplayName() const;

private:
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

    void setExecutable(const QString &executable);
    void setBaseCommandLineArguments(const QString &commandLineArguments);
    QStringList baseCommandLineArguments() const;
    QString baseWorkingDirectory() const;
    void setBaseWorkingDirectory(const QString &workingDirectory);
    void setUserName(const QString &name);
    void setRunMode(RunMode runMode);

    QString m_executable;
    QString m_workingDirectory;
    QStringList m_cmdArguments;
    RunMode m_runMode;
    bool m_userSetName;
    QString m_userName;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    BaseEnvironmentBase m_baseEnvironmentBase;
    ProjectExplorer::BuildConfiguration *m_lastActiveBuildConfiguration;
};

class CustomExecutableRunConfigurationFactory : public IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit CustomExecutableRunConfigurationFactory(QObject *parent = 0);
    ~CustomExecutableRunConfigurationFactory();

    QStringList availableCreationIds(Target *parent) const;
    QString displayNameForId(const QString &id) const;

    bool canCreate(Target *parent, const QString &id) const;
    RunConfiguration *create(Target *parent, const QString &id);
    bool canRestore(Target *parent, const QVariantMap &map) const;
    RunConfiguration *restore(Target *parent, const QVariantMap &map);
    bool canClone(Target *parent, RunConfiguration *product) const;
    RunConfiguration *clone(Target *parent, RunConfiguration *source);
};

} // namespace ProjectExplorer

#endif // CUSTOMEXECUTABLERUNCONFIGURATION_H
