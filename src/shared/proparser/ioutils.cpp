/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
