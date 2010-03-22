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

#ifndef FONTSETTINGS_H
#define FONTSETTINGS_H

#include "texteditor_global.h"

#include "colorscheme.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QVector>
#include <QtGui/QFont>

QT_BEGIN_NAMESPACE
class QTextCharFormat;
class QSettings;

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

    QVector<QTextCharFormat> toTextCharFormats(const QVector<QString> &categories) const;
    QTextCharFormat toTextCharFormat(const QString &category) const;

    QString family() const;
    void setFamily(const QString &family);

    int fontSize() const;
    void setFontSize(int size);

    int fontZoom() const;
    void setFontZoom(int zoom);

    QFont font() const
    { return QFont(family(), fontSize()); }

    bool antialias() const;
    void setAntialias(bool antialias);

    Format &formatFor(const QString &category);

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
};

inline bool operator==(const FontSettings &f1, const FontSettings &f2) { return f1.equals(f2); }
inline bool operator!=(const FontSettings &f1, const FontSettings &f2) { return !f1.equals(f2); }

} // namespace TextEditor

#endif // FONTSETTINGS_H
