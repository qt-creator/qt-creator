// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settings.h"

#include "cpasterconstants.h"
#include "cpastertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace CodePaster {

Settings &settings()
{
    static Settings theSettings;
    return theSettings;
}

Settings::Settings()
{
    setSettingsGroup("CodePaster");
    setAutoApply(false);

    username.setDisplayStyle(StringAspect::LineEditDisplay);
    username.setSettingsKey("UserName");
    username.setLabelText(Tr::tr("Username:"));

    protocols.setSettingsKey("DefaultProtocol");
    protocols.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    protocols.setLabelText(Tr::tr("Default protocol:"));
    protocols.setToSettingsTransformation([this](const QVariant &val) {
        return protocols.displayForIndex(val.toInt());
    });
    protocols.setFromSettingsTransformation([this](const QVariant &val) {
        return protocols.indexForDisplay(val.toString());
    });

    expiryDays.setSettingsKey("ExpiryDays");
    expiryDays.setDefaultValue(1);
    expiryDays.setSuffix(Tr::tr(" Days"));
    expiryDays.setLabelText(Tr::tr("&Expires after:"));

    copyToClipboard.setSettingsKey("CopyToClipboard");
    copyToClipboard.setDefaultValue(true);
    copyToClipboard.setLabelText(Tr::tr("Copy-paste URL to clipboard"));

    displayOutput.setSettingsKey("DisplayOutput");
    displayOutput.setDefaultValue(true);
    displayOutput.setLabelText(Tr::tr("Display General Messages after sending a post"));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Form {
                protocols, br,
                username, br,
                expiryDays
            },
            copyToClipboard,
            displayOutput,
            st
        };
    });
}

class CPasterSettingsPage final : public Core::IOptionsPage
{
public:
    CPasterSettingsPage()
    {
        setId("A.CodePaster.General");
        setDisplayName(Tr::tr("General"));
        setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
        setDisplayCategory(Tr::tr("Code Pasting"));
        setCategoryIconPath(":/cpaster/images/settingscategory_cpaster.png");
        setSettingsProvider([] { return &settings(); });
    }
};

const CPasterSettingsPage settingsPage;

} // CodePaster
