/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include "texteditor_global.h"

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtGui/QColor>

namespace TextEditor {

/*! Format for a particular piece of text (text/comment, etc). */
class TEXTEDITOR_EXPORT Format
{
public:
    Format();

    QColor foreground() const { return m_foreground; }
    void setForeground(const QColor &foreground);

    QColor background() const { return m_background; }
    void setBackground(const QColor &background);

    bool bold() const { return m_bold; }
    void setBold(bool bold);

    bool italic() const { return m_italic; }
    void setItalic(bool italic);

    bool equals(const Format &f) const;

    QString toString() const;
    bool fromString(const QString &str);

private:
    QColor m_foreground;
    QColor m_background;
    bool m_bold;
    bool m_italic;
};

inline bool operator==(const Format &f1, const Format &f2) { return f1.equals(f2); }
inline bool operator!=(const Format &f1, const Format &f2) { return !f1.equals(f2); }


/*! A color scheme combines a set of formats for different highlighting
    categories. It also provides saving and loading of the scheme to a file.
 */
class ColorScheme
{
public:
    ColorScheme();

    void setDisplayName(const QString &name)
    { m_displayName = name; }

    QString displayName() const
    { return m_displayName; }

    inline bool isEmpty() const
    { return m_formats.isEmpty(); }

    bool contains(const QString &category) const;

    Format &formatFor(const QString &category);
    Format formatFor(const QString &category) const;

    void setFormatFor(const QString &category, const Format &format);

    void clear();

    bool save(const QString &fileName) const;
    bool load(const QString &fileName);

    inline bool equals(const ColorScheme &cs) const
    {
        return m_formats == cs.m_formats
                && m_displayName == cs.m_displayName;
    }

    static QString readNameOfScheme(const QString &fileName);

private:
    QMap<QString, Format> m_formats;
    QString m_displayName;
};

inline bool operator==(const ColorScheme &cs1, const ColorScheme &cs2) { return cs1.equals(cs2); }
inline bool operator!=(const ColorScheme &cs1, const ColorScheme &cs2) { return !cs1.equals(cs2); }

} // namespace TextEditor

#endif // COLORSCHEME_H
