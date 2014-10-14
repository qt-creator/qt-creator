/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef REMOTELINUXRUNCONFIGURATION_H
#define REMOTELINUXRUNCONFIGURATION_H

#include "remotelinux_export.h"
#include "abstractremotelinuxrunconfiguration.h"

#include <projectexplorer/runconfiguration.h>

#include <QStringList>

namespace Utils {
class Environment;
class PortList;
}

namespace RemoteLinux {
class RemoteLinuxRunConfigurationWidget;
class RemoteLinuxDeployConfiguration;

namespace Internal {
class RemoteLinuxRunConfigurationPrivate;
class RemoteLinuxRunConfigurationFactory;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxRunConfiguration : public AbstractRemoteLinuxRunConfiguration
{
    Q_OBJECT
    Q_DISABLE_COPY(RemoteLinuxRunConfiguration)
    friend class Internal::RemoteLinuxRunConfigurationFactory;
    friend class RemoteLinuxRunConfigurationWidget;

public:
    RemoteLinuxRunConfiguration(ProjectExplorer::Target *parent, Core::Id id,
        const QString &targetName);
    ~RemoteLinuxRunConfiguration();

    bool isEnabled() const;
    QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;

    virtual Utils::Environment environment() const;

    QString localExecutableFilePath() const;
    QString defaultRemoteExecutableFilePath() const;
    QString remoteExecutableFilePath() const;
    QStringList arguments() const;
    void setArguments(const QString &args);
    QString workingDirectory() const;
    void setWorkingDirectory(const QString &wd);
    void setAlternateRemoteExecutable(const QString &exe);
    QString alternateRemoteExecutable() const;
    void setUseAlternateExecutable(bool useAlternate);
    bool useAlternateExecutable() const;

    QVariantMap toMap() const;

    static const char *IdPrefix;

signals:
    void deploySpecsChanged();
    void targetInformationChanged() const;

protected:
    RemoteLinuxRunConfiguration(ProjectExplorer::Target *parent,
        RemoteLinuxRunConfiguration *source);
    bool fromMap(const QVariantMap &map);
    QString defaultDisplayName();

protected slots:
    void updateEnabledState() { emit enabledChanged(); }

private slots:
    void handleBuildSystemDataUpdated();

private:
    void init();

    Internal::RemoteLinuxRunConfigurationPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXRUNCONFIGURATION_H
