/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectexplorer_global.h"

#include "projectexplorer/runnables.h"

namespace ProjectExplorer {

class CustomExecutableDialog;

namespace Internal { class CustomExecutableConfigurationWidget; }

class PROJECTEXPLORER_EXPORT CustomExecutableRunConfiguration : public RunConfiguration
{
    Q_OBJECT
    // the configuration widget needs to setExecutable setWorkingDirectory and setCommandLineArguments
    friend class Internal::CustomExecutableConfigurationWidget;
    friend class ProjectExplorer::IRunConfigurationFactory;

public:
    explicit CustomExecutableRunConfiguration(Target *target);
    ~CustomExecutableRunConfiguration() override;

    /**
     * Returns the executable, looks in the environment for it and might even
     * ask the user if none is specified
     */
    QString executable() const;
    QString workingDirectory() const;
    Runnable runnable() const override;

    /** Returns whether this runconfiguration ever was configured with an executable
     */
    bool isConfigured() const override;
    QWidget *createConfigurationWidget() override;
    Abi abi() const override;
    QVariantMap toMap() const override;
    ConfigurationState ensureConfigured(QString *errorMessage) override;

signals:
    void changed();

protected:
    void initialize();
    void copyFrom(const CustomExecutableRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map) override;
    QString defaultDisplayName() const;

private:
    void ctor();

    void configurationDialogFinished();
    void setExecutable(const QString &executable);
    QString rawExecutable() const;
    void setCommandLineArguments(const QString &commandLineArguments);
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setUserName(const QString &name);
    void setRunMode(ApplicationLauncher::Mode runMode);
    bool validateExecutable(QString *executable = 0, QString *errorMessage = 0) const;

    QString m_executable;
    QString m_workingDirectory;
    CustomExecutableDialog *m_dialog = nullptr;
};

class CustomExecutableRunConfigurationFactory : public IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit CustomExecutableRunConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(Target *parent, CreationMode mode) const override;
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(Target *parent, Core::Id id) const override;
    bool canRestore(Target *parent, const QVariantMap &map) const override;
    bool canClone(Target *parent, RunConfiguration *product) const override;
    RunConfiguration *clone(Target *parent, RunConfiguration *source) override;

private:
    bool canHandle(Target *parent) const;

    RunConfiguration *doCreate(Target *parent, Core::Id id) override;
    RunConfiguration *doRestore(Target *parent, const QVariantMap &map) override;
};

} // namespace ProjectExplorer
