// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QFileInfo>

namespace QmakeProjectManager {
namespace Internal {

/* Helper struct specifying how to generate file names
 * from class names according to the CppEditor settings. */

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
