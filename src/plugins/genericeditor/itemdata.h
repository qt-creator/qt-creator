/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef ITEMDATA_H
#define ITEMDATA_H

#include <QtCore/QString>

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QTextCharFormat>

namespace GenericEditor {
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

    void setStrikedOut(const QString &striked);
    bool isStrikedOut() const;
    bool isStrikedOutSpecified() const;

    static const QLatin1String kDsNormal;
    static const QLatin1String kDsKeyword;
    static const QLatin1String kDsDataType;
    static const QLatin1String kDsDecVal;
    static const QLatin1String kDsBaseN;
    static const QLatin1String kDsFloat;
    static const QLatin1String kDsChar;
    static const QLatin1String kDsString;
    static const QLatin1String kDsComment;
    static const QLatin1String kDsOthers;
    static const QLatin1String kDsAlert;
    static const QLatin1String kDsFunction;
    static const QLatin1String kDsRegionMarker;
    static const QLatin1String kDsError;

private:
    QString m_style;
    QColor m_color;
    QColor m_selectionColor;
    bool m_italic;
    bool m_italicSpecified;
    bool m_bold;
    bool m_boldSpecified;
    bool m_underlined;
    bool m_underlinedSpecified;
    bool m_strikedOut;
    bool m_strikedOutSpecified;
};

} // namespace Internal
} // namespace GenericEditor

#endif // ITEMDATA_H
