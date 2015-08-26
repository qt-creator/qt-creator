/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

namespace ProjectExplorer {
class WorkingDirectoryAspect;
class ArgumentsAspect;
class TerminalAspect;
class LocalEnvironmentAspect;
}

namespace Nim {

class NimBuildConfiguration;

class NimRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    NimRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    QWidget *createConfigurationWidget() override;
    ProjectExplorer::Runnable runnable() const override;
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &map) override;

signals:
    void executableChanged(const QString &args);
    void runModeChanged(ProjectExplorer::ApplicationLauncher::Mode);
    void workingDirectoryChanged(const QString &workingDirectory);
    void commandLineArgumentsChanged(const QString &args);
    void runInTerminalChanged(bool);

private:
    void setExecutable(const QString &path);
    void setWorkingDirectory(const QString &path);
    void setCommandLineArguments(const QString &args);
    void updateConfiguration();
    void setActiveBuildConfiguration(NimBuildConfiguration *activeBuildConfiguration);

    QString m_executable;
    NimBuildConfiguration *m_buildConfiguration;
    ProjectExplorer::WorkingDirectoryAspect* m_workingDirectoryAspect;
    ProjectExplorer::ArgumentsAspect* m_argumentAspect;
    ProjectExplorer::TerminalAspect* m_terminalAspect;
    ProjectExplorer::LocalEnvironmentAspect* m_localEnvironmentAspect;
};

}
