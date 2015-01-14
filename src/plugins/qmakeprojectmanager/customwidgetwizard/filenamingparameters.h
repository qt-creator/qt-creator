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

#ifndef FILENAMINGPARAMETERS_H
#define FILENAMINGPARAMETERS_H

#include <QString>
#include <QFileInfo>

namespace QmakeProjectManager {
namespace Internal {

/* Helper struct specifying how to generate file names
 * from class names according to the CppTools settings. */

struct FileNamingParameters
{
    FileNamingParameters(const QString &headerSuffixIn = QString(QLatin1Char('h')),
                         const QString &sourceSuffixIn = QLatin1String("cpp"),
                         bool lowerCaseIn = true) :
        headerSuffix(headerSuffixIn),
        sourceSuffix(sourceSuffixIn),
        lowerCase(lowerCaseIn) {}

    inline QString sourceFileName(const QString &className) const {
        QString rc = lowerCase ? className.toLower() : className;
        rc += QLatin1Char('.');
        rc += sourceSuffix;
        return rc;
    }

    inline QString headerFileName(const QString &className) const {
        QString rc = lowerCase ? className.toLower() : className;
        rc += QLatin1Char('.');
        rc += headerSuffix;
        return rc;
    }

    inline QString sourceToHeaderFileName(const QString &source) const {
        QString rc = QFileInfo(source).completeBaseName();
        rc += QLatin1Char('.');
        rc += headerSuffix;
        return rc;
    }

    inline QString headerToSourceFileName(const QString &header) const {
        QString rc = QFileInfo(header).completeBaseName();
        rc += QLatin1Char('.');
        rc += sourceSuffix;
        return rc;
    }

    QString headerSuffix;
    QString sourceSuffix;
    bool lowerCase;
};

}
}
#endif // FILENAMINGPARAMETERS_H
