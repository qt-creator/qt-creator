// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatconstants.h"
#include "clangformatsettings.h"
#include "clangformattr.h"

namespace ClangFormat {

ClangFormatSettings &clangFormatSettings()
{
    static ClangFormatSettings settings;
    return settings;
}

ClangFormatSettings::ClangFormatSettings()
{
    setAutoApply(false);
    setSettingsGroup("ClangFormat");

    mode.setSettingsKey(Constants::MODE_ID);
    mode.addOption(Tr::tr("Indenting only"));
    mode.addOption(Tr::tr("Full formatting"));
    mode.addOption(Tr::tr("Disable"));

    useCustomSettings.setSettingsKey(Constants::USE_CUSTOM_SETTINGS_ID);

    formatWhileTyping.setSettingsKey(Constants::FORMAT_WHILE_TYPING_ID);

    formatOnSave.setSettingsKey(Constants::FORMAT_CODE_ON_SAVE_ID);

    fileSizeThreshold.setSettingsKey(Constants::FILE_SIZE_THREDSHOLD);
    fileSizeThreshold.setDefaultValue(200);

    readSettings();
}

} // namespace ClangFormat
