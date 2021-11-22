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

#include "settings.h"

#include "cpasterconstants.h"

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace CodePaster {

Settings::Settings()
{
    setSettingsGroup("CodePaster");
    setAutoApply(false);

    registerAspect(&username);
    username.setDisplayStyle(StringAspect::LineEditDisplay);
    username.setSettingsKey("UserName");
    username.setLabelText(tr("Username:"));

    registerAspect(&protocols);
    protocols.setSettingsKey("DefaultProtocol");
    protocols.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    protocols.setLabelText(tr("Default protocol:"));
    protocols.setToSettingsTransformation([this](const QVariant &val) {
        return protocols.displayForIndex(val.toInt());
    });
    protocols.setFromSettingsTransformation([this](const QVariant &val) {
        return protocols.indexForDisplay(val.toString());
    });

    registerAspect(&expiryDays);
    expiryDays.setSettingsKey("ExpiryDays");
    expiryDays.setDefaultValue(1);
    expiryDays.setSuffix(tr(" Days"));
    expiryDays.setLabelText(tr("&Expires after:"));

    registerAspect(&copyToClipboard);
    copyToClipboard.setSettingsKey("CopyToClipboard");
    copyToClipboard.setDefaultValue(true);
    copyToClipboard.setLabelText(tr("Copy-paste URL to clipboard"));

    registerAspect(&displayOutput);
    displayOutput.setSettingsKey("DisplayOutput");
    displayOutput.setDefaultValue(true);
    displayOutput.setLabelText(tr("Display Output pane after sending a post"));
}

// SettingsPage

SettingsPage::SettingsPage(Settings *settings)
{
    setId("A.CodePaster.General");
    setDisplayName(Settings::tr("General"));
    setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
    setDisplayCategory(Settings::tr("Code Pasting"));
    setCategoryIconPath(":/cpaster/images/settingscategory_cpaster.png");
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        Settings &s = *settings;
        using namespace Layouting;

        Column {
            Form {
                s.protocols,
                s.username,
                s.expiryDays
            },
            s.copyToClipboard,
            s.displayOutput,
            Stretch()
        }.attachTo(widget);
    });
}

} // namespace CodePaster
