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

#include "ioutils.h"

#include <qdir.h>
#include <qfile.h>
#include <qregexp.h>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <utime.h>
#  include <fcntl.h>
#  include <errno.h>
#endif

#define fL1S(s) QString::fromLatin1(s)

QT_BEGIN_NAMESPACE

using namespace QMakeInternal;

IoUtils::FileType IoUtils::fileType(const QString &fileName)
{
    Q_ASSERT(fileName.isEmpty() || isAbsolutePath(fileName));
#ifdef Q_OS_WIN
    DWORD attr = GetFileAttributesW((WCHAR*)fileName.utf16());
    if (attr == INVALID_FILE_ATTRIBUTES)
        return FileNotFound;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? FileIsDir : FileIsRegular;
#else
    struct ::stat st;
    if (::stat(fileName.toLocal8Bit().constData(), &st))
        return FileNotFound;
    return S_ISDIR(st.st_mode) ? FileIsDir : S_ISREG(st.st_mode) ? FileIsRegular : FileNotFound;
#endif
}

bool IoUtils::isRelativePath(const QString &path)
{
#ifdef QMAKE_BUILTIN_PRFS
    if (path.startsWith(QLatin1String(":/")))
        return false;
#endif
#ifdef Q_OS_WIN
    // Unlike QFileInfo, this considers only paths with both a drive prefix and
    // a subsequent (back-)slash absolute:
    if (path.length() >= 3 && path.at(1) == QLatin1Char(':') && path.at(0).isLetter()
        && (path.at(2) == QLatin1Char('/') || path.at(2) == QLatin1Char('\\'))) {
        return false;
    }
    // (... unless, of course, they're UNC, which qmake fails on anyway)
#else
    if (path.startsWith(QLatin1Char('/')))
        return false;
#endif // Q_OS_WIN
    return true;
}

QStringRef IoUtils::pathName(const QString &fileName)
{
    return fileName.leftRef(fileName.lastIndexOf(QLatin1Char('/')) + 1);
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
#ifdef Q_OS_WIN // Add drive to otherwise-absolute path:
    if (fileName.at(0).unicode() == '/' || fileName.at(0).unicode() == '\\') {
        Q_ASSERT(isAbsolutePath(baseDir));
        return QDir::cleanPath(baseDir.left(2) + fileName);
    }
#endif // Q_OS_WIN
    return QDir::cleanPath(baseDir + QLatin1Char('/') + fileName);
}

inline static
bool isSpecialChar(ushort c, const uchar (&iqm)[16])
{
    if ((c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7))))
        return true;
    return false;
}

inline static
bool hasSpecialChars(const QString &arg, const uchar (&iqm)[16])
{
    for (int x = arg.length() - 1; x >= 0; --x) {
        if (isSpecialChar(arg.unicode()[x].unicode(), iqm))
            return true;
    }
    return false;
}

QString IoUtils::shellQuoteUnix(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0xdf, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x78
    }; // 0-32 \'"$`<>|;&(){}*?#!~[]

    if (!arg.length())
        return QString::fromLatin1("''");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        ret.replace(QLatin1Char('\''), QLatin1String("'\\''"));
        ret.prepend(QLatin1Char('\''));
        ret.append(QLatin1Char('\''));
    }
    return ret;
}

QString IoUtils::shellQuoteWin(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    // - control chars & space
    // - the shell meta chars "&()<>^|
    // - the potential separators ,;=
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0x45, 0x13, 0x00, 0x78,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10
    };
    // Shell meta chars that need escaping.
    static const uchar ism[] = {
        0x00, 0x00, 0x00, 0x00, 0x40, 0x03, 0x00, 0x50,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10
    }; // &()<>^|

    if (!arg.length())
        return QString::fromLatin1("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        // The process-level standard quoting allows escaping quotes with backslashes (note
        // that backslashes don't escape themselves, unless they are followed by a quote).
        // Consequently, quotes are escaped and their preceding backslashes are doubled.
        ret.replace(QRegExp(QLatin1String("(\\\\*)\"")), QLatin1String("\\1\\1\\\""));
        // Trailing backslashes must be doubled as well, as they are followed by a quote.
        ret.replace(QRegExp(QLatin1String("(\\\\+)$")), QLatin1String("\\1\\1"));
        // However, the shell also interprets the command, and no backslash-escaping exists
        // there - a quote always toggles the quoting state, but is nonetheless passed down
        // to the called process verbatim. In the unquoted state, the circumflex escapes
        // meta chars (including itself and quotes), and is removed from the command.
        bool quoted = true;
        for (int i = 0; i < ret.length(); i++) {
            QChar c = ret.unicode()[i];
            if (c.unicode() == '"')
                quoted = !quoted;
            else if (!quoted && isSpecialChar(c.unicode(), ism))
                ret.insert(i++, QLatin1Char('^'));
        }
        if (!quoted)
            ret.append(QLatin1Char('^'));
        ret.append(QLatin1Char('"'));
        ret.prepend(QLatin1Char('"'));
    }
    return ret;
}

#if defined(PROEVALUATOR_FULL)

#  if defined(Q_OS_WIN)
static QString windowsErrorCode()
{
    wchar_t *string = 0;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR)&string,
                  0,
                  NULL);
    QString ret = QString::fromWCharArray(string);
    LocalFree((HLOCAL)string);
    return ret.trimmed();
}
#  endif

bool IoUtils::touchFile(const QString &targetFileName, const QString &referenceFileName, QString *errorString)
{
#  ifdef Q_OS_UNIX
    struct stat st;
    if (stat(referenceFileName.toLocal8Bit().constData(), &st)) {
        *errorString = fL1S("Cannot stat() reference file %1: %2.").arg(referenceFileName, fL1S(strerror(errno)));
        return false;
    }
#    if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L
    const struct timespec times[2] = { { 0, UTIME_NOW }, st.st_mtim };
    const bool utimeError = utimensat(AT_FDCWD, targetFileName.toLocal8Bit().constData(), times, 0) < 0;
#    else
    struct utimbuf utb;
    utb.actime = time(0);
    utb.modtime = st.st_mtime;
    const bool utimeError= utime(targetFileName.toLocal8Bit().constData(), &utb) < 0;
#    endif
    if (utimeError) {
        *errorString = fL1S("Cannot touch %1: %2.").arg(targetFileName, fL1S(strerror(errno)));
        return false;
    }
#  else
    HANDLE rHand = CreateFile((wchar_t*)referenceFileName.utf16(),
                              GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (rHand == INVALID_HANDLE_VALUE) {
        *errorString = fL1S("Cannot open reference file %1: %2").arg(referenceFileName, windowsErrorCode());
        return false;
        }
    FILETIME ft;
    GetFileTime(rHand, 0, 0, &ft);
    CloseHandle(rHand);
    HANDLE wHand = CreateFile((wchar_t*)targetFileName.utf16(),
                              GENERIC_WRITE, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (wHand == INVALID_HANDLE_VALUE) {
        *errorString = fL1S("Cannot open %1: %2").arg(targetFileName, windowsErrorCode());
        return false;
    }
    SetFileTime(wHand, 0, 0, &ft);
    CloseHandle(wHand);
#  endif
    return true;
}

#if defined(QT_BUILD_QMAKE) && defined(Q_OS_UNIX)
bool IoUtils::readLinkTarget(const QString &symlinkPath, QString *target)
{
    const QByteArray localSymlinkPath = QFile::encodeName(symlinkPath);
#  if defined(__GLIBC__) && !defined(PATH_MAX)
#    define PATH_CHUNK_SIZE 256
    char *s = 0;
    int len = -1;
    int size = PATH_CHUNK_SIZE;

    forever {
        s = (char *)::realloc(s, size);
        len = ::readlink(localSymlinkPath.constData(), s, size);
        if (len < 0) {
            ::free(s);
            break;
        }
        if (len < size)
            break;
        size *= 2;
    }
#  else
    char s[PATH_MAX+1];
    int len = readlink(localSymlinkPath.constData(), s, PATH_MAX);
#  endif
    if (len <= 0)
        return false;
    *target = QFile::decodeName(QByteArray(s, len));
#  if defined(__GLIBC__) && !defined(PATH_MAX)
    ::free(s);
#  endif
    return true;
}
#endif

#endif  // PROEVALUATOR_FULL

QT_END_NAMESPACE
