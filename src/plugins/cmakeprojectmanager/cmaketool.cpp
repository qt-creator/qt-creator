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

#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QTextDocument>
#include <QUuid>
#include <QVariantMap>

namespace CMakeProjectManager {

const char CMAKE_INFORMATION_ID[] = "Id";
const char CMAKE_INFORMATION_COMMAND[] = "Binary";
const char CMAKE_INFORMATION_DISPLAYNAME[] = "DisplayName";
const char CMAKE_INFORMATION_AUTORUN[] = "AutoRun";
const char CMAKE_INFORMATION_AUTODETECTED[] = "AutoDetected";


bool CMakeTool::Generator::matches(const QString &n, const QString &ex) const
{
    return n == name && (ex.isEmpty() || extraGenerators.contains(ex));
}

///////////////////////////
// CMakeTool
///////////////////////////
CMakeTool::CMakeTool(Detection d, const Core::Id &id) :
    m_id(id), m_isAutoDetected(d == AutoDetection)
{
    QTC_ASSERT(m_id.isValid(), m_id = Core::Id::fromString(QUuid::createUuid().toString()));
}

CMakeTool::CMakeTool(const QVariantMap &map, bool fromSdk) : m_isAutoDetected(fromSdk)
{
    m_id = Core::Id::fromSetting(map.value(CMAKE_INFORMATION_ID));
    m_displayName = map.value(CMAKE_INFORMATION_DISPLAYNAME).toString();
    m_isAutoRun = map.value(CMAKE_INFORMATION_AUTORUN, true).toBool();

    //loading a CMakeTool from SDK is always autodetection
    if (!fromSdk)
        m_isAutoDetected = map.value(CMAKE_INFORMATION_AUTODETECTED, false).toBool();

    setCMakeExecutable(Utils::FileName::fromString(map.value(CMAKE_INFORMATION_COMMAND).toString()));
}

Core::Id CMakeTool::createId()
{
    return Core::Id::fromString(QUuid::createUuid().toString());
}

void CMakeTool::setCMakeExecutable(const Utils::FileName &executable)
{
    if (m_executable == executable)
        return;

    m_didRun = false;
    m_didAttemptToRun = false;

    m_executable = executable;
    CMakeToolManager::notifyAboutUpdate(this);
}

void CMakeTool::setAutorun(bool autoRun)
{
    if (m_isAutoRun == autoRun)
        return;

    m_isAutoRun = autoRun;
    CMakeToolManager::notifyAboutUpdate(this);
}

bool CMakeTool::isValid() const
{
    if (!m_id.isValid())
        return false;

    if (!m_didAttemptToRun)
        supportedGenerators();

    return m_didRun;
}

Utils::SynchronousProcessResponse CMakeTool::run(const QStringList &args, bool mayFail) const
{
    if (m_didAttemptToRun && !m_didRun) {
        Utils::SynchronousProcessResponse response;
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    Utils::SynchronousProcess cmake;
    cmake.setTimeoutS(1);
    cmake.setFlags(Utils::SynchronousProcess::UnixTerminalDisabled);
    Utils::Environment env = Utils::Environment::systemEnvironment();
    Utils::Environment::setupEnglishOutput(&env);
    cmake.setProcessEnvironment(env.toProcessEnvironment());
    cmake.setTimeOutMessageBoxEnabled(false);

    Utils::SynchronousProcessResponse response = cmake.runBlocking(m_executable.toString(), args);
    m_didAttemptToRun = true;
    m_didRun = mayFail ? true : (response.result == Utils::SynchronousProcessResponse::Finished);
    return response;
}

QVariantMap CMakeTool::toMap() const
{
    QVariantMap data;
    data.insert(CMAKE_INFORMATION_DISPLAYNAME, m_displayName);
    data.insert(CMAKE_INFORMATION_ID, m_id.toSetting());
    data.insert(CMAKE_INFORMATION_COMMAND, m_executable.toString());
    data.insert(CMAKE_INFORMATION_AUTORUN, m_isAutoRun);
    data.insert(CMAKE_INFORMATION_AUTODETECTED, m_isAutoDetected);
    return data;
}

Utils::FileName CMakeTool::cmakeExecutable() const
{
    if (Utils::HostOsInfo::isMacHost() && m_executable.endsWith(".app")) {
        Utils::FileName toTest = m_executable;
        toTest = toTest.appendPath("Contents/bin/cmake");
        if (toTest.exists())
            return toTest;
    }
    return m_executable;
}

bool CMakeTool::isAutoRun() const
{
    return m_isAutoRun;
}

QList<CMakeTool::Generator> CMakeTool::supportedGenerators() const
{
    readInformation(QueryType::GENERATORS);
    return m_generators;
}

TextEditor::Keywords CMakeTool::keywords()
{
    if (m_functions.isEmpty()) {
        Utils::SynchronousProcessResponse response;
        response = run({"--help-command-list"});
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            m_functions = response.stdOut().split('\n');

        response = run({"--help-commands"});
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            parseFunctionDetailsOutput(response.stdOut());

        response = run({"--help-property-list"});
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            m_variables = parseVariableOutput(response.stdOut());

        response = run({"--help-variable-list"});
        if (response.result == Utils::SynchronousProcessResponse::Finished) {
            m_variables.append(parseVariableOutput(response.stdOut()));
            m_variables = Utils::filteredUnique(m_variables);
            Utils::sort(m_variables);
        }
    }

    return TextEditor::Keywords(m_variables, m_functions, m_functionArgs);
}

bool CMakeTool::hasServerMode() const
{
    readInformation(QueryType::SERVER_MODE);
    return m_hasServerMode;
}

CMakeTool::Version CMakeTool::version() const
{
    readInformation(QueryType::VERSION);
    return m_version;
}

bool CMakeTool::isAutoDetected() const
{
    return m_isAutoDetected;
}

QString CMakeTool::displayName() const
{
    return m_displayName;
}

void CMakeTool::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
    CMakeToolManager::notifyAboutUpdate(this);
}

void CMakeTool::setPathMapper(const CMakeTool::PathMapper &pathMapper)
{
    m_pathMapper = pathMapper;
}

CMakeTool::PathMapper CMakeTool::pathMapper() const
{
    if (m_pathMapper)
        return m_pathMapper;
    return [](const Utils::FileName &fn) { return fn; };
}

void CMakeTool::readInformation(CMakeTool::QueryType type) const
{
    if ((type == QueryType::GENERATORS && !m_generators.isEmpty())
         || (type == QueryType::SERVER_MODE && m_queriedServerMode)
         || (type == QueryType::VERSION && !m_version.fullVersion.isEmpty()))
        return;

    if (!m_triedCapabilities) {
        fetchFromCapabilities();
        m_triedCapabilities = true;
        m_queriedServerMode = true; // Got added after "-E capabilities" support!
        if (type == QueryType::GENERATORS && !m_generators.isEmpty())
            return;
    }

    if (type == QueryType::GENERATORS) {
        fetchGeneratorsFromHelp();
    } else if (type == QueryType::SERVER_MODE) {
        // Nothing to do...
    } else if (type == QueryType::VERSION) {
        fetchVersionFromVersionOutput();
    } else {
        QTC_ASSERT(false, return);
    }
}

static QStringList parseDefinition(const QString &definition)
{
    QStringList result;
    QString word;
    bool ignoreWord = false;
    QVector<QChar> braceStack;

    foreach (const QChar &c, definition) {
        if (c == '[' || c == '<' || c == '(') {
            braceStack.append(c);
            ignoreWord = false;
        } else if (c == ']' || c == '>' || c == ')') {
            if (braceStack.isEmpty() || braceStack.takeLast() == '<')
                ignoreWord = true;
        }

        if (c == ' ' || c == '[' || c == '<' || c == '('
                || c == ']' || c == '>' || c == ')') {
            if (!ignoreWord && !word.isEmpty()) {
                if (result.isEmpty() || Utils::allOf(word, [](const QChar &c) { return c.isUpper() || c == '_'; }))
                    result.append(word);
            }
            word.clear();
            ignoreWord = false;
        } else {
            word.append(c);
        }
    }
    return result;
}

void CMakeTool::parseFunctionDetailsOutput(const QString &output)
{
    QSet<QString> functionSet;
    functionSet.fromList(m_functions);

    bool expectDefinition = false;
    QString currentDefinition;

    const QStringList lines = output.split('\n');
    for (int i = 0; i < lines.count(); ++i) {
        const QString line = lines.at(i);

        if (line == "::") {
            expectDefinition = true;
            continue;
        }

        if (expectDefinition) {
            if (!line.startsWith(' ') && !line.isEmpty()) {
                expectDefinition = false;
                QStringList words = parseDefinition(currentDefinition);
                if (!words.isEmpty()) {
                    const QString command = words.takeFirst();
                    if (functionSet.contains(command)) {
                        QStringList tmp = words + m_functionArgs[command];
                        Utils::sort(tmp);
                        m_functionArgs[command] = Utils::filteredUnique(tmp);
                    }
                }
                if (!words.isEmpty() && functionSet.contains(words.at(0)))
                    m_functionArgs[words.at(0)];
                currentDefinition.clear();
            } else {
                currentDefinition.append(line.trimmed() + ' ');
            }
        }
    }
}

QStringList CMakeTool::parseVariableOutput(const QString &output)
{
    const QStringList variableList = output.split('\n');
    QStringList result;
    foreach (const QString &v, variableList) {
        if (v.startsWith("CMAKE_COMPILER_IS_GNU<LANG>")) { // This key takes a compiler name :-/
            result << "CMAKE_COMPILER_IS_GNUCC" << "CMAKE_COMPILER_IS_GNUCXX";
        } else if (v.contains("<CONFIG>")) {
            const QString tmp = QString(v).replace("<CONFIG>", "%1");
            result << tmp.arg("DEBUG") << tmp.arg("RELEASE")
                   << tmp.arg("MINSIZEREL") << tmp.arg("RELWITHDEBINFO");
        } else if (v.contains("<LANG>")) {
            const QString tmp = QString(v).replace("<LANG>", "%1");
            result << tmp.arg("C") << tmp.arg("CXX");
        } else if (!v.contains('<') && !v.contains('[')) {
            result << v;
        }
    }
    return result;
}

void CMakeTool::fetchGeneratorsFromHelp() const
{
    Utils::SynchronousProcessResponse response = run({"--help"});
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        return;

    bool inGeneratorSection = false;
    QHash<QString, QStringList> generatorInfo;
    const QStringList lines = response.stdOut().split('\n');
    foreach (const QString &line, lines) {
        if (line.isEmpty())
            continue;
        if (line == "Generators") {
            inGeneratorSection = true;
            continue;
        }
        if (!inGeneratorSection)
            continue;

        if (line.startsWith("  ") && line.at(3) != ' ') {
            int pos = line.indexOf('=');
            if (pos < 0)
                pos = line.length();
            if (pos >= 0) {
                --pos;
                while (pos > 2 && line.at(pos).isSpace())
                    --pos;
            }
            if (pos > 2) {
                const QString fullName = line.mid(2, pos - 1);
                const int dashPos = fullName.indexOf(" - ");
                QString generator;
                QString extra;
                if (dashPos < 0) {
                    generator = fullName;
                } else {
                    extra = fullName.mid(0, dashPos);
                    generator = fullName.mid(dashPos + 3);
                }
                QStringList value = generatorInfo.value(generator);
                if (!extra.isEmpty())
                    value.append(extra);
                generatorInfo.insert(generator, value);
            }
        }
    }

    // Populate genertor list:
    for (auto it = generatorInfo.constBegin(); it != generatorInfo.constEnd(); ++it)
        m_generators.append(Generator(it.key(), it.value()));
}

void CMakeTool::fetchVersionFromVersionOutput() const
{
    Utils::SynchronousProcessResponse response = run({"--version" });
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        return;

    QRegularExpression versionLine("^cmake version ((\\d+).(\\d+).(\\d+).*)$");
    const QString responseText = response.stdOut();
    for (const QStringRef &line : responseText.splitRef(QLatin1Char('\n'))) {
        QRegularExpressionMatch match = versionLine.match(line);
        if (!match.hasMatch())
            continue;

        m_version.major = match.captured(2).toInt();
        m_version.minor = match.captured(3).toInt();
        m_version.patch = match.captured(4).toInt();
        m_version.fullVersion = match.captured(1).toUtf8();
        break;
    }
}

void CMakeTool::fetchFromCapabilities() const
{
    Utils::SynchronousProcessResponse response = run({"-E", "capabilities" }, true);
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        return;

    auto doc = QJsonDocument::fromJson(response.stdOut().toUtf8());
    if (!doc.isObject())
        return;

    const QVariantMap data = doc.object().toVariantMap();
    m_hasServerMode = data.value("serverMode").toBool();
    const QVariantList generatorList = data.value("generators").toList();
    for (const QVariant &v : generatorList) {
        const QVariantMap gen = v.toMap();
        m_generators.append(Generator(gen.value("name").toString(),
                                      gen.value("extraGenerators").toStringList(),
                                      gen.value("platformSupport").toBool(),
                                      gen.value("toolsetSupport").toBool()));
    }

    const QVariantMap versionInfo = data.value("version").toMap();
    m_version.major = versionInfo.value("major").toInt();
    m_version.minor = versionInfo.value("minor").toInt();
    m_version.patch = versionInfo.value("patch").toInt();
    m_version.fullVersion = versionInfo.value("string").toByteArray();
}

} // namespace CMakeProjectManager
