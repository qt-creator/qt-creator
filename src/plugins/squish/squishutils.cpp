/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishutils.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QString>

namespace Squish {
namespace Internal {

const static char squishLanguageKey[] = "LANGUAGE";
const static char squishTestCasesKey[] = "TEST_CASES";
const static char objectsMapKey[] = "OBJECTMAP";

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
    const QString objMapPath = suiteConf.value(objectsMapKey).toString();
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
