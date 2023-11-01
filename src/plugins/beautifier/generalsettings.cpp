// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generalsettings.h"

#include "beautifierconstants.h"
#include "beautifiertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/genericconstants.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Beautifier::Internal {

GeneralSettings &generalSettings()
{
    static GeneralSettings theSettings;
    return theSettings;
}

GeneralSettings::GeneralSettings()
{
    setAutoApply(false);
    setSettingsGroups("Beautifier", "General");

    autoFormatOnSave.setSettingsKey(Utils::Constants::BEAUTIFIER_AUTO_FORMAT_ON_SAVE);
    autoFormatOnSave.setDefaultValue(false);
    autoFormatOnSave.setLabelText(Tr::tr("Enable auto format on file save"));

    autoFormatOnlyCurrentProject.setSettingsKey("autoFormatOnlyCurrentProject");
    autoFormatOnlyCurrentProject.setDefaultValue(true);
    autoFormatOnlyCurrentProject.setLabelText(Tr::tr("Restrict to files contained in the current project"));

    autoFormatTools.setSettingsKey("autoFormatTool");
    autoFormatTools.setLabelText(Tr::tr("Tool:"));
    autoFormatTools.setDefaultValue(0);
    autoFormatTools.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);

    autoFormatMime.setSettingsKey("autoFormatMime");
    autoFormatMime.setDefaultValue("text/x-c++src;text/x-c++hdr");
    autoFormatMime.setLabelText(Tr::tr("Restrict to MIME types:"));
    autoFormatMime.setDisplayStyle(StringAspect::LineEditDisplay);

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("Automatic Formatting on File Save")),
                autoFormatOnSave.groupChecker(),
                Form {
                    autoFormatTools, br,
                    autoFormatMime, br,
                    Span(2, autoFormatOnlyCurrentProject)
                }
            },
            st
        };
    });
    readSettings();
}

QList<MimeType> GeneralSettings::allowedMimeTypes() const
{
    const QStringList stringTypes = autoFormatMime().split(';');

    QList<MimeType> types;
    for (QString t : stringTypes) {
        t = t.trimmed();
        const MimeType mime = Utils::mimeTypeForName(t);
        if (mime.isValid())
            types << mime;
    }
    return types;
}

class GeneralSettingsPage final : public Core::IOptionsPage
{
public:
    GeneralSettingsPage()
    {
        setId(Constants::OPTION_GENERAL_ID);
        setDisplayName(Tr::tr("General"));
        setCategory(Constants::OPTION_CATEGORY);
        setDisplayCategory(Tr::tr("Beautifier"));
        setCategoryIconPath(":/beautifier/images/settingscategory_beautifier.png");
        setSettingsProvider([] { return &generalSettings(); });
    }
};

const GeneralSettingsPage settingsPage;

} // Beautifier::Internal
