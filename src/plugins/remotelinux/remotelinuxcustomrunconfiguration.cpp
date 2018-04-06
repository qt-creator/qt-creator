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

#include "remotelinuxcustomrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxenvironmentaspect.h"

#include <projectexplorer/runnables.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtoutputformatter.h>
#include <utils/pathchooser.h>

#include <QFormLayout>
#include <QLabel>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxCustomRunConfigWidget : public QWidget
{
public:
    explicit RemoteLinuxCustomRunConfigWidget(RemoteLinuxCustomRunConfiguration *runConfig)
    {
        auto fl = new QFormLayout(this);

        auto remoteExeLabel = new QLabel(RemoteLinuxCustomRunConfiguration::tr("Remote executable:"));
        auto remoteExeLineEdit = new QLineEdit;
        remoteExeLineEdit->setText(runConfig->remoteExecutableFilePath());
        fl->addRow(remoteExeLabel, remoteExeLineEdit);

        auto localExeLabel = new QLabel(RemoteLinuxCustomRunConfiguration::tr("Local executable:"));
        auto localExeChooser = new PathChooser;
        localExeChooser->setFileName(FileName::fromString(runConfig->localExecutableFilePath()));
        fl->addRow(localExeLabel, localExeChooser);

        runConfig->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, fl);
        runConfig->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, fl);

        localExeChooser->setExpectedKind(PathChooser::File);
        localExeChooser->setPath(runConfig->localExecutableFilePath());

        connect(localExeChooser, &PathChooser::pathChanged, this,
                [runConfig](const QString &path) {
                    runConfig->setLocalExecutableFilePath(path.trimmed());
                });

        connect(remoteExeLineEdit, &QLineEdit::textEdited, this,
                [runConfig](const QString &path) {
                    runConfig->setRemoteExecutableFilePath(path.trimmed());
                });
    }
};

RemoteLinuxCustomRunConfiguration::RemoteLinuxCustomRunConfiguration(Target *target)
    : RunConfiguration(target, runConfigId())
{
    addExtraAspect(new ArgumentsAspect(this, "RemoteLinux.CustomRunConfig.Arguments"));
    addExtraAspect(new WorkingDirectoryAspect(this, "RemoteLinux.CustomRunConfig.WorkingDirectory"));
    addExtraAspect(new RemoteLinuxEnvironmentAspect(this));
    setDefaultDisplayName(runConfigDefaultDisplayName());
}

bool RemoteLinuxCustomRunConfiguration::isConfigured() const
{
    return !m_remoteExecutable.isEmpty();
}

RunConfiguration::ConfigurationState
RemoteLinuxCustomRunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (!isConfigured()) {
        if (errorMessage) {
            *errorMessage = tr("The remote executable must be set "
                               "in order to run a custom remote run configuration.");
        }
        return UnConfigured;
    }
    return Configured;
}

QWidget *RemoteLinuxCustomRunConfiguration::createConfigurationWidget()
{
    return wrapWidget(new RemoteLinuxCustomRunConfigWidget(this));
}

Utils::OutputFormatter *RemoteLinuxCustomRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

Runnable RemoteLinuxCustomRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.environment = extraAspect<RemoteLinuxEnvironmentAspect>()->environment();
    r.executable = m_remoteExecutable;
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    return r;
}

void RemoteLinuxCustomRunConfiguration::setRemoteExecutableFilePath(const QString &executable)
{
    m_remoteExecutable = executable;
    setDisplayName(runConfigDefaultDisplayName());
}

Core::Id RemoteLinuxCustomRunConfiguration::runConfigId()
{
    return "RemoteLinux.CustomRunConfig";
}

QString RemoteLinuxCustomRunConfiguration::runConfigDefaultDisplayName()
{
    QString display = m_remoteExecutable.isEmpty()
            ? tr("Custom Executable") : tr("Run \"%1\"").arg(m_remoteExecutable);
    return  RunConfigurationFactory::decoratedTargetName(display, target());
}

static QString localExeKey()
{
    return QLatin1String("RemoteLinux.CustomRunConfig.LocalExecutable");
}

static QString remoteExeKey()
{
    return QLatin1String("RemoteLinux.CustomRunConfig.RemoteExecutable");
}

bool RemoteLinuxCustomRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;
    setLocalExecutableFilePath(map.value(localExeKey()).toString());
    setRemoteExecutableFilePath(map.value(remoteExeKey()).toString());
    return true;
}

QVariantMap RemoteLinuxCustomRunConfiguration::toMap() const
{
    QVariantMap map = RunConfiguration::toMap();
    map.insert(localExeKey(), m_localExecutable);
    map.insert(remoteExeKey(), m_remoteExecutable);
    return map;
}

// RemoteLinuxCustomRunConfigurationFactory

RemoteLinuxCustomRunConfigurationFactory::RemoteLinuxCustomRunConfigurationFactory()
    : FixedRunConfigurationFactory(RemoteLinuxCustomRunConfiguration::tr("Custom Executable"), true)
{
    registerRunConfiguration<RemoteLinuxCustomRunConfiguration>
            (RemoteLinuxCustomRunConfiguration::runConfigId());
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

} // namespace Internal
} // namespace RemoteLinux
