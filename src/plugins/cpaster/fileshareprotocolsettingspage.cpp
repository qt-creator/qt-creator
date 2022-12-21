// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileshareprotocolsettingspage.h"

#include "cpastertr.h"
#include "cpasterconstants.h"

#include <utils/layoutbuilder.h>
#include <utils/temporarydirectory.h>

using namespace Utils;

namespace CodePaster {

FileShareProtocolSettings::FileShareProtocolSettings()
{
    setSettingsGroup("FileSharePasterSettings");
    setAutoApply(false);

    registerAspect(&path);
    path.setSettingsKey("Path");
    path.setDisplayStyle(StringAspect::PathChooserDisplay);
    path.setExpectedKind(PathChooser::ExistingDirectory);
    path.setDefaultValue(TemporaryDirectory::masterDirectoryPath());
    path.setLabelText(Tr::tr("&Path:"));

    registerAspect(&displayCount);
    displayCount.setSettingsKey("DisplayCount");
    displayCount.setDefaultValue(10);
    displayCount.setSuffix(' ' + Tr::tr("entries"));
    displayCount.setLabelText(Tr::tr("&Display:"));
}

// Settings page

FileShareProtocolSettingsPage::FileShareProtocolSettingsPage(FileShareProtocolSettings *settings)
{
    setId("X.CodePaster.FileSharePaster");
    setDisplayName(Tr::tr("Fileshare"));
    setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
    setSettings(settings);

    setLayouter([&s = *settings](QWidget *widget) {
        using namespace Layouting;

        auto label = new QLabel(Tr::tr(
            "The fileshare-based paster protocol allows for sharing code snippets using "
            "simple files on a shared network drive. Files are never deleted."));
        label->setWordWrap(true);

        Column {
            Form {
                label, br,
                s.path,
                s.displayCount
            },
            st
        }.attachTo(widget);
    });
}

} // namespace CodePaster
