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

#pragma once

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
