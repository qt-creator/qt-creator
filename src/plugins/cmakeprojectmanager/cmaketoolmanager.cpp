// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmaketoolmanager.h"

#include "cmakekitaspect.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "cmaketoolsettingsaccessor.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <cmakelang/cmakedoc.h>
#include <cmakelang/cmakedocument.h>

#include <rstlang/rstdocument.h>
#include <rstlang/rstmarkdown.h>

#include <utils/async.h>
#include <utils/environment.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <nanotrace/nanotrace.h>

#include <QCache>
#include <QCryptographicHash>
#include <QDateTime>
#include <QMutex>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <winioctl.h>

// taken from qtbase/src/corelib/io/qfilesystemengine_win.cpp
#if !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)
typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#  define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#endif // !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)

#ifndef FSCTL_SET_REPARSE_POINT
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM,41,METHOD_BUFFERED,FILE_ANY_ACCESS)
#endif
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

#ifdef Q_OS_WIN
static Q_LOGGING_CATEGORY(cmakeToolManagerLog, "qtc.cmake.toolmanager", QtWarningMsg);
#endif

class CMakeToolManagerPrivate
{
public:
    Id m_defaultCMake;
    std::vector<std::unique_ptr<CMakeTool>> m_cmakeTools;
    Internal::CMakeToolSettingsAccessor m_accessor;
    FilePath m_junctionsDir;
    int m_junctionsHashLength = 32;

    CMakeToolManagerPrivate();
};

static CMakeToolManagerPrivate *d = nullptr;
static CMakeToolManager *m_instance = nullptr;

CMakeToolManager::CMakeToolManager()
{
    qRegisterMetaType<QString *>();

    d = new CMakeToolManagerPrivate;
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &CMakeToolManager::saveCMakeTools);

    connect(DeviceManager::instance(), &DeviceManager::toolDetectionRequested,
            this, &CMakeToolManager::handleDeviceToolDetectionRequest);

    setObjectName("CMakeToolManager");
    ExtensionSystem::PluginManager::addObject(this);
}

CMakeToolManager::~CMakeToolManager()
{
    ExtensionSystem::PluginManager::removeObject(this);
    delete d;
}

CMakeToolManager *CMakeToolManager::instance()
{
    return m_instance;
}

QList<CMakeTool *> CMakeToolManager::cmakeTools()
{
    return Utils::toRawPointer<QList>(d->m_cmakeTools);
}

bool CMakeToolManager::registerCMakeTool(std::unique_ptr<CMakeTool> &&tool)
{
    if (!tool || Utils::contains(d->m_cmakeTools, tool.get()))
        return true;

    const Utils::Id toolId = tool->id();
    QTC_ASSERT(toolId.isValid(),return false);

    //make sure the same id was not used before
    QTC_ASSERT(!Utils::contains(d->m_cmakeTools, [toolId](const std::unique_ptr<CMakeTool> &known) {
        return toolId == known->id();
    }), return false);

    d->m_cmakeTools.emplace_back(std::move(tool));

    emit m_instance->cmakeAdded(toolId);

    ensureDefaultCMakeToolIsValid();

    updateDocumentation();

    return true;
}

void CMakeToolManager::deregisterCMakeTool(const Id &id)
{
    auto toRemove = Utils::take(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
    if (toRemove.has_value()) {
        ensureDefaultCMakeToolIsValid();

        updateDocumentation();

        emit m_instance->cmakeRemoved(id);
    }
}

std::vector<std::unique_ptr<CMakeTool>> CMakeToolManager::autoDetectCMakeTools(
    const FilePaths &searchPaths, const FilePath &rootPath)
{
    QStringList extraDirs;

    if (rootPath.osType() == OsTypeWindows) {
        for (const auto &envVar : QStringList{"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
            if (qtcEnvironmentVariableIsSet(envVar)) {
                const QString progFiles = qtcEnvironmentVariable(envVar);
                extraDirs.append(progFiles + "/CMake");
                extraDirs.append(progFiles + "/CMake/bin");
            }
        }
    } else if (rootPath.osType() == OsTypeMac) {
        extraDirs.append("/Applications/CMake.app/Contents/bin");
        extraDirs.append("/usr/local/bin");    // homebrew intel
        extraDirs.append("/opt/homebrew/bin"); // homebrew arm
        extraDirs.append("/opt/local/bin");    // macports
    }

    const FilePaths suspects = rootPath.withNewMappedPath(FilePath("cmake"))
                                   .searchAllInDirectories(
                                       searchPaths + FilePaths::resolvePaths(rootPath, extraDirs));

    std::vector<std::unique_ptr<CMakeTool>> found;
    for (const FilePath &command : std::as_const(suspects)) {
        // Consider remote tools as manual, like we want for the "Auto-detect" button in the settings
        const DetectionSource detectionSource = command.isLocal() ? DetectionSource::FromSystem
                                                                  : DetectionSource::Manual;
        auto item = std::make_unique<CMakeTool>(detectionSource, CMakeTool::createId());
        item->setFilePath(command);
        item->setDisplayName(Tr::tr("System CMake at %1").arg(command.toUserOutput()));

        found.emplace_back(std::move(item));
    }

    return found;
}

CMakeKeywords CMakeToolManager::defaultProjectOrDefaultCMakeKeyWords()
{
    if (auto bs = activeBuildSystemForCurrentProject()) {
        CMakeKeywords keywords = CMakeKitAspect::cmakeKeywords(bs->kit());
        if (!keywords.variables.isEmpty())
            return keywords;
    }

    if (auto tool = CMakeToolManager::defaultCMakeTool())
        return tool->keywords();

    return {};
}

// The CMake whose keywords an editor shows: the one the current project is
// configured with, or the default one.
static CMakeTool *keywordTool()
{
    CMakeTool *tool = nullptr;
    if (auto bs = activeBuildSystemForCurrentProject()) {
        const FilePath executable = CMakeKitAspect::cmakeExecutable(bs->kit());
        if (!executable.isEmpty())
            tool = CMakeToolManager::findByCommand(executable);
    }
    if (!tool)
        tool = CMakeToolManager::defaultCMakeTool();
    return tool;
}

void CMakeToolManager::readKeywords()
{
    CMakeTool *tool = keywordTool();
    if (!tool)
        return;

    tool->readKeywords().then(m_instance, [] { emit m_instance->keywordsRead(); });
}

std::optional<CMakeKeywords> CMakeToolManager::keywordsIfRead()
{
    if (CMakeTool *tool = keywordTool())
        return tool->keywordsIfRead();

    return {};
}

CMakeTool *CMakeToolManager::defaultCMakeTool()
{
    return findById(d->m_defaultCMake);
}

void CMakeToolManager::setDefaultCMakeTool(const Id &id)
{
    if (d->m_defaultCMake != id && findById(id)) {
        d->m_defaultCMake = id;
        emit m_instance->defaultCMakeChanged();
        return;
    }

    ensureDefaultCMakeToolIsValid();
}

CMakeTool *CMakeToolManager::findByCommand(const FilePath &command)
{
    return Utils::findOrDefault(
        d->m_cmakeTools,
        Utils::equal(&CMakeTool::cmakeExecutable, CMakeTool::cmakeExecutable(command)));
}

Id CMakeToolManager::idForExecutable(const FilePath &cmakeExecutable)
{
    if (CMakeTool *tool = findByCommand(cmakeExecutable))
        return tool->id();
    return {};
}

CMakeTool *CMakeToolManager::findById(const Id &id)
{
    return Utils::findOrDefault(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
}

FilePath CMakeToolManager::executableForId(const Id id)
{
    if (CMakeTool *tool = findById(id))
        return tool->cmakeExecutable();
    return {};
}

void CMakeToolManager::restoreCMakeTools()
{
    NANOTRACE_SCOPE("CMakeProjectManager", "CMakeToolManager::restoreCMakeTools");
    Internal::CMakeToolSettingsAccessor::CMakeTools tools = d->m_accessor.restoreCMakeTools();
    d->m_cmakeTools = std::move(tools.cmakeTools);
    setDefaultCMakeTool(tools.defaultToolId);

    updateDocumentation();

    emit m_instance->cmakeToolsLoaded();
}

void CMakeToolManager::updateDocumentation()
{
    const QList<CMakeTool *> tools = cmakeTools();
    FilePaths docs;
    for (const auto tool : tools) {
        if (!tool->qchFilePath().isEmpty())
            docs.append(tool->qchFilePath());
    }
    Core::HelpManager::registerDocumentation(docs);
}

static void createJunction(const FilePath &from, const FilePath &to)
{
#ifdef Q_OS_WIN
    to.createDir();
    const QString toString = to.path();

    HANDLE handle = ::CreateFile((wchar_t *) toString.utf16(),
                                 GENERIC_WRITE,
                                 0,
                                 nullptr,
                                 OPEN_EXISTING,
                                 FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                 nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        qCDebug(cmakeToolManagerLog())
            << "Failed to open" << toString << "to create a junction." << ::GetLastError();
        return;
    }

    QString fromString("\\??\\");
    fromString.append(from.absoluteFilePath().nativePath());

    auto fromStringLength = uint16_t(fromString.length() * sizeof(wchar_t));
    auto toStringLength = uint16_t(toString.length() * sizeof(wchar_t));
    auto reparseDataLength = fromStringLength + toStringLength + 12;

    std::vector<char> buf(reparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE, 0);
    REPARSE_DATA_BUFFER &reparse = *reinterpret_cast<REPARSE_DATA_BUFFER *>(buf.data());

    reparse.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    reparse.ReparseDataLength = reparseDataLength;

    reparse.MountPointReparseBuffer.SubstituteNameOffset = 0;
    reparse.MountPointReparseBuffer.SubstituteNameLength = fromStringLength;
    fromString.toWCharArray(reparse.MountPointReparseBuffer.PathBuffer);

    reparse.MountPointReparseBuffer.PrintNameOffset = fromStringLength + sizeof(UNICODE_NULL);
    reparse.MountPointReparseBuffer.PrintNameLength = toStringLength;
    toString.toWCharArray(reparse.MountPointReparseBuffer.PathBuffer + fromString.length() + 1);

    DWORD retsize = 0;
    if (!::DeviceIoControl(handle,
                           FSCTL_SET_REPARSE_POINT,
                           &reparse,
                           uint16_t(buf.size()),
                           nullptr,
                           0,
                           &retsize,
                           nullptr)) {
        qCDebug(cmakeToolManagerLog()) << "Failed to create junction from" << fromString << "to"
                                       << toString << "GetLastError:" << ::GetLastError();
    }
    ::CloseHandle(handle);
#else
    Q_UNUSED(from)
    Q_UNUSED(to)
#endif
}

// What a file of the Help of CMake documents, which is what it is named
// after.
static CMakeLang::Documentation::Kind kindOfHelpFile(const FilePath &file)
{
    const QString directory = file.parentDir().fileName();
    if (directory == "command")
        return CMakeLang::Documentation::Command;
    if (directory == "variable")
        return CMakeLang::Documentation::Variable;
    if (directory == "envvar")
        return CMakeLang::Documentation::EnvironmentVariable;
    if (directory == "module")
        return CMakeLang::Documentation::Module;
    if (directory == "policy")
        return CMakeLang::Documentation::Policy;
    if (directory.startsWith("prop_"))
        return CMakeLang::Documentation::Property;
    return CMakeLang::Documentation::Unknown;
}

// The Help of CMake is written over several files: one names another with an
// include, and the documentation of a module stands in the module itself.
static RstLang::ParseOptions parseOptions(const FilePath &directory)
{
    RstLang::ParseOptions options;
    options.includeDirectives << "cmake-module";
    options.resolveInclude = [directory](const QString &type,
                                         const QString &path) -> std::optional<QString> {
        const Result<QByteArray> contents = directory.resolvePath(path).fileContents();
        if (!contents)
            return {};

        const QString source = QString::fromUtf8(*contents);
        if (type != "cmake-module")
            return source;

        // What a module documents it documents in a ".rst:" comment of its
        // own.
        QStringList blocks;
        for (const CMakeLang::DocComment &comment : CMakeLang::documentationComments(source))
            blocks.append(comment.text);
        return blocks.join('\n');
    };
    return options;
}

static QList<CMakeLang::Documentation> readDocumentation(const FilePath &file)
{
    const Result<QByteArray> contents = file.fileContents();
    if (!contents)
        return {};

    const QString source = QString::fromUtf8(*contents);

    // Everything that is not written in reStructuredText already is a CMake
    // file, and one of those carries the documentation of what it provides
    // in its own comments.
    if (file.suffix() != "rst")
        return CMakeLang::documentation(CMakeLang::Document::fromSource(source));

    const RstLang::DocumentPtr rst
        = RstLang::Document::fromSource(source, parseOptions(file.parentDir()));

    // The file is named after what it documents, and spells out what else it
    // has to say about the commands of a module.
    QList<CMakeLang::Documentation> result
        = {CMakeLang::documentationFor(rst, file.completeBaseName(), kindOfHelpFile(file))};
    result.append(CMakeLang::documentation(rst));
    return result;
}

CMakeLang::Documentation CMakeToolManager::documentation(const QString &name,
                                                         const FilePath &file)
{
    if (file.isEmpty())
        return {};

    // What a file was read to say, and when it said it.  A file of a
    // project is written while it is being edited, and what it said then is
    // not what it says now.
    class Read
    {
    public:
        QDateTime lastModified;
        QList<CMakeLang::Documentation> documentation;
    };

    // One of these holds the parsed source of the file it was read from, so
    // only the files that are being asked about are kept.
    static QCache<FilePath, Read> cache(32);
    static QMutex mutex;

    const QDateTime lastModified = file.lastModified();

    std::optional<QList<CMakeLang::Documentation>> documentation;
    {
        QMutexLocker locker(&mutex);
        const Read *read = cache.object(file);
        if (read && read->lastModified == lastModified)
            documentation = read->documentation;
    }

    // Reading the file and parsing what it says is what this costs, and no
    // other reader waits for it.
    if (!documentation) {
        documentation = readDocumentation(file);
        QMutexLocker locker(&mutex);
        cache.insert(file, new Read{lastModified, *documentation});
    }

    for (const CMakeLang::Documentation &candidate : *documentation) {
        if (candidate.isNamed(name))
            return candidate;
    }

    // A file that says nothing about the name still documents what it is
    // there for.
    return documentation->isEmpty() ? CMakeLang::Documentation() : documentation->first();
}

QString CMakeToolManager::toolTip(const QString &name, const FilePath &file)
{
    return CMakeToolManager::documentation(name, file).brief();
}

FilePath CMakeToolManager::mappedFilePath(Project *project, const FilePath &path)
{
    if (!HostOsInfo::isWindowsHost())
        return path;

    if (!path.isLocal())
        return path;

    auto environment = Environment::systemEnvironment();
    if (project)
        project->additionalEnvironment().modifyEnvironment(environment, globalMacroExpander());
    const bool enableJunctions
        = QVariant(environment.value_or(
                       "QTC_CMAKE_USE_JUNCTIONS",
                       Internal::cmakeSettingsForProject(project).useJunctionsForSourceAndBuildDirectories() ? "1"
                                                                                              : "0"))
              .toBool();

    if (!enableJunctions)
        return path;

    if (!d->m_junctionsDir.isDir())
        return path;

    const auto hashPath = QString::fromUtf8(
        QCryptographicHash::hash(path.path().toUtf8(), QCryptographicHash::Md5).toHex(0));
    const auto fullHashPath = d->m_junctionsDir.pathAppended(
        hashPath.left(d->m_junctionsHashLength));

    if (!fullHashPath.exists())
        createJunction(path, fullHashPath);

    return fullHashPath.exists() ? fullHashPath : path;
}

void CMakeToolManager::removeDetectedCMake(
    const QString &detectionSource, const LogCallback &logCallback)
{
    while (true) {
        auto toRemove = Utils::take(d->m_cmakeTools, [detectionSource](const auto &tool) {
            return tool->detectionSource().id == detectionSource
                   && tool->detectionSource().isAutoDetected();
        });
        if (!toRemove.has_value())
            break;
        logCallback(Tr::tr("Removing CMake tool \"%1\".").arg((*toRemove)->displayName()));
        emit m_instance->cmakeRemoved((*toRemove)->id());
    }

    ensureDefaultCMakeToolIsValid();
    updateDocumentation();
}

void CMakeToolManager::notifyAboutUpdate(CMakeTool *tool)
{
    if (!tool || !Utils::contains(d->m_cmakeTools, tool))
        return;
    emit m_instance->cmakeUpdated(tool->id());
}

void CMakeToolManager::saveCMakeTools()
{
    d->m_accessor.saveCMakeTools(cmakeTools(), d->m_defaultCMake);
}

void CMakeToolManager::ensureDefaultCMakeToolIsValid()
{
    const Utils::Id oldId = d->m_defaultCMake;
    if (d->m_cmakeTools.size() == 0) {
        d->m_defaultCMake = Utils::Id();
    } else {
        if (findById(d->m_defaultCMake))
            return;
        auto cmakeTool = Utils::findOrDefault(cmakeTools(), [](CMakeTool *tool) {
            return tool->cmakeExecutable().isLocal();
        });
        if (cmakeTool)
            d->m_defaultCMake = cmakeTool->id();
    }

    // signaling:
    if (oldId != d->m_defaultCMake)
        emit m_instance->defaultCMakeChanged();
}

void CMakeToolManager::handleDeviceToolDetectionRequest(
    Utils::Id devId, const FilePaths &searchPaths, quint64 token,
    const ProjectExplorer::ToolDetectionLogger &logger)
{
    const IDevicePtr dev = DeviceManager::find(devId);
    QTC_ASSERT(dev, return);
    dev->registerToolDetectionTask(token);
    if (logger)
        logger.logTopLevel(Tr::tr("Searching for CMake..."));
    const auto future = Utils::asyncRun(autoDetectCMakeTools, searchPaths, dev->rootPath());
    const auto cont = [devId, token, logger](auto &&future) {
        const IDevicePtr dev = DeviceManager::find(devId);
        if (!dev)
            return;
        auto detected = future.takeResult();
        bool foundNew = false;
        for (auto &&tool : detected) {
            if (!CMakeToolManager::findByCommand(tool->cmakeExecutable())) {
                foundNew = true;
                if (logger)
                    logger.logItem(
                        Tr::tr("Found CMake: %1").arg(tool->cmakeExecutable().toUserOutput()));
                CMakeToolManager::registerCMakeTool(std::move(tool));
            }
        }
        if (logger && !foundNew)
            logger.logItem(Tr::tr("No new CMake found."));
        dev->deregisterToolDetectionTask(token);
    };
    Utils::onFinished(future, this, cont);
}

void Internal::setupCMakeToolManager(QObject *guard)
{
    m_instance = new CMakeToolManager;
    m_instance->setParent(guard);
}

CMakeToolManagerPrivate::CMakeToolManagerPrivate()
{
    if (HostOsInfo::isWindowsHost()) {
        QStringList locations = QStandardPaths::standardLocations(
            QStandardPaths::GenericConfigLocation);
        Utils::sort(locations, [](const QString &lhs, const QString &rhs) {
            return lhs.size() < rhs.size();
        });
        m_junctionsDir = FilePath::fromString(locations.first()).pathAppended("QtCreator/Links");

        auto project = ProjectManager::startupProject();
        auto environment = Environment::systemEnvironment();
        if (project)
            project->additionalEnvironment().modifyEnvironment(environment, globalMacroExpander());

        if (environment.hasKey("QTC_CMAKE_JUNCTIONS_DIR"))
            m_junctionsDir = FilePath::fromUserInput(environment.value("QTC_CMAKE_JUNCTIONS_DIR"));

        if (environment.hasKey("QTC_CMAKE_JUNCTIONS_HASH_LENGTH")) {
            bool ok = false;
            const int hashLength = environment.value("QTC_CMAKE_JUNCTIONS_HASH_LENGTH").toInt(&ok);
            if (ok && hashLength >= 4 && hashLength < 32)
                m_junctionsHashLength = hashLength;
        }
        if (!m_junctionsDir.exists())
            m_junctionsDir.createDir();
    }
}

} // CMakeProjectManager
