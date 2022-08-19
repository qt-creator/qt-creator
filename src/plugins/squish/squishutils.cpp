// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "squishutils.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QString>

namespace Squish {
namespace Internal {

const char squishLanguageKey[] = "LANGUAGE";
const char squishTestCasesKey[] = "TEST_CASES";
const char objectsMapKey[] = "OBJECTMAP";
const char objectMapStyleKey[] = "OBJECTMAPSTYLE";

QStringList SquishUtils::validTestCases(const QString &baseDirectory)
{
    QStringList validCases;
    QDir subDir(baseDirectory);
    QFileInfo suiteConf(subDir, "suite.conf");
    if (suiteConf.exists()) {
        QVariantMap conf = readSuiteConf(suiteConf.absoluteFilePath());
        QString extension = extensionForLanguage(conf.value(squishLanguageKey).toString());
        QStringList cases = conf.value(squishTestCasesKey)
                                .toString()
                                .split(QRegularExpression("\\s+"));

        for (const QString &testCase : qAsConst(cases)) {
            QFileInfo testCaseDirInfo(subDir, testCase);
            if (testCaseDirInfo.isDir()) {
                QFileInfo testCaseTestInfo(testCaseDirInfo.filePath(), "test" + extension);
                if (testCaseTestInfo.isFile())
                    validCases.append(testCaseTestInfo.absoluteFilePath());
            }
        }
    }

    return validCases;
}

QVariantMap SquishUtils::readSuiteConf(const QString &suiteConfPath)
{
    const QSettings suiteConf(suiteConfPath, QSettings::IniFormat);
    QVariantMap result;
    // TODO get all information - actually only the information needed now is fetched
    result.insert(squishLanguageKey, suiteConf.value(squishLanguageKey));
    result.insert(squishTestCasesKey, suiteConf.value(squishTestCasesKey));
    return result;
}

QString SquishUtils::objectsMapPath(const QString &suitePath)
{
    const QString suiteDir = QFileInfo(suitePath).absolutePath();
    const QSettings suiteConf(suitePath, QSettings::IniFormat);

    const QString style = suiteConf.value(objectMapStyleKey).toString();
    if (style == "script") {
        const QString language = suiteConf.value(squishLanguageKey).toString();
        return QFileInfo(suiteDir, "shared/scripts/names" + extensionForLanguage(language))
                .canonicalFilePath();
    }

    const QString objMapPath = suiteConf.value(objectsMapKey, "objects.map").toString();
    return QFileInfo(suiteDir, objMapPath).canonicalFilePath();
}

QString SquishUtils::extensionForLanguage(const QString &language)
{
    if (language == "Python")
        return ".py";
    if (language == "Perl")
        return ".pl";
    if (language == "JavaScript")
        return ".js";
    if (language == "Ruby")
        return ".rb";
    if (language == "Tcl")
        return ".tcl";
    return QString(); // better return an invalid extension?
}

} // namespace Internal
} // namespace Squish
