/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "dockersettings.h"

#include "dockerconstants.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/filepath.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Docker {
namespace Internal {

DockerSettings::DockerSettings()
{
    setSettingsGroup(Constants::DOCKER);
    setAutoApply(false);

    registerAspect(&dockerBinaryPath);
    dockerBinaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    dockerBinaryPath.setExpectedKind(PathChooser::ExistingCommand);
    dockerBinaryPath.setDefaultFilePath(FilePath::fromString("docker").searchInPath({"/usr/local/bin"}));
    dockerBinaryPath.setDisplayName(tr("Docker CLI"));
    dockerBinaryPath.setHistoryCompleter("Docker.Command.History");
    dockerBinaryPath.setLabelText(tr("Command:"));
    dockerBinaryPath.setSettingsKey("cli");

    readSettings(Core::ICore::settings());
}

// DockerSettingsPage

DockerSettingsPage::DockerSettingsPage(DockerSettings *settings)
{
    setId(Docker::Constants::DOCKER_SETTINGS_ID);
    setDisplayName(DockerSettings::tr("Docker"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        DockerSettings &s = *settings;
        using namespace Layouting;

        // clang-format off
        Column {
            Group {
                Title(DockerSettings::tr("Configuration")),
                Row { s.dockerBinaryPath }
            },
            Stretch()
        }.attachTo(widget);
        // clang-format on
    });
}

} // namespace Internal
} // namespace Docker
