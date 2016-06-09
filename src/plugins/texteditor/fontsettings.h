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
    typedef std::vector<FormatDescription> FormatDescriptions;

    FontSettings();
    void clear();
    inline bool isEmpty() const { return m_scheme.isEmpty(); }

    void toSettings(const QString &category,
                    QSettings *s) const;

    bool fromSettings(const QString &category,
                      const FormatDescriptions &descriptions,
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

private:
    void addMixinStyle(QTextCharFormat &textCharFormat, const MixinTextStyles &mixinStyles) const;

private:
    QString m_family;
    QString m_schemeFileName;
    int m_fontSize;
    int m_fontZoom;
    bool m_antialias;
    ColorScheme m_scheme;
    mutable QHash<TextStyle, QTextCharFormat> m_formatCache;
    mutable QHash<TextStyles, QTextCharFormat> m_textCharFormatCache;
};

inline bool operator==(const FontSettings &f1, const FontSettings &f2) { return f1.equals(f2); }
inline bool operator!=(const FontSettings &f1, const FontSettings &f2) { return !f1.equals(f2); }

} // namespace TextEditor
