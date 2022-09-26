// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "suiteconf.h"

#include <coreplugin/documentmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>
#include <QSettings>

namespace Squish {
namespace Internal {

const char squishLanguageKey[] = "LANGUAGE";
const char squishTestCasesKey[] = "TEST_CASES";
const char squishAutKey[] = "AUT";
const char objectsMapKey[] = "OBJECTMAP";
const char objectMapStyleKey[] = "OBJECTMAPSTYLE";

bool SuiteConf::read()
{
    if (!m_filePath.isReadableFile())
        return false;

    const QSettings suiteConf(m_filePath.toString(), QSettings::IniFormat);
    // TODO get all information - actually only the information needed now is fetched
    m_aut = suiteConf.value(squishAutKey).toString();
    // TODO args are listed in config.xml?
    setLanguage(suiteConf.value(squishLanguageKey).toString());
    m_testcases = suiteConf.value(squishTestCasesKey).toString();
    m_objectMap = suiteConf.value(objectsMapKey).toString();
    m_objectMapStyle = suiteConf.value(objectMapStyleKey).toString();
    return true;
}

static QString languageEntry(Language language)
{
    switch (language) {
    case Language::Python: return "Python";
    case Language::Perl: return "Perl";
    case Language::JavaScript: return "JavaScript";
    case Language::Ruby: return "Ruby";
    case Language::Tcl: return "Tcl";
    }
    return {};
}

bool SuiteConf::write()
{
    Core::DocumentManager::expectFileChange(m_filePath);
    QSettings suiteConf(m_filePath.toString(), QSettings::IniFormat);
    suiteConf.setValue(squishAutKey, m_aut);
    suiteConf.setValue(squishLanguageKey, languageEntry(m_language));
    suiteConf.setValue(objectsMapKey, m_objectMap);
    if (!m_objectMap.isEmpty())
        suiteConf.setValue(objectMapStyleKey, m_objectMapStyle);
    suiteConf.setValue(squishTestCasesKey, m_testcases);
    suiteConf.sync();
    return suiteConf.status() == QSettings::NoError;
}

QString SuiteConf::langParameter() const
{
    switch (m_language) {
    case Language::Python: return "py";
    case Language::Perl: return "pl";
    case Language::JavaScript: return "js";
    case Language::Ruby: return "rb";
    case Language::Tcl: return "tcl";
    }
    return {};
}

Utils::FilePath SuiteConf::objectMapPath() const
{
    const Utils::FilePath suiteDir = m_filePath.parentDir();
    if (m_objectMapStyle == "script")
        return suiteDir.resolvePath("shared/scripts/names" + scriptExtension());

    return suiteDir.resolvePath(m_objectMap);
}

QString SuiteConf::scriptExtension() const
{
    return '.' + langParameter(); // for now okay
}

QStringList SuiteConf::testCases() const
{
    return m_testcases.split(QRegularExpression("\\s+"));
}

QStringList SuiteConf::usedTestCases() const
{
    QStringList result = testCases();

    auto suiteDir = m_filePath.parentDir();
    const Utils::FilePaths entries = Utils::filtered(
                suiteDir.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot),
                [](const Utils::FilePath &fp) {
        return fp.fileName().startsWith("tst_");
    });
    const QStringList testCaseNames = Utils::transform(entries, &Utils::FilePath::fileName);
    for (const QString &testCaseName : testCaseNames) {
        if (result.contains(testCaseName))
            continue;
        result.append(testCaseName); // should this check for test.*?
    }
    return result;
}

void SuiteConf::addTestCase(const QString &name)
{
    QStringList current = testCases();
    int insertAt = 0;
    for (int count = current.count(); insertAt < count; ++insertAt) {
        if (current.at(insertAt) > name)
            break;
    }
    current.insert(insertAt, name);
    m_testcases = current.join(' ');
}

void SuiteConf::setLanguage(const QString &language)
{
    if (language == "Python")
        m_language = Language::Python;
    else if (language == "Perl")
        m_language = Language::Perl;
    else if (language == "JavaScript")
        m_language = Language::JavaScript;
    else if (language == "Ruby")
        m_language = Language::Ruby;
    else if (language == "Tcl")
        m_language = Language::Tcl;
    else
        QTC_ASSERT(false, m_language = Language::JavaScript);
}

QStringList SuiteConf::validTestCases(const QString &baseDirectory)
{
    QStringList validCases;
    const Utils::FilePath subDir = Utils::FilePath::fromString(baseDirectory);
    const Utils::FilePath suiteConf = subDir / "suite.conf";
    if (suiteConf.exists()) {
        const SuiteConf conf = readSuiteConf(suiteConf);
        const QString extension = conf.scriptExtension();
        const QStringList cases = conf.testCases();

        for (const QString &testCase : cases) {
            const Utils::FilePath testCaseDir = subDir / testCase;
            if (testCaseDir.isDir()) {
                Utils::FilePath testCaseTest = testCaseDir.pathAppended("test" + extension);
                if (testCaseTest.isFile())
                    validCases.append(testCaseTest.toString());
            }
        }
    }
    return validCases;
}

SuiteConf SuiteConf::readSuiteConf(const Utils::FilePath &suiteConfPath)
{
    SuiteConf suiteConf(suiteConfPath);
    suiteConf.read();
    return suiteConf;
}

} // namespace Internal
} // namespace Squish
