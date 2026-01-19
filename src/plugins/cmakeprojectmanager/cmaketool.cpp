// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmaketool.h"

#include "cmakeprojectmanagertr.h"
#include "cmaketoolmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/algorithm.h>
#include <utils/datafromprocess.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSet>
#include <QXmlStreamReader>

#include <memory>

using namespace Utils;
using namespace ProjectExplorer;

namespace CMakeProjectManager {

static Q_LOGGING_CATEGORY(cmakeToolLog, "qtc.cmake.tool", QtWarningMsg);


const char CMAKE_INFORMATION_ID[] = "Id";
const char CMAKE_INFORMATION_COMMAND[] = "Binary";
const char CMAKE_INFORMATION_DISPLAYNAME[] = "DisplayName";
const char CMAKE_INFORMATION_QCH_FILE_PATH[] = "QchFile";
// obsolete since Qt Creator 5. Kept for backward compatibility
const char CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY[] = "AutoCreateBuildDirectory";
const char CMAKE_INFORMATION_AUTODETECTED[] = "AutoDetected";
const char CMAKE_INFORMATION_DETECTIONSOURCE[] = "DetectionSource";

bool operator==(const CMakeTool::Version &v1, const CMakeTool::Version &v2)
{
    return v1.major == v2.major && v1.minor == v2.minor && v1.patch == v2.patch
           && v1.fullVersion == v2.fullVersion;
}

bool operator!=(const CMakeTool::Version &v1, const CMakeTool::Version &v2)
{
    return !(v1 == v2);
}

bool CMakeTool::Generator::matches(const QString &n) const
{
    return n == name;
}

bool operator==(const CMakeTool::Generator &g1, const CMakeTool::Generator &g2)
{
    return g1.name == g2.name
           && g1.extraGenerators == g2.extraGenerators
           && g1.supportsPlatform == g2.supportsPlatform
           && g1.supportsToolset == g2.supportsToolset;
}

bool operator!=(const CMakeTool::Generator &g1, const CMakeTool::Generator &g2)
{
    return !(g1 == g2);
}

namespace Internal {

// --------------------------------------------------------------------
// CMakeIntrospectionData:
// --------------------------------------------------------------------

class FileApi {
public:
    QString kind;
    std::pair<int, int> version;
};

bool operator==(const FileApi &f1, const FileApi &f2)
{
    return f1.kind == f2.kind && f1.version == f2.version;
}

bool operator!=(const FileApi &f1, const FileApi &f2)
{
    return !(f1 == f2);
}

class Capabilities {
public:
    static Capabilities fromJson(const QString &input);

    QList<CMakeTool::Generator> generators;
    QList<FileApi> fileApis;
    CMakeTool::Version version;
};

bool operator==(const Capabilities &c1, const Capabilities &c2)
{
    return c1.generators == c2.generators
           && c1.fileApis == c2.fileApis
           && c1.version == c2.version;
}

bool operator!=(const Capabilities &c1, const Capabilities &c2)
{
    return !(c1 == c2);
}

class IntrospectionData
{
public:
    bool m_didAttemptToRun = false;
    bool m_haveCapabilitites = true;
    bool m_haveKeywords = false;

    Capabilities m_capabilities;
    CMakeKeywords m_keywords;
    QMutex m_keywordsMutex;
};

} // namespace Internal

///////////////////////////
// CMakeTool
///////////////////////////

CMakeTool::CMakeTool(const DetectionSource &d, const Id &id)
    : m_id(id)
    , m_detectionSource(d)
    , m_introspection(std::make_unique<Internal::IntrospectionData>())
{
    QTC_ASSERT(m_id.isValid(), m_id = Id::generate());
}

CMakeTool::CMakeTool(const Store &map, bool fromSdk)
    : m_id(Id::fromSetting(map.value(CMAKE_INFORMATION_ID)))
    , m_introspection(std::make_unique<Internal::IntrospectionData>())
{
    m_displayName = map.value(CMAKE_INFORMATION_DISPLAYNAME).toString();
    m_autoCreateBuildDirectory = map.value(CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY, false).toBool();

    const DetectionSource::DetectionType type = [&] {
        if (fromSdk)
            return DetectionSource::FromSdk;
        if (map.value(CMAKE_INFORMATION_AUTODETECTED, false).toBool())
            return DetectionSource::FromSystem;
        return DetectionSource::Manual;
    }();
    const QString detectionSourceId = map.value(CMAKE_INFORMATION_DETECTIONSOURCE).toString();

    m_detectionSource = {type, detectionSourceId};

    m_qchFilePath = FilePath::fromSettings(map.value(CMAKE_INFORMATION_QCH_FILE_PATH));

    // setFilePath searches for qchFilePath if not already set
    setFilePath(FilePath::fromSettings(map.value(CMAKE_INFORMATION_COMMAND)));
}

CMakeTool::~CMakeTool() = default;

Id CMakeTool::createId()
{
    return Id::generate();
}

void CMakeTool::setFilePath(const FilePath &executable)
{
    if (m_executable == executable)
        return;

    m_introspection = std::make_unique<Internal::IntrospectionData>();

    m_executable = executable;
    if (m_qchFilePath.isEmpty())
        m_qchFilePath = searchQchFile(m_executable);

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

    return m_introspection->m_haveCapabilitites
           && !m_introspection->m_capabilities.fileApis.isEmpty();
}

Store CMakeTool::toMap() const
{
    Store data;
    data.insert(CMAKE_INFORMATION_DISPLAYNAME, m_displayName);
    data.insert(CMAKE_INFORMATION_ID, m_id.toSetting());
    data.insert(CMAKE_INFORMATION_COMMAND, m_executable.toSettings());
    data.insert(CMAKE_INFORMATION_QCH_FILE_PATH, m_qchFilePath.toSettings());
    data.insert(CMAKE_INFORMATION_AUTO_CREATE_BUILD_DIRECTORY, m_autoCreateBuildDirectory);
    data.insert(CMAKE_INFORMATION_AUTODETECTED, m_detectionSource.isAutoDetected());
    data.insert(CMAKE_INFORMATION_DETECTIONSOURCE, m_detectionSource.id);
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
        const QString executableString = path.path();
        const int appIndex = executableString.lastIndexOf(".app");
        const int appCutIndex = appIndex + 4;
        const bool endsWithApp = appIndex >= 0 && appCutIndex >= executableString.size();
        const bool containsApp = appIndex >= 0 && !endsWithApp
                                 && executableString.at(appCutIndex) == '/';
        if (endsWithApp || containsApp) {
            const FilePath toTest = path.withNewPath(executableString.left(appCutIndex))
                    .pathAppended("Contents/bin/cmake");
            if (toTest.exists())
                return toTest.canonicalPath();
        }
    }

    const FilePath resolvedPath = path.canonicalPath();
    // Evil hack to make snap-packages of CMake work. See QTCREATORBUG-23376
    if (path.osType() == OsTypeLinux && resolvedPath.fileName() == "snap")
        return path;

    return resolvedPath;
}

QList<CMakeTool::Generator> CMakeTool::supportedGenerators() const
{
    return isValid() ? m_introspection->m_capabilities.generators : QList<CMakeTool::Generator>();
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

        CommandLine command(cmakeExecutable(), {"-P", findCMakeRoot.nativePath()});
        auto outputParser = [](const QString &stdOut, const QString &) -> std::optional<FilePath> {
            QStringList output = filtered(stdOut.split('\n'), std::not_fn(&QString::isEmpty));
            if (output.size() > 0)
                return FilePath::fromString(output[0]);
            return {};
        };
        DataFromProcess<FilePath>::Parameters params(command, outputParser);
        params.environment = command.executable().deviceEnvironment();
        params.environment.setupEnglishOutput();
        params.disableUnixTerminal = true;
        const FilePath cmakeRoot = DataFromProcess<FilePath>::getData(params).value_or(FilePath());

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
    return isValid() ? !m_introspection->m_capabilities.fileApis.isEmpty() : false;
}

CMakeTool::Version CMakeTool::version() const
{
    return isValid() ? m_introspection->m_capabilities.version : CMakeTool::Version();
}

QString CMakeTool::versionDisplay() const
{
    if (m_executable.isEmpty())
        return {};

    if (!isValid())
        return Tr::tr("Version not parseable");

    const Version &version = m_introspection->m_capabilities.version;
    if (version.fullVersion.isEmpty())
        return QString::fromUtf8(version.fullVersion);

    return QString("%1.%2.%3").arg(version.major).arg(version.minor).arg(version.patch);
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

FilePath CMakeTool::searchQchFile(const FilePath &executable)
{
    if (executable.isEmpty() || !executable.isLocal()) // do not register docs from devices
        return {};

    const FilePath prefixDir = executable.resolveSymlinks().parentDir().parentDir();
    FilePath docDir = prefixDir.pathAppended("doc/cmake");
    if (!docDir.exists())
        docDir = prefixDir.pathAppended("share/doc/cmake");
    if (!docDir.exists())
        return {};

    const FilePaths files = docDir.dirEntries(QStringList("*.qch"));
    for (const FilePath &docFile : files) {
        if (docFile.fileName().startsWith("cmake", Qt::CaseInsensitive)) {
            return docFile.absoluteFilePath();
        }
    }

    return {};
}

void CMakeTool::readInformation() const
{
    QTC_ASSERT(m_introspection, return );
    if (!m_introspection->m_haveCapabilitites && m_introspection->m_didAttemptToRun)
        return;

    m_introspection->m_didAttemptToRun = true;

    fetchFromCapabilities();
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
                    const auto functionName = name.left(name.size() - 6);
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
    IDevice::ConstPtr device = DeviceManager::deviceForPath(cmakeExecutable());
    if (device
        && (device->deviceState() == IDevice::DeviceReadyToUse
            || device->deviceState() == IDevice::DeviceConnected)) {

        CommandLine command(cmakeExecutable(), {"-E", "capabilities"});
        auto outputParser = [](const QString &stdOut, const QString &) {
            return Internal::Capabilities::fromJson(stdOut);
        };
        DataFromProcess<Internal::Capabilities>::Parameters params(command, outputParser);
        params.environment = command.executable().deviceEnvironment();
        params.environment.setupEnglishOutput();
        params.disableUnixTerminal = true;
        params.errorHandler = [](const Process &p) {
            qCCritical(cmakeToolLog) << "Fetching capabilities failed: " << p.verboseExitMessage();
        };
        const auto capabilities = DataFromProcess<Internal::Capabilities>::getData(params);

        if (const auto capabilities = DataFromProcess<Internal::Capabilities>::getData(params)) {
            m_introspection->m_haveCapabilitites = true;
            m_introspection->m_capabilities = *capabilities;
            return;
        }
    } else {
        qCDebug(cmakeToolLog) << "Device for" << cmakeExecutable().toUserOutput()
                              << "is not connected";
    }
    m_introspection->m_haveCapabilitites = false;

    // In the rare case when "cmake -E capabilities" crashes / fails to run,
    // or if the device is not connected, allow to try again
    m_introspection->m_didAttemptToRun = false;
}

static int getVersion(const QVariantMap &obj, const QString &value)
{
    bool ok;
    int result = obj.value(value).toInt(&ok);
    if (!ok)
        return -1;
    return result;
}

Internal::Capabilities Internal::Capabilities::fromJson(const QString &input)
{
    Capabilities result;
    auto doc = QJsonDocument::fromJson(input.toUtf8());
    if (!doc.isObject())
        return result;

    const QVariantMap data = doc.object().toVariantMap();
    const QVariantList generatorList = data.value("generators").toList();
    for (const QVariant &v : generatorList) {
        const QVariantMap gen = v.toMap();
        result.generators.append(CMakeTool::Generator(
            gen.value("name").toString(),
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
            result.fileApis.append({kind, highestVersion});
    }

    const QVariantMap versionInfo = data.value("version").toMap();
    result.version.major = versionInfo.value("major").toInt();
    result.version.minor = versionInfo.value("minor").toInt();
    result.version.patch = versionInfo.value("patch").toInt();
    result.version.fullVersion = versionInfo.value("string").toByteArray();

    return result;
}

void CMakeTool::setDetectionSource(const DetectionSource &source)
{
    m_detectionSource = source;
}

DetectionSource CMakeTool::detectionSource() const
{
    return m_detectionSource;
}

} // namespace CMakeProjectManager
