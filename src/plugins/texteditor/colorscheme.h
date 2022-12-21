// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

namespace Utils { class FilePath; }

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

    friend bool operator==(const Format &f1, const Format &f2) { return f1.equals(f2); }
    friend bool operator!=(const Format &f1, const Format &f2) { return !f1.equals(f2); }

private:
    QColor m_foreground;
    QColor m_background;
    QColor m_underlineColor;
    double m_relativeForegroundSaturation = 0.0;
    double m_relativeForegroundLightness = 0.0;
    double m_relativeBackgroundSaturation = 0.0;
    double m_relativeBackgroundLightness = 0.0;
    QTextCharFormat::UnderlineStyle m_underlineStyle = QTextCharFormat::NoUnderline;
    bool m_bold = false;
    bool m_italic = false;
};

/*! A color scheme combines a set of formats for different highlighting
    categories. It also provides saving and loading of the scheme to a file.
 */
class TEXTEDITOR_EXPORT ColorScheme
{
public:
    void setDisplayName(const QString &name) { m_displayName = name; }
    QString displayName() const { return m_displayName; }

    bool isEmpty() const { return m_formats.isEmpty(); }

    bool contains(TextStyle category) const;

    Format &formatFor(TextStyle category);
    Format formatFor(TextStyle category) const;

    void setFormatFor(TextStyle category, const Format &format);

    void clear();

    bool save(const Utils::FilePath &filePath, QWidget *parent) const;
    bool load(const Utils::FilePath &filePath);

    bool equals(const ColorScheme &cs) const
    {
        return m_formats == cs.m_formats && m_displayName == cs.m_displayName;
    }

    static QString readNameOfScheme(const Utils::FilePath &filePath);

    friend bool operator==(const ColorScheme &cs1, const ColorScheme &cs2) { return cs1.equals(cs2); }
    friend bool operator!=(const ColorScheme &cs1, const ColorScheme &cs2) { return !cs1.equals(cs2); }

private:
    QMap<TextStyle, Format> m_formats;
    QString m_displayName;
};

} // namespace TextEditor
