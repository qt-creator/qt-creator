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

    /** Returns whether this runconfiguration ever was configured with a executable
     */
    bool isConfigured() const;

    LocalApplicationRunConfiguration::RunMode runMode() const;
    QString workingDirectory() const;
    QString commandLineArguments() const;
    Utils::Environment environment() const;

    QWidget *createConfigurationWidget();
    QString dumperLibrary() const;
    QStringList dumperLibraryLocations() const;

    ProjectExplorer::Abi abi() const;

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
    QString rawExecutable() const;
    void setCommandLineArguments(const QString &commandLineArguments);
    QString rawCommandLineArguments() const;
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setUserName(const QString &name);
    void setRunMode(RunMode runMode);

    QString m_executable;
    QString m_workingDirectory;
    QString m_cmdArguments;
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
