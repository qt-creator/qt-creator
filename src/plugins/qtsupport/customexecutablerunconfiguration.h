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

#ifndef CUSTOMEXECUTABLERUNCONFIGURATION_H
#define CUSTOMEXECUTABLERUNCONFIGURATION_H

#include "qtsupport_global.h"

#include <projectexplorer/runnables.h>

#include <QVariantMap>

namespace ProjectExplorer { class Target; }

namespace QtSupport {
namespace Internal { class CustomExecutableConfigurationWidget; }

class CustomExecutableRunConfigurationFactory;

class QTSUPPORT_EXPORT CustomExecutableRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    // the configuration widget needs to setExecutable setWorkingDirectory and setCommandLineArguments
    friend class Internal::CustomExecutableConfigurationWidget;
    friend class CustomExecutableRunConfigurationFactory;

public:
    explicit CustomExecutableRunConfiguration(ProjectExplorer::Target *parent);
    ~CustomExecutableRunConfiguration() override;

    /**
     * Returns the executable, looks in the environment for it and might even
     * ask the user if none is specified
     */
    QString executable() const;
    QString workingDirectory() const;
    ProjectExplorer::Runnable runnable() const override;

    /** Returns whether this runconfiguration ever was configured with an executable
     */
    bool isConfigured() const override;

    QWidget *createConfigurationWidget() override;

    ProjectExplorer::Abi abi() const override;

    QVariantMap toMap() const override;

    ConfigurationState ensureConfigured(QString *errorMessage) override;

signals:
    void changed();

protected:
    CustomExecutableRunConfiguration(ProjectExplorer::Target *parent,
                                     CustomExecutableRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map) override;
    QString defaultDisplayName() const;

private slots:
    void configurationDialogFinished();
private:
    void ctor();

    void setExecutable(const QString &executable);
    QString rawExecutable() const;
    void setCommandLineArguments(const QString &commandLineArguments);
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setUserName(const QString &name);
    void setRunMode(ProjectExplorer::ApplicationLauncher::Mode runMode);
    bool validateExecutable(QString *executable = 0, QString *errorMessage = 0) const;

    QString m_executable;
    QString m_workingDirectory;
    QWidget *m_dialog;
};

class CustomExecutableRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit CustomExecutableRunConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const override;
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *source) override;

private:
    bool canHandle(ProjectExplorer::Target *parent) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;
};

} // namespace QtSupport

#endif // CUSTOMEXECUTABLERUNCONFIGURATION_H
