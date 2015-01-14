/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FONTSETTINGS_H
#define FONTSETTINGS_H

#include "texteditor_global.h"

#include "colorscheme.h"

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
    typedef QList<FormatDescription> FormatDescriptions;

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
    QString m_family;
    QString m_schemeFileName;
    int m_fontSize;
    int m_fontZoom;
    bool m_antialias;
    ColorScheme m_scheme;
    mutable QHash<TextStyle, QTextCharFormat> m_formatCache;
};

inline bool operator==(const FontSettings &f1, const FontSettings &f2) { return f1.equals(f2); }
inline bool operator!=(const FontSettings &f1, const FontSettings &f2) { return !f1.equals(f2); }

} // namespace TextEditor

#endif // FONTSETTINGS_H
