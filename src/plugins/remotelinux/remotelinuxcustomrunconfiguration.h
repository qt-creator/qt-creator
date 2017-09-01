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

#include <projectexplorer/runconfiguration.h>

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxCustomRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    explicit RemoteLinuxCustomRunConfiguration(ProjectExplorer::Target *target);

    void initialize();
    void copyFrom(const RemoteLinuxCustomRunConfiguration *source);

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool isConfigured() const override;
    ConfigurationState ensureConfigured(QString *errorMessage) override;
    QWidget *createConfigurationWidget() override;
    Utils::OutputFormatter *createOutputFormatter() const override;
    ProjectExplorer::Runnable runnable() const override;
    QString localExecutableFilePath() const { return m_localExecutable; }

    void setLocalExecutableFilePath(const QString &executable) { m_localExecutable = executable; }
    void setRemoteExecutableFilePath(const QString &executable);
    void setArguments(const QString &args) { m_arguments = args; }
    void setWorkingDirectory(const QString &wd) { m_workingDirectory = wd; }

    static Core::Id runConfigId();
    static QString runConfigDefaultDisplayName();

private:
    QString m_localExecutable;
    QString m_remoteExecutable;
    QString m_arguments;
    QString m_workingDirectory;
};

} // namespace Internal
} // namespace RemoteLinux
