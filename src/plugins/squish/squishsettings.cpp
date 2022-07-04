/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishsettings.h"

#include "squishconstants.h"

#include <utils/icon.h>
#include <utils/layoutbuilder.h>

#include <QSettings>

using namespace Utils;

namespace Squish {
namespace Internal {

SquishSettings::SquishSettings()
{
    setSettingsGroup("Squish");
    setAutoApply(false);

    registerAspect(&squishPath);
    squishPath.setSettingsKey("SquishPath");
    squishPath.setLabelText(tr("Squish path:"));
    squishPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    squishPath.setExpectedKind(PathChooser::ExistingDirectory);
    squishPath.setPlaceHolderText(tr("Path to Squish installation"));

    registerAspect(&licensePath);
    licensePath.setSettingsKey("LicensePath");
    licensePath.setLabelText(tr("License path:"));
    licensePath.setDisplayStyle(StringAspect::PathChooserDisplay);
    licensePath.setExpectedKind(PathChooser::ExistingDirectory);

    registerAspect(&local);
    local.setSettingsKey("Local");
    local.setLabel(tr("Local Server"));
    local.setDefaultValue(true);

    registerAspect(&serverHost);
    serverHost.setSettingsKey("ServerHost");
    serverHost.setLabelText(tr("Server host:"));
    serverHost.setDisplayStyle(StringAspect::LineEditDisplay);
    serverHost.setDefaultValue("localhost");
    serverHost.setEnabled(false);

    registerAspect(&serverPort);
    serverPort.setSettingsKey("ServerPort");
    serverPort.setLabel(tr("Server Port"));
    serverPort.setRange(1, 65535);
    serverPort.setDefaultValue(9999);
    serverPort.setEnabled(false);


    registerAspect(&verbose);
    verbose.setSettingsKey("Verbose");
    verbose.setLabel(tr("Verbose log"));
    verbose.setDefaultValue(false);

    connect(&local, &BoolAspect::volatileValueChanged,
            this, [this] (bool checked) {
        serverHost.setEnabled(!checked);
        serverPort.setEnabled(!checked);
    });
}

SquishSettingsPage::SquishSettingsPage(SquishSettings *settings)
{
    setId("A.Squish.General");
    setDisplayName(tr("General"));
    setCategory(Constants::SQUISH_SETTINGS_CATEGORY);
    setDisplayCategory("Squish");
    setCategoryIcon(Icon({{":/squish/images/settingscategory_squish.png",
                           Theme::PanelTextColorDark}}, Icon::Tint));

    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        SquishSettings &s = *settings;
        using namespace Layouting;

        const Break nl;

        Grid grid {
            s.squishPath, nl,
            s.licensePath, nl,
            Span {2, Row { s.local, s.serverHost, s.serverPort } }, nl,
            s.verbose, nl,
        };
        Column { Row { grid }, Stretch() }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace Squish
