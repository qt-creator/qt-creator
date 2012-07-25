/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef IOUTILS_H
#define IOUTILS_H

#include <QString>

QT_BEGIN_NAMESPACE

namespace ProFileEvaluatorInternal {

/*!
  This class provides replacement functionality for QFileInfo, QFile & QDir,
  as these are abysmally slow.
*/
class IoUtils {
public:
    enum FileType {
        FileNotFound = 0,
        FileIsRegular = 1,
        FileIsDir = 2
    };

    static FileType fileType(const QString &fileName);
    static bool exists(const QString &fileName) { return fileType(fileName) != FileNotFound; }
    static bool isRelativePath(const QString &fileName);
    static bool isAbsolutePath(const QString &fileName) { return !isRelativePath(fileName); }
    static QStringRef fileName(const QString &fileName); // Requires normalized path
    static QString resolvePath(const QString &baseDir, const QString &fileName);
    static QString shellQuoteUnix(const QString &arg);
    static QString shellQuoteWin(const QString &arg);
    static QString shellQuote(const QString &arg)
#ifdef Q_OS_UNIX
        { return shellQuoteUnix(arg); }
#else
        { return shellQuoteWin(arg); }
#endif
};

} // namespace ProFileEvaluatorInternal

QT_END_NAMESPACE

#endif // IOUTILS_H
