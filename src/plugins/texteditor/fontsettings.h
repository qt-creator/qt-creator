/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef FONTSETTINGS_H
#define FONTSETTINGS_H

#include "texteditor_global.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QVector>
#include <QtGui/QColor>

QT_BEGIN_NAMESPACE
class QTextCharFormat;
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class FormatDescription;

// Format for a particular piece of text (text/comment, etc).
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

/**
 * Font settings (default font and enumerated list of formats).
 */
class TEXTEDITOR_EXPORT FontSettings
{
public:
    typedef QList<FormatDescription> FormatDescriptions;

    FontSettings(const FormatDescriptions &fd);
    void clear();

    void toSettings(const QString &category,
                    const FormatDescriptions &descriptions,
                    QSettings *s) const;

    bool fromSettings(const QString &category,
                      const FormatDescriptions &descriptions,
                      const QSettings *s);

    /**
     * Returns the list of QTextCharFormats that corresponds to the list of
     * requested format categories.
     */
    QVector<QTextCharFormat> toTextCharFormats(const QVector<QString> &categories) const;

    /**
     * Returns the QTextCharFormat of the given format category.
     */
    QTextCharFormat toTextCharFormat(const QString &category) const;

    /**
     * Returns the configured font family.
     */
    QString family() const;
    void setFamily(const QString &family);

    /**
     * Returns the configured font size.
     */
    int fontSize() const;
    void setFontSize(int size);

    /**
     * Returns the format for the given font category.
     */
    Format &formatFor(const QString &category);

    bool equals(const FontSettings &f) const;

    static QString defaultFixedFontFamily();
    static int defaultFontSize();

private:
    QString m_family;
    int m_fontSize;
    QMap<QString, Format> m_formats;
};

inline bool operator==(const FontSettings &f1, const FontSettings &f2) { return f1.equals(f2); }
inline bool operator!=(const FontSettings &f1, const FontSettings &f2) { return !f1.equals(f2); }

} // namespace TextEditor

#endif // FONTSETTINGS_H
