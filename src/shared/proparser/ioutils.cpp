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

#include "ioutils.h"

#include <QDir>
#include <QFile>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

using namespace ProFileEvaluatorInternal;

IoUtils::FileType IoUtils::fileType(const QString &fileName)
{
    Q_ASSERT(fileName.isEmpty() || isAbsolutePath(fileName));
#ifdef Q_OS_WIN
    DWORD attr = GetFileAttributesW((WCHAR*)fileName.constData());
    if (attr == INVALID_FILE_ATTRIBUTES)
        return FileNotFound;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? FileIsDir : FileIsRegular;
#else
    struct ::stat st;
    if (::stat(fileName.toLatin1().constData(), &st)) // latin1 symmetric to the file reader
        return FileNotFound;
    return S_ISDIR(st.st_mode) ? FileIsDir : FileIsRegular;
#endif
}

bool IoUtils::isRelativePath(const QString &path)
{
    if (path.startsWith(QLatin1Char('/')))
        return false;
#ifdef Q_OS_WIN
    if (path.startsWith(QLatin1Char('\\')))
        return false;
    // Unlike QFileInfo, this won't accept a relative path with a drive letter.
    // Such paths result in a royal mess anyway ...
    if (path.length() >= 3 && path.at(1) == QLatin1Char(':') && path.at(0).isLetter()
        && (path.at(2) == QLatin1Char('/') || path.at(2) == QLatin1Char('\\')))
        return false;
#endif
    return true;
}

QStringRef IoUtils::fileName(const QString &fileName)
{
    return fileName.midRef(fileName.lastIndexOf(QLatin1Char('/')) + 1);
}

QString IoUtils::resolvePath(const QString &baseDir, const QString &fileName)
{
    if (fileName.isEmpty())
        return QString();
    if (isAbsolutePath(fileName))
        return QDir::cleanPath(fileName);
    return QDir::cleanPath(baseDir + QLatin1Char('/') + fileName);
}

#ifdef Q_OS_WIN

// FIXME: Without this, quoting is not foolproof. But it needs support in the process setup, etc.
//#define PERCENT_ESCAPE QLatin1String("%PERCENT_SIGN%")

static QString quoteArgInternal(const QString &arg)
{
    // Escape quotes, preceding backslashes are doubled. Surround with quotes.
    // Note that cmd does not understand quote escapes in quoted strings,
    // so the quoting needs to be "suspended".
    const QLatin1Char bs('\\'), dq('"');
    QString ret;
    bool inquote = false;
    int bslashes = 0;
    for (int p = 0; p < arg.length(); p++) {
        if (arg[p] == bs) {
            bslashes++;
        } else if (arg[p] == dq) {
            if (inquote) {
                ret.append(dq);
                inquote = false;
            }
            for (; bslashes; bslashes--)
                ret.append(QLatin1String("\\\\"));
            ret.append(QLatin1String("\\^\""));
        } else {
            if (!inquote) {
                ret.append(dq);
                inquote = true;
            }
            for (; bslashes; bslashes--)
                ret.append(bs);
            ret.append(arg[p]);
        }
    }
    //ret.replace(QLatin1Char('%'), PERCENT_ESCAPE);
    if (bslashes) {
        // Ensure that we don't have directly trailing backslashes,
        // so concatenating with another string won't cause surprises.
        if (!inquote)
            ret.append(dq);
        for (; bslashes; bslashes--)
            ret.append(QLatin1String("\\\\"));
        ret.append(dq);
    } else if (inquote) {
        ret.append(dq);
    }
    return ret;
}

inline static bool isSpecialChar(ushort c)
{
    // Chars that should be quoted (TM). This includes:
    // - control chars & space
    // - the shell meta chars &()<>^|
    // - the potential separators ,;=
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0x41, 0x13, 0x00, 0x78,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10
    };

    return (c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7)));
}

QString IoUtils::shellQuote(const QString &arg)
{
    if (arg.isEmpty())
        return QString::fromLatin1("\"\"");

    // Ensure that we don't have directly trailing backslashes,
    // so concatenating with another string won't cause surprises.
    if (arg.endsWith(QLatin1Char('\\')))
        return quoteArgInternal(arg);

    for (int x = arg.length() - 1; x >= 0; --x)
        if (isSpecialChar(arg[x].unicode()))
            return quoteArgInternal(arg);

    // Escape quotes. Preceding backslashes are doubled.
    // Note that the remaining string is not quoted.
    QString ret(arg);
    ret.replace(QRegExp(QLatin1String("(\\\\*)\"")), QLatin1String("\\1\\1\\^\""));
    //ret.replace('%', PERCENT_ESCAPE);
    return ret;
}

#else // Q_OS_WIN

inline static bool isSpecial(const QChar &cUnicode)
{
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0xdf, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x78
    }; // 0-32 \'"$`<>|;&(){}*?#!~[]

    uint c = cUnicode.unicode();
    return (c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7)));
}

QString IoUtils::shellQuote(const QString &arg)
{
    if (!arg.length())
        return QString::fromLatin1("''");
    for (int i = 0; i < arg.length(); i++)
        if (isSpecial(arg.unicode()[i])) {
            const QLatin1Char q('\'');
            return q + QString(arg).replace(q, QLatin1String("'\\''")) + q;
        }
    return arg;
}

#endif // Q_OS_WIN
