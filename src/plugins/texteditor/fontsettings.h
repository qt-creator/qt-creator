// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "colorscheme.h"
#include "textstyles.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QTextCharFormat>
#include <QVector>

QT_BEGIN_NAMESPACE
class QSettings;
class QFont;
QT_END_NAMESPACE

namespace TextEditor {

class FormatDescription;

/**
 * Font settings (default font and enumerated list of formats).
 */
class TEXTEDITOR_EXPORT FontSettings
{
public:
    using FormatDescriptions = std::vector<FormatDescription>;

    FontSettings();
    void clear();
    inline bool isEmpty() const { return m_scheme.isEmpty(); }

    void toSettings(QSettings *s) const;

    bool fromSettings(const FormatDescriptions &descriptions,
                      const QSettings *s);

    QVector<QTextCharFormat> toTextCharFormats(const QVector<TextStyle> &categories) const;
    QTextCharFormat toTextCharFormat(TextStyle category) const;
    QTextCharFormat toTextCharFormat(TextStyles textStyles) const;

    QString family() const;
    void setFamily(const QString &family);

    int fontSize() const;
    void setFontSize(int size);

    int fontZoom() const;
    void setFontZoom(int zoom);

    qreal lineSpacing() const;
    int relativeLineSpacing() const;
    void setRelativeLineSpacing(int relativeLineSpacing);

    QFont font() const;

    bool antialias() const;
    void setAntialias(bool antialias);

    Format &formatFor(TextStyle category);
    Format formatFor(TextStyle category) const;

    QString colorSchemeFileName() const;
    void setColorSchemeFileName(const QString &fileName);
    bool loadColorScheme(const QString &fileName, const FormatDescriptions &descriptions);
    bool saveColorScheme(const QString &fileName);

    const ColorScheme &colorScheme() const;
    void setColorScheme(const ColorScheme &scheme);

    bool equals(const FontSettings &f) const;

    static QString defaultFixedFontFamily();
    static int defaultFontSize();

    static QString defaultSchemeFileName(const QString &fileName = QString());

    friend bool operator==(const FontSettings &f1, const FontSettings &f2) { return f1.equals(f2); }
    friend bool operator!=(const FontSettings &f1, const FontSettings &f2) { return !f1.equals(f2); }

private:
    void addMixinStyle(QTextCharFormat &textCharFormat, const MixinTextStyles &mixinStyles) const;
    void clearCaches();

private:
    QString m_family;
    QString m_schemeFileName;
    int m_fontSize;
    int m_fontZoom;
    int m_lineSpacing;
    bool m_antialias;
    ColorScheme m_scheme;
    mutable QHash<TextStyle, QTextCharFormat> m_formatCache;
    mutable QHash<TextStyles, QTextCharFormat> m_textCharFormatCache;
    mutable qreal m_lineSpacingCache;
};

} // namespace TextEditor
