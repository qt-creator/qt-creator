// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "suiteconf.h"

#include "squishsettings.h"

#include <coreplugin/documentmanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Squish {
namespace Internal {

const char squishLanguageKey[] = "LANGUAGE";
const char squishTestCasesKey[] = "TEST_CASES";
const char squishAutKey[] = "AUT";
const char objectsMapKey[] = "OBJECTMAP";
const char objectMapStyleKey[] = "OBJECTMAPSTYLE";

// splits an input string into chunks separated by ws, but keeps quoted items without splitting
// them (quotes get removed inside the resulting list)
static QStringList parseHelper(const QStringView input)
{
    if (input.isEmpty())
        return {};

    QStringList result;
    QString chunk;

    auto appendChunk = [&] {
        if (!chunk.isEmpty())
            result.append(chunk);
        chunk.clear();
    };

    bool inQuote = false;
    for (const QChar &inChar : input) {
        switch (inChar.toLatin1()) {
        case '"':
            appendChunk();
            inQuote = !inQuote;
            break;
        case ' ':
            if (!inQuote) {
                appendChunk();
                break;
            }
            Q_FALLTHROUGH();
        default:
            chunk.append(inChar);
        }
    }
    appendChunk();
    return result;
}

static QString quoteIfNeeded(const QString &input)
{
    if (input.contains(' '))
        return QString('"' + input + '"');
    return input;
}

// joins items, separating them by single ws and quoting items if needed
static QString joinItems(const QStringList &items)
{
    QStringList result;
    for (const QString &current : items)
        result.append(quoteIfNeeded(current));
    return result.join(' ');
}

static QMap<QString, QString> readSuiteConfContent(const Utils::FilePath &file)
{
    if (!file.isReadableFile())
        return {};

    const Utils::expected_str<QByteArray> suiteConfContent = file.fileContents();
    if (!suiteConfContent)
        return {};

    QMap<QString, QString> suiteConf;
    int invalidCounter = 0;
    static const QRegularExpression validLine("^(?<key>[A-Z_]+)=(?<value>.*)$");
    for (const QByteArray &line : suiteConfContent->split('\n')) {
        const QString utf8Line = QString::fromUtf8(line.trimmed());
        if (utf8Line.isEmpty()) // skip empty lines
            continue;
        const QRegularExpressionMatch match = validLine.match(utf8Line);
        if (match.hasMatch())
            suiteConf.insert(match.captured("key"), match.captured("value"));
        else // save invalid lines
            suiteConf.insert(QString::number(++invalidCounter), utf8Line);
    }
    return suiteConf;
}

static bool writeSuiteConfContent(const Utils::FilePath &file, const QMap<QString, QString> &data)
{
    auto isNumber = [](const QString &str) {
        return !str.isEmpty() && Utils::allOf(str, &QChar::isDigit);
    };
    QByteArray outData;
    for (auto it = data.begin(), end = data.end(); it != end; ++it) {
        if (isNumber(it.key())) // an invalid line we just write out as we got it
            outData.append(it.value().toUtf8()).append('\n');
        else
            outData.append(it.key().toUtf8()).append('=').append(it.value().toUtf8()).append('\n');
    }
    const Utils::expected_str<qint64> result = file.writeFileContents(outData);
    QTC_ASSERT_EXPECTED(result, return false);
    return true;
}

bool SuiteConf::read()
{
    const QMap<QString, QString> suiteConf = readSuiteConfContent(m_filePath);

    // TODO get all information - actually only the information needed now is fetched
    const QStringList parsedAUT = parseHelper(suiteConf.value(squishAutKey));
    if (parsedAUT.isEmpty()) {
        m_aut.clear();
        m_arguments.clear();
    } else {
        m_aut = parsedAUT.first();
        if (parsedAUT.size() > 1)
            m_arguments = joinItems(parsedAUT.mid(1));
        else
            m_arguments.clear();
    }

    setLanguage(suiteConf.value(squishLanguageKey));
    m_testcases = suiteConf.value(squishTestCasesKey);
    m_objectMap = suiteConf.value(objectsMapKey);
    m_objectMapStyle = suiteConf.value(objectMapStyleKey);
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

    // we need the original suite.conf content to handle invalid content "correctly"
    QMap<QString, QString> suiteConf = readSuiteConfContent(m_filePath);
    if (m_arguments.isEmpty())
        suiteConf.insert(squishAutKey, quoteIfNeeded(m_aut));
    else if (QTC_GUARD(!m_aut.isEmpty()))
        suiteConf.insert(squishAutKey, QString(quoteIfNeeded(m_aut) + ' ' + m_arguments));

    suiteConf.insert(squishLanguageKey, languageEntry(m_language));
    suiteConf.insert(objectsMapKey, m_objectMap);
    if (!m_objectMap.isEmpty())
        suiteConf.insert(objectMapStyleKey, m_objectMapStyle);
    suiteConf.insert(squishTestCasesKey, m_testcases);

    return writeSuiteConfContent(m_filePath, suiteConf);
}

QString SuiteConf::suiteName() const
{
    if (!m_filePath.exists())
        return {};
    return m_filePath.parentDir().fileName();
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

    return suiteDir.resolvePath(m_objectMap.isEmpty() ? QString{"objects.map"} : m_objectMap);
}

QString SuiteConf::scriptExtension() const
{
    return '.' + langParameter(); // for now okay
}

QStringList SuiteConf::testCases() const
{
    return parseHelper(m_testcases);
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
    m_testcases = joinItems(current);
}

void SuiteConf::removeTestCase(const QString &name)
{
    QStringList current = testCases();
    int position = current.indexOf(name);
    if (position == -1) // it had been an unlisted test case
        return;

    current.remove(position);
    m_testcases = joinItems(current);
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
                validCases.append(testCaseTest.toString());
            }
        }

        // now unlisted matching tests (suite.conf's TEST_CASES is used for some ordering)
        const Utils::FilePaths entries = subDir.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const Utils::FilePath &entry : entries) {
            if (!entry.fileName().startsWith("tst_"))
                continue;
            const QString testFileStr = entry.pathAppended("test" + extension).toString();
            if (!validCases.contains(testFileStr))
                validCases.append(testFileStr);
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

bool SuiteConf::ensureObjectMapExists() const
{
    if (m_objectMapStyle != "script") {
        const Utils::FilePath objectMap = objectMapPath();
        return objectMap.parentDir().ensureWritableDir() && objectMap.ensureExistingFile();
    }

    const Utils::FilePath scripts = settings().scriptsPath(language());
    QTC_ASSERT(scripts.exists(), return false);

    const QString extension = scriptExtension();
    const Utils::FilePath destinationObjectMap = m_filePath.parentDir()
            .pathAppended("shared/scripts/names" + extension);
    if (destinationObjectMap.exists()) // do not overwrite existing
        return true;

    const Utils::FilePath objectMap = scripts.pathAppended("objectmap_template" + extension);
    Utils::expected_str<void> result = destinationObjectMap.parentDir().ensureWritableDir();
    QTC_ASSERT_EXPECTED(result, return false);
    result = objectMap.copyFile(destinationObjectMap);
    QTC_ASSERT_EXPECTED(result, return false);
    return true;
}

} // namespace Internal
} // namespace Squish
