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

#pragma once

#include "texteditor_global.h"

#include "texteditorconstants.h"
#include "colorscheme.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QColor;
QT_END_NAMESPACE

namespace TextEditor {

class Format;
class FontSettings;
namespace Internal { class FontSettingsPagePrivate; }

// GUI description of a format consisting of id (settings key)
// and displayName to be displayed
class TEXTEDITOR_EXPORT FormatDescription
{
public:
    enum ShowControls {
        ShowForegroundControl = 0x1,
        ShowBackgroundControl = 0x2,
        ShowFontControls = 0x4,
        ShowUnderlineControl = 0x8,
        ShowRelativeForegroundControl = 0x10,
        ShowRelativeBackgroundControl = 0x20,
        ShowRelativeControls = ShowRelativeForegroundControl | ShowRelativeBackgroundControl,
        ShowFontUnderlineAndRelativeControls = ShowFontControls
                                             | ShowUnderlineControl
                                             | ShowRelativeControls,
        ShowAllAbsoluteControls = ShowForegroundControl
                                | ShowBackgroundControl
                                | ShowFontControls
                                | ShowUnderlineControl,
        ShowAllAbsoluteControlsExceptUnderline = ShowAllAbsoluteControls & ~ShowUnderlineControl,
        ShowAllControls = ShowAllAbsoluteControls | ShowRelativeControls
    };
    FormatDescription() = default;

    FormatDescription(TextStyle id,
                      const QString &displayName,
                      const QString &tooltipText,
                      ShowControls showControls = ShowAllAbsoluteControls);

    FormatDescription(TextStyle id,
                      const QString &displayName,
                      const QString &tooltipText,
                      const QColor &foreground,
                      ShowControls showControls = ShowAllAbsoluteControls);
    FormatDescription(TextStyle id,
                      const QString &displayName,
                      const QString &tooltipText,
                      const Format &format,
                      ShowControls showControls = ShowAllAbsoluteControls);
    FormatDescription(TextStyle id,
                      const QString &displayName,
                      const QString &tooltipText,
                      const QColor &underlineColor,
                      const QTextCharFormat::UnderlineStyle underlineStyle,
                      ShowControls showControls = ShowAllAbsoluteControls);

    TextStyle id() const { return m_id; }

    QString displayName() const
    { return m_displayName; }

    static QColor defaultForeground(TextStyle id);
    static QColor defaultBackground(TextStyle id);

    const Format &format() const { return m_format; }
    Format &format() { return m_format; }

    QString tooltipText() const
    { return  m_tooltipText; }

    bool showControl(ShowControls showControl) const;

private:
    TextStyle m_id;            // Name of the category
    Format m_format;            // Default format
    QString m_displayName;      // Displayed name of the category
    QString m_tooltipText;      // Description text for category
    ShowControls m_showControls = ShowAllAbsoluteControls;
};

using FormatDescriptions = std::vector<FormatDescription>;

class TEXTEDITOR_EXPORT FontSettingsPage final : public Core::IOptionsPage
{
public:
    FontSettingsPage(FontSettings *fontSettings, const FormatDescriptions &fd);

    void setFontZoom(int zoom);
};

} // namespace TextEditor
