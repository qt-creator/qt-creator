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

#ifndef FILENAMINGPARAMETERS_H
#define FILENAMINGPARAMETERS_H

#include <QtCore/QString>
#include <QtCore/QFileInfo>

namespace Qt4ProjectManager {
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
