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

#ifndef REUSE_H
#define REUSE_H

#include <Qt>
#include <QtCore/QString>
#include <QtCore/QLatin1String>
#include <QtCore/QChar>
#include <QtGui/QFont>

namespace TextEditor {
namespace Internal {

inline bool toBool(const QString &s)
{
    static const QLatin1String kTrue("true");
    static const QLatin1String k1("1");

    if (s.toLower() == kTrue || s == k1)
        return true;
    return false;
}


inline Qt::CaseSensitivity toCaseSensitivity(const bool sensitive)
{
    if (sensitive)
        return Qt::CaseSensitive;
    return Qt::CaseInsensitive;
}

inline QFont::Weight toFontWeight(const bool bold)
{
    if (bold)
        return QFont::Bold;
    else
        return QFont::Normal;
}

inline bool isOctalDigit(const QChar &c)
{
    static const QLatin1Char k0('0');
    static const QLatin1Char k7('7');

    return c >= k0 && c <= k7;
}

inline bool isHexDigit(const QChar &c)
{
    static const QLatin1Char k0('0');
    static const QLatin1Char k9('9');
    static const QLatin1Char kA('A');
    static const QLatin1Char kF('F');
    static const QLatin1Char ka('a');
    static const QLatin1Char kf('f');

    if ((c >= k0 && c <= k9) || (c >= kA && c <= kF) || (c >= ka && c <= kf))
        return true;

    return false;
}

inline void setStartCharacter(QChar *c, const QString &character)
{
    if (!character.isEmpty())
        *c = character.at(0);
}

} // namespace Internal
} // namespace TextEditor

#endif // REUSE_H
