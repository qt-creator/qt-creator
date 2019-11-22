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

#include <QDir>
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
const char CMAKE_INFORMATION_QCH_FILE_PATH[] = "QchFile";
const char CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY[] = "AutoCreateBuildDirectory";
const char CMAKE_INFORMATION_AUTODETECTED[] = "AutoDetected";
const char CMAKE_INFORMATION_READERTYPE[] = "ReaderType";

bool CMakeTool::Generator::matches(const QString &n, const QString &ex) const
{
    return n == name && (ex.isEmpty() || extraGenerators.contains(ex));
}

namespace Internal {

const char READER_TYPE_TEALEAF[] = "tealeaf";
const char READER_TYPE_SERVERMODE[] = "servermode";
const char READER_TYPE_FILEAPI[] = "fileapi";

static bool ignoreFileApi()
{
    static bool s_ignoreFileApi = qEnvironmentVariableIsSet("QTC_CMAKE_IGNORE_FILEAPI");
    return s_ignoreFileApi;
}

static Utils::optional<CMakeTool::ReaderType> readerTypeFromString(const QString &input)
{
    if (input == READER_TYPE_TEALEAF)
        return CMakeTool::TeaLeaf;
    if (input == READER_TYPE_SERVERMODE)
        return CMakeTool::ServerMode;
    if (input == READER_TYPE_FILEAPI)
        return ignoreFileApi() ? CMakeTool::ServerMode : CMakeTool::FileApi;
    return {};
}

static QString readerTypeToString(const CMakeTool::ReaderType &type)
{
    switch (type) {
    case CMakeTool::TeaLeaf:
        return QString(READER_TYPE_TEALEAF);
    case CMakeTool::ServerMode:
        return QString(READER_TYPE_SERVERMODE);
    case CMakeTool::FileApi:
        return QString(READER_TYPE_FILEAPI);
    }
    return QString();
}

// --------------------------------------------------------------------
// CMakeIntrospectionData:
// --------------------------------------------------------------------

class FileApi {
public:
    QString kind;
    std::pair<int, int> version;
};

class IntrospectionData
{
public:
    bool m_didAttemptToRun = false;
    bool m_didRun = true;
    bool m_hasServerMode = false;

    bool m_queriedServerMode = false;
    bool m_triedCapabilities = false;

    QList<CMakeTool::Generator> m_generators;
    QMap<QString, QStringList> m_functionArgs;
    QVector<FileApi> m_fileApis;
    QStringList m_variables;
    QStringList m_functions;
    CMakeTool::Version m_version;
};

} // namespace Internal

///////////////////////////
// CMakeTool
///////////////////////////
CMakeTool::CMakeTool(Detection d, const Core::Id &id) :
    m_id(id), m_isAutoDetected(d == AutoDetection),
    m_introspection(std::make_unique<Internal::IntrospectionData>())
{
    QTC_ASSERT(m_id.isValid(), m_id = Core::Id::fromString(QUuid::createUuid().toString()));
}

CMakeTool::CMakeTool(const QVariantMap &map, bool fromSdk) :
    CMakeTool(fromSdk ? CMakeTool::AutoDetection : CMakeTool::ManualDetection,
              Core::Id::fromSetting(map.value(CMAKE_INFORMATION_ID)))
{
    m_displayName = map.value(CMAKE_INFORMATION_DISPLAYNAME).toString();
    m_isAutoRun = map.value(CMAKE_INFORMATION_AUTORUN, true).toBool();
    m_autoCreateBuildDirectory = map.value(CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY, false).toBool();
    m_readerType = Internal::readerTypeFromString(
        map.value(CMAKE_INFORMATION_READERTYPE).toString());

    //loading a CMakeTool from SDK is always autodetection
    if (!fromSdk)
        m_isAutoDetected = map.value(CMAKE_INFORMATION_AUTODETECTED, false).toBool();

    setFilePath(Utils::FilePath::fromString(map.value(CMAKE_INFORMATION_COMMAND).toString()));

    m_qchFilePath = Utils::FilePath::fromVariant(map.value(CMAKE_INFORMATION_QCH_FILE_PATH));

    if (m_qchFilePath.isEmpty())
        m_qchFilePath = searchQchFile(m_executable);
}

CMakeTool::~CMakeTool() = default;

Core::Id CMakeTool::createId()
{
    return Core::Id::fromString(QUuid::createUuid().toString());
}

void CMakeTool::setFilePath(const Utils::FilePath &executable)
{
    if (m_executable == executable)
        return;

    m_introspection = std::make_unique<Internal::IntrospectionData>();

    m_executable = executable;
    CMakeToolManager::notifyAboutUpdate(this);
}

Utils::FilePath CMakeTool::filePath() const
{
    return m_executable;
}

void CMakeTool::setAutorun(bool autoRun)
{
    if (m_isAutoRun == autoRun)
        return;

    m_isAutoRun = autoRun;
    CMakeToolManager::notifyAboutUpdate(this);
}

void CMakeTool::setAutoCreateBuildDirectory(bool autoBuildDir)
{
    if (m_autoCreateBuildDirectory == autoBuildDir)
        return;

    m_autoCreateBuildDirectory = autoBuildDir;
    CMakeToolManager::notifyAboutUpdate(this);
}

bool CMakeTool::isValid() const
{
    if (!m_id.isValid())
        return false;

    if (!m_introspection->m_didAttemptToRun)
        supportedGenerators();

    return m_introspection->m_didRun;
}

Utils::SynchronousProcessResponse CMakeTool::run(const QStringList &args, int timeoutS) const
{
    Utils::SynchronousProcess cmake;
    cmake.setTimeoutS(timeoutS);
    cmake.setFlags(Utils::SynchronousProcess::UnixTerminalDisabled);
    Utils::Environment env = Utils::Environment::systemEnvironment();
    Utils::Environment::setupEnglishOutput(&env);
    cmake.setProcessEnvironment(env.toProcessEnvironment());
    cmake.setTimeOutMessageBoxEnabled(false);

    return cmake.runBlocking({cmakeExecutable(), args});
}

QVariantMap CMakeTool::toMap() const
{
    QVariantMap data;
    data.insert(CMAKE_INFORMATION_DISPLAYNAME, m_displayName);
    data.insert(CMAKE_INFORMATION_ID, m_id.toSetting());
    data.insert(CMAKE_INFORMATION_COMMAND, m_executable.toString());
    data.insert(CMAKE_INFORMATION_QCH_FILE_PATH, m_qchFilePath.toString());
    data.insert(CMAKE_INFORMATION_AUTORUN, m_isAutoRun);
    data.insert(CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY, m_autoCreateBuildDirectory);
    if (m_readerType.has_value())
        data.insert(CMAKE_INFORMATION_READERTYPE,
                    Internal::readerTypeToString(m_readerType.value()));
    data.insert(CMAKE_INFORMATION_AUTODETECTED, m_isAutoDetected);
    return data;
}

Utils::FilePath CMakeTool::cmakeExecutable() const
{
    return cmakeExecutable(m_executable);
}

void CMakeTool::setQchFilePath(const Utils::FilePath &path)
{
    m_qchFilePath = path;
}

Utils::FilePath CMakeTool::qchFilePath() const
{
    return m_qchFilePath;
}

Utils::FilePath CMakeTool::cmakeExecutable(const Utils::FilePath &path)
{
    if (Utils::HostOsInfo::isMacHost()) {
        const QString executableString = path.toString();
        const int appIndex = executableString.lastIndexOf(".app");
        const int appCutIndex = appIndex + 4;
        const bool endsWithApp = appIndex >= 0 && appCutIndex >= executableString.size();
        const bool containsApp = appIndex >= 0 && !endsWithApp
                                 && executableString.at(appCutIndex) == '/';
        if (endsWithApp || containsApp) {
            const Utils::FilePath toTest = Utils::FilePath::fromString(
                                               executableString.left(appCutIndex))
                                               .pathAppended("Contents/bin/cmake");
            if (toTest.exists())
                return toTest.canonicalPath();
        }
    }
    return path.canonicalPath();
}

bool CMakeTool::isAutoRun() const
{
    return m_isAutoRun;
}

bool CMakeTool::autoCreateBuildDirectory() const
{
    return m_autoCreateBuildDirectory;
}

QList<CMakeTool::Generator> CMakeTool::supportedGenerators() const
{
    readInformation(QueryType::GENERATORS);
    return m_introspection->m_generators;
}

TextEditor::Keywords CMakeTool::keywords()
{
    if (m_introspection->m_functions.isEmpty() && m_introspection->m_didRun) {
        Utils::SynchronousProcessResponse response;
        response = run({"--help-command-list"}, 5);
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            m_introspection->m_functions = response.stdOut().split('\n');

        response = run({"--help-commands"}, 5);
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            parseFunctionDetailsOutput(response.stdOut());

        response = run({"--help-property-list"}, 5);
        if (response.result == Utils::SynchronousProcessResponse::Finished)
            m_introspection->m_variables = parseVariableOutput(response.stdOut());

        response = run({"--help-variable-list"}, 5);
        if (response.result == Utils::SynchronousProcessResponse::Finished) {
            m_introspection->m_variables.append(parseVariableOutput(response.stdOut()));
            m_introspection->m_variables = Utils::filteredUnique(m_introspection->m_variables);
            Utils::sort(m_introspection->m_variables);
        }
    }

    return TextEditor::Keywords(m_introspection->m_variables,
                                m_introspection->m_functions,
                                m_introspection->m_functionArgs);
}

bool CMakeTool::hasServerMode() const
{
    readInformation(QueryType::SERVER_MODE);
    return m_introspection->m_hasServerMode;
}

bool CMakeTool::hasFileApi() const
{
    readInformation(QueryType::SERVER_MODE);
    return !m_introspection->m_fileApis.isEmpty();
}

QVector<std::pair<QString, int> > CMakeTool::supportedFileApiObjects() const
{
    readInformation(QueryType::SERVER_MODE);
    return Utils::transform(m_introspection->m_fileApis, [](const Internal::FileApi &api) { return std::make_pair(api.kind, api.version.first); });
}

CMakeTool::Version CMakeTool::version() const
{
    readInformation(QueryType::VERSION);
    return m_introspection->m_version;
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
    return [](const Utils::FilePath &fn) { return fn; };
}

CMakeTool::ReaderType CMakeTool::readerType() const
{
    if (!m_readerType.has_value()) {
        // Find best possible reader type:
        if (hasFileApi()) {
            if (hasServerMode() && Internal::ignoreFileApi())
                return ServerMode; // We were asked to fall back to server mode
            return FileApi;
        }
        if (hasServerMode())
            return ServerMode;
        return TeaLeaf;
    }
    return m_readerType.value();
}

Utils::FilePath CMakeTool::searchQchFile(const Utils::FilePath &executable)
{
    if (executable.isEmpty())
        return {};

    Utils::FilePath prefixDir = executable.parentDir().parentDir();
    QDir docDir{prefixDir.pathAppended("doc/cmake").toString()};
    if (!docDir.exists())
        docDir.setPath(prefixDir.pathAppended("share/doc/cmake").toString());
    if (!docDir.exists())
        return {};

    const QStringList files = docDir.entryList(QStringList("*.qch"));
    for (const QString &docFile : files) {
        if (docFile.startsWith("cmake", Qt::CaseInsensitive)) {
            return Utils::FilePath::fromString(docDir.absoluteFilePath(docFile));
        }
    }

    return {};
}

void CMakeTool::readInformation(CMakeTool::QueryType type) const
{
    if (!m_introspection->m_didRun && m_introspection->m_didAttemptToRun)
        return;

    m_introspection->m_didAttemptToRun = true;

    if (!m_introspection->m_triedCapabilities) {
        fetchFromCapabilities();
        m_introspection->m_triedCapabilities = true;
        m_introspection->m_queriedServerMode = true; // Got added after "-E capabilities" support!
    } else {
        if ((type == QueryType::GENERATORS && !m_introspection->m_generators.isEmpty())
            || (type == QueryType::SERVER_MODE && m_introspection->m_queriedServerMode)
            || (type == QueryType::VERSION && !m_introspection->m_version.fullVersion.isEmpty()))
            return;

        if (type == QueryType::GENERATORS) {
            fetchGeneratorsFromHelp();
        } else if (type == QueryType::SERVER_MODE) {
            // Nothing to do...
        } else if (type == QueryType::VERSION) {
            fetchVersionFromVersionOutput();
        } else {
            QTC_ASSERT(false, return );
        }
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

        if (c == ' ' || c == '[' || c == '<' || c == '(' || c == ']' || c == '>' || c == ')') {
            if (!ignoreWord && !word.isEmpty()) {
                if (result.isEmpty()
                    || Utils::allOf(word, [](const QChar &c) { return c.isUpper() || c == '_'; }))
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
    const QSet<QString> functionSet = Utils::toSet(m_introspection->m_functions);

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
                        QStringList tmp = words + m_introspection->m_functionArgs[command];
                        Utils::sort(tmp);
                        m_introspection->m_functionArgs[command] = Utils::filteredUnique(tmp);
                    }
                }
                if (!words.isEmpty() && functionSet.contains(words.at(0)))
                    m_introspection->m_functionArgs[words.at(0)];
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
            result << "CMAKE_COMPILER_IS_GNUCC"
                   << "CMAKE_COMPILER_IS_GNUCXX";
        } else if (v.contains("<CONFIG>")) {
            const QString tmp = QString(v).replace("<CONFIG>", "%1");
            result << tmp.arg("DEBUG") << tmp.arg("RELEASE") << tmp.arg("MINSIZEREL")
                   << tmp.arg("RELWITHDEBINFO");
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
    m_introspection->m_didRun = m_introspection->m_didRun
                                && response.result == Utils::SynchronousProcessResponse::Finished;

    if (response.result == Utils::SynchronousProcessResponse::Finished)
        parseGeneratorsFromHelp(response.stdOut().split('\n'));
}

void CMakeTool::parseGeneratorsFromHelp(const QStringList &lines) const
{
    bool inGeneratorSection = false;
    QHash<QString, QStringList> generatorInfo;
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
        m_introspection->m_generators.append(Generator(it.key(), it.value()));
}

void CMakeTool::fetchVersionFromVersionOutput() const
{
    Utils::SynchronousProcessResponse response = run({"--version"});

    m_introspection->m_didRun = m_introspection->m_didRun
                                && response.result == Utils::SynchronousProcessResponse::Finished;

    if (response.result == Utils::SynchronousProcessResponse::Finished)
        parseVersionFormVersionOutput(response.stdOut().split('\n'));
}

void CMakeTool::parseVersionFormVersionOutput(const QStringList &lines) const
{
    QRegularExpression versionLine("^cmake.* version ((\\d+).(\\d+).(\\d+).*)$");
    for (const QString &line : lines) {
        QRegularExpressionMatch match = versionLine.match(line);
        if (!match.hasMatch())
            continue;

        m_introspection->m_version.major = match.captured(2).toInt();
        m_introspection->m_version.minor = match.captured(3).toInt();
        m_introspection->m_version.patch = match.captured(4).toInt();
        m_introspection->m_version.fullVersion = match.captured(1).toUtf8();
        break;
    }
}

void CMakeTool::fetchFromCapabilities() const
{
    Utils::SynchronousProcessResponse response = run({"-E", "capabilities"});

    if (response.result == Utils::SynchronousProcessResponse::Finished)
        parseFromCapabilities(response.stdOut());
}

static int getVersion(const QVariantMap &obj, const QString value)
{
    bool ok;
    int result = obj.value(value).toInt(&ok);
    if (!ok)
        return -1;
    return result;
}

void CMakeTool::parseFromCapabilities(const QString &input) const
{
    auto doc = QJsonDocument::fromJson(input.toUtf8());
    if (!doc.isObject())
        return;

    const QVariantMap data = doc.object().toVariantMap();
    m_introspection->m_hasServerMode = data.value("serverMode").toBool();
    const QVariantList generatorList = data.value("generators").toList();
    for (const QVariant &v : generatorList) {
        const QVariantMap gen = v.toMap();
        m_introspection->m_generators.append(Generator(gen.value("name").toString(),
                                                       gen.value("extraGenerators").toStringList(),
                                                       gen.value("platformSupport").toBool(),
                                                       gen.value("toolsetSupport").toBool()));
    }

    {
        const QVariantMap fileApis = data.value("fileApi").toMap();
        const QVariantList requests = fileApis.value("requests").toList();
        for (const QVariant &r : requests) {
            const QVariantMap object = r.toMap();
            const QString kind = object.value("kind").toString();
            const QVariantList versionList = object.value("version").toList();
            std::pair<int, int> highestVersion = std::make_pair(-1, -1);
            for (const QVariant &v : versionList) {
                const QVariantMap versionObject = v.toMap();
                const std::pair<int, int> version = std::make_pair(getVersion(versionObject,
                                                                              "major"),
                                                                   getVersion(versionObject,
                                                                              "minor"));
                if (version.first > highestVersion.first
                    || (version.first == highestVersion.first
                        && version.second > highestVersion.second))
                    highestVersion = version;
            }
            if (!kind.isNull() && highestVersion.first != -1 && highestVersion.second != -1)
                m_introspection->m_fileApis.append({kind, highestVersion});
        }
    }

    const QVariantMap versionInfo = data.value("version").toMap();
    m_introspection->m_version.major = versionInfo.value("major").toInt();
    m_introspection->m_version.minor = versionInfo.value("minor").toInt();
    m_introspection->m_version.patch = versionInfo.value("patch").toInt();
    m_introspection->m_version.fullVersion = versionInfo.value("string").toByteArray();

    // Fix up fileapi support for cmake 3.14:
    if (m_introspection->m_version.major == 3 && m_introspection->m_version.minor == 14) {
        m_introspection->m_fileApis.append({QString("codemodel"), std::make_pair(2, 0)});
        m_introspection->m_fileApis.append({QString("cache"), std::make_pair(2, 0)});
        m_introspection->m_fileApis.append({QString("cmakefiles"), std::make_pair(1, 0)});
    }
}

} // namespace CMakeProjectManager
