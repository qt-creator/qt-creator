// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileshareprotocolsettingspage.h"

#include "cpastertr.h"
#include "cpasterconstants.h"

#include <utils/layoutbuilder.h>
#include <utils/temporarydirectory.h>

using namespace Utils;

namespace CodePaster {

FileShareProtocolSettings &fileShareSettings()
{
    static FileShareProtocolSettings theSettings;
    return theSettings;
}

FileShareProtocolSettings::FileShareProtocolSettings()
{
    setAutoApply(false);
    setSettingsGroup("FileSharePasterSettings");

    path.setSettingsKey("Path");
    path.setExpectedKind(PathChooser::ExistingDirectory);
    path.setDefaultValue(TemporaryDirectory::masterDirectoryPath());
    path.setLabelText(Tr::tr("&Path:"));

    displayCount.setSettingsKey("DisplayCount");
    displayCount.setDefaultValue(10);
    displayCount.setSuffix(' ' + Tr::tr("entries"));
    displayCount.setLabelText(Tr::tr("&Display:"));

    setLayouter([this] {
        using namespace Layouting;

        auto label = new QLabel(Tr::tr(
            "The fileshare-based paster protocol allows for sharing code snippets using "
            "simple files on a shared network drive. Files are never deleted."));
        label->setWordWrap(true);

        return Column {
            Form {
                label, br,
                path, br,
                displayCount
            },
            st
        };
    });

    readSettings();
}

class FileShareProtocolSettingsPage final : public Core::IOptionsPage
{
public:
    FileShareProtocolSettingsPage()
    {
        setId("X.CodePaster.FileSharePaster");
        setDisplayName(Tr::tr("Fileshare"));
        setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &fileShareSettings(); });
    }
};

Core::IOptionsPage &fileShareSettingsPage()
{
    static FileShareProtocolSettingsPage theSettings;
    return theSettings;
}

} // namespace CodePaster
