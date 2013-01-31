/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef REMOTELINUXRUNCONFIGURATION_H
#define REMOTELINUXRUNCONFIGURATION_H

#include "remotelinux_export.h"

#include <projectexplorer/runconfiguration.h>
#include <utils/environment.h>

namespace Utils { class PortList; }

namespace RemoteLinux {
class RemoteLinuxRunConfigurationWidget;
class RemoteLinuxDeployConfiguration;

namespace Internal {
class RemoteLinuxRunConfigurationPrivate;
class RemoteLinuxRunConfigurationFactory;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    Q_DISABLE_COPY(RemoteLinuxRunConfiguration)
    friend class Internal::RemoteLinuxRunConfigurationFactory;
    friend class RemoteLinuxRunConfigurationWidget;

public:
    enum BaseEnvironmentType {
        CleanBaseEnvironment = 0,
        RemoteBaseEnvironment = 1
    };

    enum DebuggingType { DebugCppOnly, DebugQmlOnly, DebugCppAndQml };

    RemoteLinuxRunConfiguration(ProjectExplorer::Target *parent, const Core::Id id,
        const QString &projectFilePath);
    ~RemoteLinuxRunConfiguration();

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;

    virtual QString environmentPreparationCommand() const;
    virtual QString commandPrefix() const;

    QString localExecutableFilePath() const;
    QString defaultRemoteExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    QString arguments() const;
    void setArguments(const QString &args);
    QString workingDirectory() const;
    void setWorkingDirectory(const QString &wd);
    void setAlternateRemoteExecutable(const QString &exe);
    QString alternateRemoteExecutable() const;
    void setUseAlternateExecutable(bool useAlternate);
    bool useAlternateExecutable() const;

    QVariantMap toMap() const;

    QString baseEnvironmentText() const;
    BaseEnvironmentType baseEnvironmentType() const;
    Utils::Environment environment() const;
    Utils::Environment baseEnvironment() const;
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    Utils::Environment remoteEnvironment() const;

    int portsUsedByDebuggers() const;

    QString projectFilePath() const;

    static const QString IdPrefix;

signals:
    void deploySpecsChanged();
    void targetInformationChanged() const;
    void baseEnvironmentChanged();
    void remoteEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

protected:
    RemoteLinuxRunConfiguration(ProjectExplorer::Target *parent,
        RemoteLinuxRunConfiguration *source);
    bool fromMap(const QVariantMap &map);
    QString defaultDisplayName();
    void setDisabledReason(const QString &reason) const;
    QString userEnvironmentChangesAsString() const;

protected slots:
    void updateEnabledState() { emit enabledChanged(); }

private slots:
    void handleBuildSystemDataUpdated();

private:
    void init();

    void setBaseEnvironmentType(BaseEnvironmentType env);
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    void setRemoteEnvironment(const Utils::Environment &environment);

    Internal::RemoteLinuxRunConfigurationPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXRUNCONFIGURATION_H
