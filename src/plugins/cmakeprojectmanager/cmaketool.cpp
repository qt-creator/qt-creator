// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmaketool.h"

#include "cmakeprojectmanagertr.h"
#include "cmaketoolmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSet>
#include <QXmlStreamReader>
#include <QUuid>

#include <memory>

using namespace Utils;

namespace CMakeProjectManager {

static Q_LOGGING_CATEGORY(cmakeToolLog, "qtc.cmake.tool", QtWarningMsg);


const char CMAKE_INFORMATION_ID[] = "Id";
const char CMAKE_INFORMATION_COMMAND[] = "Binary";
const char CMAKE_INFORMATION_DISPLAYNAME[] = "DisplayName";
const char CMAKE_INFORMATION_AUTORUN[] = "AutoRun";
const char CMAKE_INFORMATION_QCH_FILE_PATH[] = "QchFile";
// obsolete since Qt Creator 5. Kept for backward compatibility
const char CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY[] = "AutoCreateBuildDirectory";
const char CMAKE_INFORMATION_AUTODETECTED[] = "AutoDetected";
const char CMAKE_INFORMATION_DETECTIONSOURCE[] = "DetectionSource";
const char CMAKE_INFORMATION_READERTYPE[] = "ReaderType";

bool CMakeTool::Generator::matches(const QString &n) const
{
    return n == name;
}

namespace Internal {

const char READER_TYPE_FILEAPI[] = "fileapi";

static std::optional<CMakeTool::ReaderType> readerTypeFromString(const QString &input)
{
    // Do not try to be clever here, just use whatever is in the string!
    if (input == READER_TYPE_FILEAPI)
        return CMakeTool::FileApi;
    return {};
}

static QString readerTypeToString(const CMakeTool::ReaderType &type)
{
    switch (type) {
    case CMakeTool::FileApi:
        return QString(READER_TYPE_FILEAPI);
    default:
        return QString();
    }
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
    bool m_haveCapabilitites = true;
    bool m_haveKeywords = false;

    QList<CMakeTool::Generator> m_generators;
    CMakeKeywords m_keywords;
    QMutex m_keywordsMutex;
    QVector<FileApi> m_fileApis;
    CMakeTool::Version m_version;
};

} // namespace Internal

///////////////////////////
// CMakeTool
///////////////////////////
CMakeTool::CMakeTool(Detection d, const Id &id)
    : m_id(id)
    , m_isAutoDetected(d == AutoDetection)
    , m_introspection(std::make_unique<Internal::IntrospectionData>())
{
    QTC_ASSERT(m_id.isValid(), m_id = Id::fromString(QUuid::createUuid().toString()));
}

CMakeTool::CMakeTool(const Store &map, bool fromSdk) :
    CMakeTool(fromSdk ? CMakeTool::AutoDetection : CMakeTool::ManualDetection,
              Id::fromSetting(map.value(CMAKE_INFORMATION_ID)))
{
    m_displayName = map.value(CMAKE_INFORMATION_DISPLAYNAME).toString();
    m_isAutoRun = map.value(CMAKE_INFORMATION_AUTORUN, true).toBool();
    m_autoCreateBuildDirectory = map.value(CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY, false).toBool();
    m_readerType = Internal::readerTypeFromString(
        map.value(CMAKE_INFORMATION_READERTYPE).toString());

    //loading a CMakeTool from SDK is always autodetection
    if (!fromSdk)
        m_isAutoDetected = map.value(CMAKE_INFORMATION_AUTODETECTED, false).toBool();
    m_detectionSource = map.value(CMAKE_INFORMATION_DETECTIONSOURCE).toString();

    setFilePath(FilePath::fromString(map.value(CMAKE_INFORMATION_COMMAND).toString()));

    m_qchFilePath = FilePath::fromSettings(map.value(CMAKE_INFORMATION_QCH_FILE_PATH));

    if (m_qchFilePath.isEmpty())
        m_qchFilePath = searchQchFile(m_executable);
}

CMakeTool::~CMakeTool() = default;

Id CMakeTool::createId()
{
    return Id::fromString(QUuid::createUuid().toString());
}

void CMakeTool::setFilePath(const FilePath &executable)
{
    if (m_executable == executable)
        return;

    m_introspection = std::make_unique<Internal::IntrospectionData>();

    m_executable = executable;
    CMakeToolManager::notifyAboutUpdate(this);
}

FilePath CMakeTool::filePath() const
{
    return m_executable;
}

bool CMakeTool::isValid() const
{
    if (!m_id.isValid() || !m_introspection)
        return false;

    if (!m_introspection->m_didAttemptToRun)
        readInformation();

    return m_introspection->m_haveCapabilitites && !m_introspection->m_fileApis.isEmpty();
}

void CMakeTool::runCMake(Process &cmake, const QStringList &args, int timeoutS) const
{
    const FilePath executable = cmakeExecutable();
    cmake.setDisableUnixTerminal();
    Environment env = executable.deviceEnvironment();
    env.setupEnglishOutput();
    cmake.setEnvironment(env);
    cmake.setCommand({executable, args});
    cmake.runBlocking(std::chrono::seconds(timeoutS));
}

Store CMakeTool::toMap() const
{
    Store data;
    data.insert(CMAKE_INFORMATION_DISPLAYNAME, m_displayName);
    data.insert(CMAKE_INFORMATION_ID, m_id.toSetting());
    data.insert(CMAKE_INFORMATION_COMMAND, m_executable.toString());
    data.insert(CMAKE_INFORMATION_QCH_FILE_PATH, m_qchFilePath.toString());
    data.insert(CMAKE_INFORMATION_AUTORUN, m_isAutoRun);
    data.insert(CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY, m_autoCreateBuildDirectory);
    if (m_readerType)
        data.insert(CMAKE_INFORMATION_READERTYPE,
                    Internal::readerTypeToString(m_readerType.value()));
    data.insert(CMAKE_INFORMATION_AUTODETECTED, m_isAutoDetected);
    data.insert(CMAKE_INFORMATION_DETECTIONSOURCE, m_detectionSource);
    return data;
}

FilePath CMakeTool::cmakeExecutable() const
{
    return cmakeExecutable(m_executable);
}

void CMakeTool::setQchFilePath(const FilePath &path)
{
    m_qchFilePath = path;
}

FilePath CMakeTool::qchFilePath() const
{
    return m_qchFilePath;
}

FilePath CMakeTool::cmakeExecutable(const FilePath &path)
{
    if (path.osType() == OsTypeMac) {
        const QString executableString = path.toString();
        const int appIndex = executableString.lastIndexOf(".app");
        const int appCutIndex = appIndex + 4;
        const bool endsWithApp = appIndex >= 0 && appCutIndex >= executableString.size();
        const bool containsApp = appIndex >= 0 && !endsWithApp
                                 && executableString.at(appCutIndex) == '/';
        if (endsWithApp || containsApp) {
            const FilePath toTest = FilePath::fromString(executableString.left(appCutIndex))
                    .pathAppended("Contents/bin/cmake");
            if (toTest.exists())
                return toTest.canonicalPath();
        }
    }

    FilePath resolvedPath = path.canonicalPath();
    // Evil hack to make snap-packages of CMake work. See QTCREATORBUG-23376
    if (path.osType() == OsTypeLinux && resolvedPath.fileName() == "snap")
        return path;

    return resolvedPath;
}

bool CMakeTool::isAutoRun() const
{
    return m_isAutoRun;
}

QList<CMakeTool::Generator> CMakeTool::supportedGenerators() const
{
    return isValid() ? m_introspection->m_generators : QList<CMakeTool::Generator>();
}

CMakeKeywords CMakeTool::keywords()
{
    if (!isValid())
        return {};

    if (!m_introspection->m_haveKeywords && m_introspection->m_haveCapabilitites) {
        QMutexLocker locker(&m_introspection->m_keywordsMutex);
        if (m_introspection->m_haveKeywords)
            return m_introspection->m_keywords;

        Process proc;

        const FilePath findCMakeRoot = TemporaryDirectory::masterDirectoryFilePath()
                                       / "find-root.cmake";
        findCMakeRoot.writeFileContents("message(${CMAKE_ROOT})");

        FilePath cmakeRoot;
        runCMake(proc, {"-P", findCMakeRoot.nativePath()}, 5);
        if (proc.result() == ProcessResult::FinishedWithSuccess) {
            QStringList output = filtered(proc.allOutput().split('\n'),
                                          std::not_fn(&QString::isEmpty));
            if (output.size() > 0)
                cmakeRoot = FilePath::fromString(output[0]);
        }

        const struct
        {
            const QString helpPath;
            QMap<QString, FilePath> &targetMap;
        } introspections[] = {
            // Functions
            {"Help/command", m_introspection->m_keywords.functions},
            // Properties
            {"Help/prop_dir", m_introspection->m_keywords.directoryProperties},
            {"Help/prop_sf", m_introspection->m_keywords.sourceProperties},
            {"Help/prop_test", m_introspection->m_keywords.testProperties},
            {"Help/prop_tgt", m_introspection->m_keywords.targetProperties},
            {"Help/prop_gbl", m_introspection->m_keywords.properties},
            // Variables
            {"Help/variable", m_introspection->m_keywords.variables},
            // Policies
            {"Help/policy", m_introspection->m_keywords.policies},
            // Environment Variables
            {"Help/envvar", m_introspection->m_keywords.environmentVariables},
        };
        for (auto &i : introspections) {
            const FilePaths files = cmakeRoot.pathAppended(i.helpPath)
                                        .dirEntries({{"*.rst"}, QDir::Files}, QDir::Name);
            for (const auto &filePath : files)
                i.targetMap[filePath.completeBaseName()] = filePath;
        }

        for (const auto &map : {m_introspection->m_keywords.directoryProperties,
                                m_introspection->m_keywords.sourceProperties,
                                m_introspection->m_keywords.testProperties,
                                m_introspection->m_keywords.targetProperties}) {
            m_introspection->m_keywords.properties.insert(map);
        }

        // Modules
        const FilePaths files
            = cmakeRoot.pathAppended("Help/module").dirEntries({{"*.rst"}, QDir::Files}, QDir::Name);
        for (const FilePath &filePath : files) {
            const QString fileName = filePath.completeBaseName();
            if (fileName.startsWith("Find"))
                m_introspection->m_keywords.findModules[fileName.mid(4)] = filePath;
            else
                m_introspection->m_keywords.includeStandardModules[fileName] = filePath;
        }

        const QStringList moduleFunctions = parseSyntaxHighlightingXml();
        for (const auto &function : moduleFunctions)
            m_introspection->m_keywords.functions[function] = FilePath();

        m_introspection->m_haveKeywords = true;
    }

    return m_introspection->m_keywords;
}

bool CMakeTool::hasFileApi() const
{
    return isValid() ? !m_introspection->m_fileApis.isEmpty() : false;
}

CMakeTool::Version CMakeTool::version() const
{
    return isValid() ? m_introspection->m_version : CMakeTool::Version();
}

QString CMakeTool::versionDisplay() const
{
    if (m_executable.isEmpty())
        return {};

    if (!isValid())
        return Tr::tr("Version not parseable");

    const Version &version = m_introspection->m_version;
    if (version.fullVersion.isEmpty())
        return QString::fromUtf8(version.fullVersion);

    return QString("%1.%2.%3").arg(version.major).arg(version.minor).arg(version.patch);
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
    return [](const FilePath &fn) { return fn; };
}

std::optional<CMakeTool::ReaderType> CMakeTool::readerType() const
{
    if (m_readerType)
        return m_readerType; // Allow overriding the auto-detected value via .user files

    // Find best possible reader type:
    if (hasFileApi())
        return FileApi;
    return {};
}

FilePath CMakeTool::searchQchFile(const FilePath &executable)
{
    if (executable.isEmpty() || executable.needsDevice()) // do not register docs from devices
        return {};

    FilePath prefixDir = executable.parentDir().parentDir();
    QDir docDir{prefixDir.pathAppended("doc/cmake").toString()};
    if (!docDir.exists())
        docDir.setPath(prefixDir.pathAppended("share/doc/cmake").toString());
    if (!docDir.exists())
        return {};

    const QStringList files = docDir.entryList(QStringList("*.qch"));
    for (const QString &docFile : files) {
        if (docFile.startsWith("cmake", Qt::CaseInsensitive)) {
            return FilePath::fromString(docDir.absoluteFilePath(docFile));
        }
    }

    return {};
}

QString CMakeTool::documentationUrl(const Version &version, bool online)
{
    if (online) {
        QString helpVersion = "latest";
        if (!(version.major == 0 && version.minor == 0))
            helpVersion = QString("v%1.%2").arg(version.major).arg(version.minor);

        return QString("https://cmake.org/cmake/help/%1").arg(helpVersion);
    }

    return QString("qthelp://org.cmake.%1.%2.%3/doc")
        .arg(version.major)
        .arg(version.minor)
        .arg(version.patch);
}

void CMakeTool::openCMakeHelpUrl(const CMakeTool *tool, const QString &linkUrl)
{
    bool online = true;
    Version version;
    if (tool && tool->isValid()) {
        online = tool->qchFilePath().isEmpty();
        version = tool->version();
    }

    Core::HelpManager::showHelpUrl(linkUrl.arg(documentationUrl(version, online)));
}

void CMakeTool::readInformation() const
{
    QTC_ASSERT(m_introspection, return );
    if (!m_introspection->m_haveCapabilitites && m_introspection->m_didAttemptToRun)
        return;

    m_introspection->m_didAttemptToRun = true;

    fetchFromCapabilities();
}


static QStringList parseDefinition(const QString &definition)
{
    QStringList result;
    QString word;
    bool ignoreWord = false;
    QVector<QChar> braceStack;

    for (const QChar &c : definition) {
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
    bool expectDefinition = false;
    QString currentDefinition;

    const QStringList lines = output.split('\n');
    for (int i = 0; i < lines.count(); ++i) {
        const QString &line = lines.at(i);

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
                    if (m_introspection->m_keywords.functions.contains(command)) {
                        const QStringList tmp = Utils::sorted(
                            words + m_introspection->m_keywords.functionArgs[command]);
                        m_introspection->m_keywords.functionArgs[command] = Utils::filteredUnique(
                            tmp);
                    }
                }
                if (!words.isEmpty() && m_introspection->m_keywords.functions.contains(words.at(0)))
                    m_introspection->m_keywords.functionArgs[words.at(0)];
                currentDefinition.clear();
            } else {
                currentDefinition.append(line.trimmed() + ' ');
            }
        }
    }
}

QStringList CMakeTool::parseVariableOutput(const QString &output)
{
    const QStringList variableList = Utils::filtered(output.split('\n'),
                                                     std::not_fn(&QString::isEmpty));
    QStringList result;
    for (const QString &v : variableList) {
        if (v.startsWith("CMAKE_COMPILER_IS_GNU<LANG>")) { // This key takes a compiler name :-/
            result << "CMAKE_COMPILER_IS_GNUCC"
                   << "CMAKE_COMPILER_IS_GNUCXX";
        } else if (v.contains("<CONFIG>") && v.contains("<LANG>")) {
            const QString tmp = QString(v).replace("<CONFIG>", "%1").replace("<LANG>", "%2");
            result << tmp.arg("DEBUG").arg("C") << tmp.arg("DEBUG").arg("CXX")
                   << tmp.arg("RELEASE").arg("C") << tmp.arg("RELEASE").arg("CXX")
                   << tmp.arg("MINSIZEREL").arg("C") << tmp.arg("MINSIZEREL").arg("CXX")
                   << tmp.arg("RELWITHDEBINFO").arg("C") << tmp.arg("RELWITHDEBINFO").arg("CXX");
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

QStringList CMakeTool::parseSyntaxHighlightingXml()
{
    QStringList moduleFunctions;

    const FilePath cmakeXml = Core::ICore::resourcePath("generic-highlighter/syntax/cmake.xml");
    QXmlStreamReader reader(cmakeXml.fileContents().value_or(QByteArray()));

    auto readItemList = [](QXmlStreamReader &reader) -> QStringList {
        QStringList arguments;
        while (!reader.atEnd() && reader.readNextStartElement()) {
            if (reader.name() == u"item")
                arguments.append(reader.readElementText());
            else
                reader.skipCurrentElement();
        }
        return arguments;
    };

    while (!reader.atEnd() && reader.readNextStartElement()) {
        if (reader.name() != u"highlighting")
            continue;
        while (!reader.atEnd() && reader.readNextStartElement()) {
            if (reader.name() == u"list") {
                const auto name = reader.attributes().value("name").toString();
                if (name.endsWith(u"_sargs") || name.endsWith(u"_nargs")) {
                    const auto functionName = name.left(name.length() - 6);
                    QStringList arguments = readItemList(reader);

                    if (m_introspection->m_keywords.functionArgs.contains(functionName))
                        arguments.append(
                            m_introspection->m_keywords.functionArgs.value(functionName));

                    m_introspection->m_keywords.functionArgs[functionName] = arguments;

                    // Functions that are part of CMake modules like ExternalProject_Add
                    // which are not reported by cmake --help-list-commands
                    if (!m_introspection->m_keywords.functions.contains(functionName)) {
                        moduleFunctions << functionName;
                    }
                } else if (name == u"generator-expressions") {
                    m_introspection->m_keywords.generatorExpressions = toSet(readItemList(reader));
                } else {
                    reader.skipCurrentElement();
                }
            } else {
                reader.skipCurrentElement();
            }
        }
    }

    // Some commands have the same arguments as other commands and the `cmake.xml`
    // but their relationship is weirdly defined in the `cmake.xml` file.
    using ListStringPair = QList<QPair<QString, QString>>;
    const ListStringPair functionPairs = {{"if", "elseif"},
                                          {"while", "elseif"},
                                          {"find_path", "find_file"},
                                          {"find_program", "find_library"},
                                          {"target_link_libraries", "target_compile_definitions"},
                                          {"target_link_options", "target_compile_definitions"},
                                          {"target_link_directories", "target_compile_options"},
                                          {"set_target_properties", "set_directory_properties"},
                                          {"set_tests_properties", "set_directory_properties"}};
    for (const auto &pair : std::as_const(functionPairs)) {
        if (!m_introspection->m_keywords.functionArgs.contains(pair.first))
            m_introspection->m_keywords.functionArgs[pair.first]
                = m_introspection->m_keywords.functionArgs.value(pair.second);
    }

    // Special case for cmake_print_variables, which will print the names and values for variables
    // and needs to be as a known function
    const QString cmakePrintVariables("cmake_print_variables");
    m_introspection->m_keywords.functionArgs[cmakePrintVariables] = {};
    moduleFunctions << cmakePrintVariables;

    moduleFunctions.removeDuplicates();
    return moduleFunctions;
}

void CMakeTool::fetchFromCapabilities() const
{
    Process cmake;
    runCMake(cmake, {"-E", "capabilities"});

    if (cmake.result() == ProcessResult::FinishedWithSuccess) {
        m_introspection->m_haveCapabilitites = true;
        parseFromCapabilities(cmake.cleanedStdOut());
    } else {
        qCCritical(cmakeToolLog) << "Fetching capabilities failed: " << cmake.allOutput() << cmake.error();
        m_introspection->m_haveCapabilitites = false;
    }
}

static int getVersion(const QVariantMap &obj, const QString &value)
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
    const QVariantList generatorList = data.value("generators").toList();
    for (const QVariant &v : generatorList) {
        const QVariantMap gen = v.toMap();
        m_introspection->m_generators.append(Generator(gen.value("name").toString(),
                                                       gen.value("extraGenerators").toStringList(),
                                                       gen.value("platformSupport").toBool(),
                                                       gen.value("toolsetSupport").toBool()));
    }

    const QVariantMap fileApis = data.value("fileApi").toMap();
    const QVariantList requests = fileApis.value("requests").toList();
    for (const QVariant &r : requests) {
        const QVariantMap object = r.toMap();
        const QString kind = object.value("kind").toString();
        const QVariantList versionList = object.value("version").toList();
        std::pair<int, int> highestVersion{-1, -1};
        for (const QVariant &v : versionList) {
            const QVariantMap versionObject = v.toMap();
            const std::pair<int, int> version{getVersion(versionObject, "major"),
                                              getVersion(versionObject, "minor")};
            if (version.first > highestVersion.first
                || (version.first == highestVersion.first && version.second > highestVersion.second))
                highestVersion = version;
        }
        if (!kind.isNull() && highestVersion.first != -1 && highestVersion.second != -1)
            m_introspection->m_fileApis.append({kind, highestVersion});
    }

    const QVariantMap versionInfo = data.value("version").toMap();
    m_introspection->m_version.major = versionInfo.value("major").toInt();
    m_introspection->m_version.minor = versionInfo.value("minor").toInt();
    m_introspection->m_version.patch = versionInfo.value("patch").toInt();
    m_introspection->m_version.fullVersion = versionInfo.value("string").toByteArray();
}

} // namespace CMakeProjectManager
