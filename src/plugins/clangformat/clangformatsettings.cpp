// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include <coreplugin/icore.h>

using namespace Utils;

namespace ClangFormat {

const char FORMAT_CODE_INSTEAD_OF_INDENT_ID[] = "ClangFormat.FormatCodeInsteadOfIndent";

ClangFormatSettings &ClangFormatSettings::instance()
{
    static ClangFormatSettings settings;
    return settings;
}

ClangFormatSettings::ClangFormatSettings()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::SETTINGS_ID);
    m_overrideDefaultFile = settings->value(Constants::OVERRIDE_FILE_ID, false).toBool();
    m_formatWhileTyping = settings->value(Constants::FORMAT_WHILE_TYPING_ID, false).toBool();
    m_formatOnSave = settings->value(Constants::FORMAT_CODE_ON_SAVE_ID, false).toBool();
    m_fileSizeThreshold = settings->value(Constants::FILE_SIZE_THREDSHOLD,
                                          m_fileSizeThreshold).toInt();

    // Convert old settings to new ones. New settings were added to QtC 8.0
    bool isOldFormattingOn = settings->value(FORMAT_CODE_INSTEAD_OF_INDENT_ID, false).toBool();
    Core::ICore::settings()->remove(FORMAT_CODE_INSTEAD_OF_INDENT_ID);

    if (isOldFormattingOn) {
        settings->setValue(Constants::MODE_ID,
                           static_cast<int>(ClangFormatSettings::Mode::Formatting));
        m_mode = ClangFormatSettings::Mode::Formatting;
    } else
        m_mode = static_cast<ClangFormatSettings::Mode>(
            settings->value(Constants::MODE_ID, ClangFormatSettings::Mode::Indenting).toInt());

    settings->endGroup();
}

void ClangFormatSettings::write() const
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::SETTINGS_ID);
    settings->setValue(Constants::OVERRIDE_FILE_ID, m_overrideDefaultFile);
    settings->setValue(Constants::FORMAT_WHILE_TYPING_ID, m_formatWhileTyping);
    settings->setValue(Constants::FORMAT_CODE_ON_SAVE_ID, m_formatOnSave);
    settings->setValue(Constants::MODE_ID, static_cast<int>(m_mode));
    settings->setValue(Constants::FILE_SIZE_THREDSHOLD, m_fileSizeThreshold);
    settings->endGroup();
}

void ClangFormatSettings::setOverrideDefaultFile(bool enable)
{
    m_overrideDefaultFile = enable;
}

bool ClangFormatSettings::overrideDefaultFile() const
{
    return m_overrideDefaultFile;
}

void ClangFormatSettings::setFormatWhileTyping(bool enable)
{
    m_formatWhileTyping = enable;
}

bool ClangFormatSettings::formatWhileTyping() const
{
    return m_formatWhileTyping;
}

void ClangFormatSettings::setFormatOnSave(bool enable)
{
    m_formatOnSave = enable;
}

bool ClangFormatSettings::formatOnSave() const
{
    return m_formatOnSave;
}

void ClangFormatSettings::setMode(Mode mode)
{
    m_mode = mode;
}

ClangFormatSettings::Mode ClangFormatSettings::mode() const
{
    return m_mode;
}

void ClangFormatSettings::setFileSizeThreshold(int fileSizeInKb)
{
    m_fileSizeThreshold = fileSizeInKb;
}

int ClangFormatSettings::fileSizeThreshold() const
{
    return m_fileSizeThreshold;
}

} // namespace ClangFormat
