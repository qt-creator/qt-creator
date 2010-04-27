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

namespace Highlight {
namespace Internal {

class ItemData
{
public:
    ItemData();

    void setStyle(const QString &style);
    void setColor(const QString &color);
    void setSelectionColor(const QString &color);
    void setItalic(const QString &italic);
    void setBold(const QString &bold);
    void setUnderlined(const QString &underlined);
    void setStrikedOut(const QString &striked);    
    void configureFormat();

    const QTextCharFormat &format() const;

private:
    QString m_style;
    QColor m_color;
    QColor m_selectionColor;
    QFont m_font;
    QTextCharFormat m_format;
};

} // namespace Internal
} // namespace Highlight

#endif // ITEMDATA_H
