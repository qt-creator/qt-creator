/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "clangformatconstants.h"
#include "clangformatsettings.h"

#include <coreplugin/icore.h>

namespace ClangFormat {

ClangFormatSettings &ClangFormatSettings::instance()
{
    static ClangFormatSettings settings;
    return settings;
}

ClangFormatSettings::ClangFormatSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_ID));
    m_formatCodeInsteadOfIndent
        = settings->value(QLatin1String(Constants::FORMAT_CODE_INSTEAD_OF_INDENT_ID), false).toBool();
    m_formatWhileTyping = settings->value(QLatin1String(Constants::FORMAT_WHILE_TYPING_ID), false)
                              .toBool();
    m_formatOnSave = settings->value(QLatin1String(Constants::FORMAT_CODE_ON_SAVE_ID), false)
                         .toBool();
    m_overrideDefaultFile = settings->value(QLatin1String(Constants::OVERRIDE_FILE_ID), false)
                                .toBool();
    settings->endGroup();
}

void ClangFormatSettings::write() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_ID));
    settings->setValue(QLatin1String(Constants::FORMAT_CODE_INSTEAD_OF_INDENT_ID),
                       m_formatCodeInsteadOfIndent);
    settings->setValue(QLatin1String(Constants::FORMAT_WHILE_TYPING_ID), m_formatWhileTyping);
    settings->setValue(QLatin1String(Constants::FORMAT_CODE_ON_SAVE_ID), m_formatOnSave);
    settings->setValue(QLatin1String(Constants::OVERRIDE_FILE_ID), m_overrideDefaultFile);
    settings->endGroup();
}

void ClangFormatSettings::setFormatCodeInsteadOfIndent(bool enable)
{
    m_formatCodeInsteadOfIndent = enable;
}

bool ClangFormatSettings::formatCodeInsteadOfIndent() const
{
    return m_formatCodeInsteadOfIndent;
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

void ClangFormatSettings::setOverrideDefaultFile(bool enable)
{
    m_overrideDefaultFile = enable;
}

bool ClangFormatSettings::overrideDefaultFile() const
{
    return m_overrideDefaultFile;
}

} // namespace ClangFormat
