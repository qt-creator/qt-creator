// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include <coreplugin/icore.h>

namespace ClangFormat {
static const char FORMAT_CODE_INSTEAD_OF_INDENT_ID[] = "ClangFormat.FormatCodeInsteadOfIndent";

ClangFormatSettings &ClangFormatSettings::instance()
{
    static ClangFormatSettings settings;
    return settings;
}

ClangFormatSettings::ClangFormatSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_ID));
    m_overrideDefaultFile = settings->value(QLatin1String(Constants::OVERRIDE_FILE_ID), false)
                                .toBool();
    m_formatWhileTyping = settings->value(QLatin1String(Constants::FORMAT_WHILE_TYPING_ID), false)
                              .toBool();
    m_formatOnSave = settings->value(QLatin1String(Constants::FORMAT_CODE_ON_SAVE_ID), false)
                         .toBool();

    // Convert old settings to new ones. New settings were added to QtC 8.0
    bool isOldFormattingOn
        = settings->value(QLatin1String(FORMAT_CODE_INSTEAD_OF_INDENT_ID), false).toBool();
    Core::ICore::settings()->remove(QLatin1String(FORMAT_CODE_INSTEAD_OF_INDENT_ID));

    if (isOldFormattingOn) {
        settings->setValue(QLatin1String(Constants::MODE_ID),
                           static_cast<int>(ClangFormatSettings::Mode::Formatting));
        m_mode = ClangFormatSettings::Mode::Formatting;
    } else
        m_mode = static_cast<ClangFormatSettings::Mode>(
            settings->value(QLatin1String(Constants::MODE_ID), ClangFormatSettings::Mode::Indenting)
                .toInt());

    settings->endGroup();
}

void ClangFormatSettings::write() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_ID));
    settings->setValue(QLatin1String(Constants::OVERRIDE_FILE_ID), m_overrideDefaultFile);
    settings->setValue(QLatin1String(Constants::FORMAT_WHILE_TYPING_ID), m_formatWhileTyping);
    settings->setValue(QLatin1String(Constants::FORMAT_CODE_ON_SAVE_ID), m_formatOnSave);
    settings->setValue(QLatin1String(Constants::MODE_ID), static_cast<int>(m_mode));
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

} // namespace ClangFormat
