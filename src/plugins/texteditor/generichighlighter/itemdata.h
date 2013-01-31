/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ITEMDATA_H
#define ITEMDATA_H

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

#endif // ITEMDATA_H
