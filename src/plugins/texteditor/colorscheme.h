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

#include <QMap>
#include <QString>
#include <QColor>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace TextEditor {

/*! Format for a particular piece of text (text/comment, etc). */
class TEXTEDITOR_EXPORT Format
{
public:
    Format() = default;
    Format(const QColor &foreground, const QColor &background);

    QColor foreground() const { return m_foreground; }
    void setForeground(const QColor &foreground);

    QColor background() const { return m_background; }
    void setBackground(const QColor &background);

    double relativeForegroundSaturation() const { return m_relativeForegroundSaturation; }
    void setRelativeForegroundSaturation(double relativeForegroundSaturation);

    double relativeForegroundLightness() const { return m_relativeForegroundLightness; }
    void setRelativeForegroundLightness(double relativeForegroundLightness);

    double relativeBackgroundSaturation() const { return m_relativeBackgroundSaturation; }
    void setRelativeBackgroundSaturation(double relativeBackgroundSaturation);

    double relativeBackgroundLightness() const { return m_relativeBackgroundLightness; }
    void setRelativeBackgroundLightness(double relativeBackgroundLightness);

    bool bold() const { return m_bold; }
    void setBold(bool bold);

    bool italic() const { return m_italic; }
    void setItalic(bool italic);

    void setUnderlineColor(const QColor &underlineColor);
    QColor underlineColor() const;

    void setUnderlineStyle(QTextCharFormat::UnderlineStyle underlineStyle);
    QTextCharFormat::UnderlineStyle underlineStyle() const;

    bool equals(const Format &f) const;

    QString toString() const;
    bool fromString(const QString &str);

private:
    QColor m_foreground = Qt::black;
    QColor m_background = Qt::white;
    QColor m_underlineColor;
    double m_relativeForegroundSaturation = 0.0;
    double m_relativeForegroundLightness = 0.0;
    double m_relativeBackgroundSaturation = 0.0;
    double m_relativeBackgroundLightness = 0.0;
    QTextCharFormat::UnderlineStyle m_underlineStyle = QTextCharFormat::NoUnderline;
    bool m_bold = false;
    bool m_italic = false;
};

inline bool operator==(const Format &f1, const Format &f2) { return f1.equals(f2); }
inline bool operator!=(const Format &f1, const Format &f2) { return !f1.equals(f2); }


/*! A color scheme combines a set of formats for different highlighting
    categories. It also provides saving and loading of the scheme to a file.
 */
class ColorScheme
{
public:
    void setDisplayName(const QString &name)
    { m_displayName = name; }

    QString displayName() const
    { return m_displayName; }

    inline bool isEmpty() const
    { return m_formats.isEmpty(); }

    bool contains(TextStyle category) const;

    Format &formatFor(TextStyle category);
    Format formatFor(TextStyle category) const;

    void setFormatFor(TextStyle category, const Format &format);

    void clear();

    bool save(const QString &fileName, QWidget *parent) const;
    bool load(const QString &fileName);

    inline bool equals(const ColorScheme &cs) const
    {
        return m_formats == cs.m_formats
                && m_displayName == cs.m_displayName;
    }

    static QString readNameOfScheme(const QString &fileName);

private:
    QMap<TextStyle, Format> m_formats;
    QString m_displayName;
};

inline bool operator==(const ColorScheme &cs1, const ColorScheme &cs2) { return cs1.equals(cs2); }
inline bool operator!=(const ColorScheme &cs1, const ColorScheme &cs2) { return !cs1.equals(cs2); }

} // namespace TextEditor
