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

#ifndef IOUTILS_H
#define IOUTILS_H

#include <QString>

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
};

}

#endif // IOUTILS_H
