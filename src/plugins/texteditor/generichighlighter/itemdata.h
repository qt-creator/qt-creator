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

#include <QString>
#include <QColor>

namespace TextEditor {
namespace Internal {

class ItemData
{
public:
    ItemData();

    void setStyle(const QString &style);
    const QString &style() const;

    void setColor(const QString &color);
    const QColor &color() const;

    void setSelectionColor(const QString &color);
    const QColor &selectionColor() const;

    void setItalic(const QString &italic);
    bool isItalic() const;
    bool isItalicSpecified() const;

    void setBold(const QString &bold);
    bool isBold() const;
    bool isBoldSpecified() const;

    void setUnderlined(const QString &underlined);
    bool isUnderlined() const;
    bool isUnderlinedSpecified() const;

    void setStrikeOut(const QString &strike);
    bool isStrikeOut() const;
    bool isStrikeOutSpecified() const;

    bool isCustomized() const;

private:
    bool m_italic;
    bool m_italicSpecified;
    bool m_bold;
    bool m_boldSpecified;
    bool m_underlined;
    bool m_underlinedSpecified;
    bool m_strikedOut;
    bool m_strikeOutSpecified;
    bool m_isCustomized;
    QString m_style;
    QColor m_color;
    QColor m_selectionColor;
};

} // namespace Internal
} // namespace TextEditor
