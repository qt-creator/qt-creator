// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonbuildsystem.h"

#include "kitdata.h"
#include "mesonbuildconfiguration.h"
#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "settings.h"
#include "toolkitaspectwidget.h"

#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtcppkitinfo.h>

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>

#define LEAVE_IF_BUSY() \
    { \
        if (m_parseGuard.guardsProject()) \
            return false; \
    }
#define LOCK() \
    { \
        m_parseGuard = guardParsingRun(); \
    }

#define UNLOCK(success) \
    { \
        if (success) \
            m_parseGuard.markAsSuccess(); \
        m_parseGuard = {}; \
    };

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

static Q_LOGGING_CATEGORY(mesonBuildSystemLog, "qtc.meson.buildsystem", QtWarningMsg);

const char MACHINE_FILE_PREFIX[] = "Meson-MachineFile-";
const char MACHINE_FILE_EXT[] = ".ini";

static KitData createKitData(const Kit *kit)
{
    QTC_ASSERT(kit, return {});

    MacroExpander *expander = kit->macroExpander();

    KitData data;
    data.cCompilerPath = expander->expand(QString("%{Compiler:Executable:C}"));
    data.cxxCompilerPath = expander->expand(QString("%{Compiler:Executable:Cxx}"));
    data.cmakePath = expander->expand(QString("%{CMake:Executable:FilePath}"));
    data.qmakePath = expander->expand(QString("%{Qt:qmakeExecutable}"));
    data.qtPrefixPath = expander->expand(QString("%{Qt:QT_INSTALL_PREFIX}"));
    data.qtVersionStr = expander->expand(QString("%{Qt:Version}"));
    data.pythonPath = expander->expand(QString("%{Python:Path}"));
    data.qtVersion = QtMajorVersion::None;
    auto version = QVersionNumber::fromString(data.qtVersionStr);
    if (!version.isNull()) {
        switch (version.majorVersion()) {
        case 4:
            data.qtVersion = QtMajorVersion::Qt4;
            break;
        case 5:
            data.qtVersion = QtMajorVersion::Qt5;
            break;
        case 6:
            data.qtVersion = QtMajorVersion::Qt6;
            break;
        default:
            data.qtVersion = QtMajorVersion::Unknown;
        }
    }
    return data;
}

static FilePath machineFilesDir()
{
    return Core::ICore::userResourcePath("Meson-machine-files");
}

static FilePath machineFile(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    auto baseName
        = QString("%1%2%3").arg(MACHINE_FILE_PREFIX).arg(kit->id().toString()).arg(MACHINE_FILE_EXT);
    baseName = baseName.remove('{').remove('}');
    return machineFilesDir().pathAppended(baseName);
}

// MachineFileManager

class MachineFileManager final : public QObject
{
public:
    MachineFileManager();

private:
    void addMachineFile(const Kit *kit);
    void removeMachineFile(const Kit *kit);
    void updateMachineFile(const Kit *kit);
    void cleanupMachineFiles();
};

MachineFileManager::MachineFileManager()
{
    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &MachineFileManager::addMachineFile);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &MachineFileManager::updateMachineFile);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &MachineFileManager::removeMachineFile);
    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &MachineFileManager::cleanupMachineFiles);
}

void MachineFileManager::addMachineFile(const Kit *kit)
{
    FilePath filePath = machineFile(kit);
    QTC_ASSERT(!filePath.isEmpty(), return );
    auto kitData = createKitData(kit);

    auto entry = [](const QString &key, const QString &value) {
        return QString("%1 = '%2'\n").arg(key).arg(value).toUtf8();
    };

    QByteArray ba = "[binaries]\n";
    ba += entry("c", kitData.cCompilerPath);
    ba += entry("cpp", kitData.cxxCompilerPath);
    ba += entry("qmake", kitData.qmakePath);
    if (!kitData.pythonPath.isEmpty()){
        ba += entry("python3", kitData.pythonPath);
        ba += entry("python", kitData.pythonPath);
    }
    if (kitData.qtVersion == QtMajorVersion::Qt4)
        ba += entry("qmake-qt4", kitData.qmakePath);
    else if (kitData.qtVersion == QtMajorVersion::Qt5)
        ba += entry("qmake-qt5", kitData.qmakePath);
    else if (kitData.qtVersion == QtMajorVersion::Qt6)
        ba += entry("qmake-qt6", kitData.qmakePath);
    ba += entry("cmake", kitData.cmakePath);
    ba += "\n[cmake]\n";
    ba += entry("CMAKE_C_COMPILER", kitData.cCompilerPath);
    ba += entry("CMAKE_CXX_COMPILER", kitData.cxxCompilerPath);
    ba += entry("CMAKE_PREFIX_PATH", kitData.qtPrefixPath);

    filePath.writeFileContents(ba);
}

void MachineFileManager::removeMachineFile(const Kit *kit)
{
    FilePath filePath = machineFile(kit);
    if (filePath.exists())
        filePath.removeFile();
}

void MachineFileManager::updateMachineFile(const Kit *kit)
{
    addMachineFile(kit);
}

void MachineFileManager::cleanupMachineFiles()
{
    FilePath dir = machineFilesDir();
    dir.ensureWritableDir();

    const FileFilter filter = {{QString("%1*%2").arg(MACHINE_FILE_PREFIX).arg(MACHINE_FILE_EXT)}};
    const FilePaths machineFiles = dir.dirEntries(filter);

    FilePaths expected;
    for (Kit const *kit : KitManager::kits()) {
        const FilePath fname = machineFile(kit);
        expected.push_back(fname);
        if (!machineFiles.contains(fname))
            addMachineFile(kit);
    }

    for (const FilePath &file : machineFiles) {
        if (!expected.contains(file))
            file.removeFile();
    }
}

// MesonBuildSystem

MesonBuildSystem::MesonBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
    , m_parser(MesonToolKitAspect::mesonToolId(bc->kit()), bc->environment(), project())
    , m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater())
{
    qCDebug(mesonBuildSystemLog) << "Init";
    connect(bc, &BuildConfiguration::kitChanged, this, [this] {
        updateKit(kit());
    });
    connect(bc, &MesonBuildConfiguration::buildDirectoryChanged, this,
            &MesonBuildSystem::buildDirectoryChanged);
    connect(static_cast<MesonBuildConfiguration *>(bc),
            &MesonBuildConfiguration::parametersChanged, this, [this] {
        updateKit(kit());
        wipe();
    });
    connect(bc, &MesonBuildConfiguration::environmentChanged, this, [this] {
        m_parser.setEnvironment(buildConfiguration()->environment());
    });

    connect(project(), &ProjectExplorer::Project::projectFileIsDirty, this, [this] {
        if (buildConfiguration()->isActive())
            parseProject();
    });
    connect(&m_parser, &MesonProjectParser::parsingCompleted, this, &MesonBuildSystem::parsingCompleted);

    connect(&m_introWatcher, &FileSystemWatcher::fileChanged, this, [this] {
        if (buildConfiguration()->isActive())
            parseProject();
    });

    updateKit(kit());
}

MesonBuildSystem::~MesonBuildSystem()
{
    // Trigger any pending parsingFinished signals before destroying any other build system part:
    m_parseGuard = {};
    qCDebug(mesonBuildSystemLog) << "dtor";
}

void MesonBuildSystem::triggerParsing()
{
    qCDebug(mesonBuildSystemLog) << "Trigger parsing";
    parseProject();
}

static FilePaths linkingBinaries(const TargetsList &targets,
                                 const FilePath &buildDir,
                                 const FilePath &sourceFile)
{
    QHash<QString, const Target *> targetsById;
    QStringList pendingTargets;
    for (const Target &target : targets) {
        targetsById.insert(target.id, &target);
        const auto isSourceFile = [&sourceFile](const QString &source) {
            return FilePath::fromString(source) == sourceFile;
        };
        for (const Target::SourceGroup &group : target.sources) {
            if (Utils::contains(group.sources, isSourceFile)
                || Utils::contains(group.generatedSources, isSourceFile)) {
                pendingTargets << target.id;
            }
        }
    }

    FilePaths binaries;
    QSet<QString> seenTargets;
    while (!pendingTargets.isEmpty()) {
        const QString id = pendingTargets.takeLast();
        if (!Utils::insert(seenTargets, id))
            continue;
        const Target * const target = targetsById.value(id);
        if (!target || target->fileName.isEmpty())
            continue;
        const FilePath artifact = FilePath::fromString(target->fileName.first());
        if (target->type == Target::Type::executable
            || target->type == Target::Type::sharedLibrary
            || target->type == Target::Type::sharedModule) {
            binaries << artifact;
            continue;
        }
        if (target->type != Target::Type::staticLibrary)
            continue;
        // Code from a static library ends up in whatever links it. The linker
        // arguments are relative to the build directory.
        const FilePath artifactOnDevice = buildDir.withNewPath(artifact.path());
        const auto isArtifact = [&buildDir, &artifactOnDevice](const QString &linked) {
            return buildDir.resolvePath(linked) == artifactOnDevice;
        };
        for (const Target &other : targets) {
            if (Utils::anyOf(other.linkedFiles, isArtifact))
                pendingTargets << other.id;
        }
    }
    return binaries;
}

FilePaths MesonBuildSystem::binariesForSourceFile(const FilePath &sourceFile) const
{
    // Meson reports the artifacts under the build directory with symlinks resolved.
    return linkingBinaries(targets(), buildConfiguration()->buildDirectory().canonicalPath(),
                           sourceFile);
}

bool MesonBuildSystem::needsSetup()
{
    const FilePath buildDir = buildConfiguration()->buildDirectory();
    return !isSetup(buildDir) || !m_parser.usesSameMesonVersion(buildDir)
            || !m_parser.matchesKit(m_kitData);
}

void MesonBuildSystem::parsingCompleted(bool success)
{
    if (success) {
        setRootProjectNode(m_parser.takeProjectNode());
        if (kit() && buildConfiguration()) {
            KitInfo kitInfo{kit()};
            m_cppCodeModelUpdater->update(
                {project(),
                 QtSupport::CppKitInfo(kit()),
                 buildConfiguration()->environment(),
                 m_parser.buildProjectParts(buildConfiguration()->buildDirectory(),
                                            kitInfo.cxxToolchain, kitInfo.cToolchain)});
        }
        setApplicationTargets(m_parser.appsTargets());
        UNLOCK(true);
        emitBuildSystemUpdated();
    } else {
        TaskHub::addTask<BuildSystemTask>(Task::Error, Tr::tr("Meson build: Parsing failed"));
        UNLOCK(false);
        emitBuildSystemUpdated();
    }
    emitParsingFinished(success);

    emit buildConfiguration()->enabledChanged(); // HACK. Should not be needed.
}

QStringList MesonBuildSystem::configArgs(bool isSetup)
{
    MesonBuildConfiguration *bc = static_cast<MesonBuildConfiguration *>(buildConfiguration());

    const QString &params = bc->parameters();
    if (!isSetup || params.contains("--cross-file") || params.contains("--native-file"))
        return m_pendingConfigArgs + bc->mesonConfigArgs();

    return QStringList{QString("--native-file=%1").arg(machineFile(kit()).toUrlishString())}
           + m_pendingConfigArgs + bc->mesonConfigArgs();
}

void MesonBuildSystem::buildDirectoryChanged()
{
    updateKit(kit());

    m_introWatcher.clear();
    if (buildConfiguration()->isActive()) {
        // as specified here https://mesonbuild.com/IDE-integration.html#ide-integration
        // meson-info.json is the last written file, which ensure that all others introspection
        // files are ready when a modification is detected on this one.
        m_introWatcher.addFile(buildConfiguration()->buildDirectory()
                                   .pathAppended(Constants::MESON_INFO_DIR)
                                   .pathAppended(Constants::MESON_INFO),
                              FileSystemWatcher::WatchModifiedDate);
    }

    triggerParsing();
}

bool MesonBuildSystem::configure()
{
    LEAVE_IF_BUSY();
    qCDebug(mesonBuildSystemLog) << "Configure";
    if (needsSetup())
        return setup();
    LOCK();
    if (m_parser.configure(projectDirectory(),
                           buildConfiguration()->buildDirectory(),
                           configArgs(false))) {
        return true;
    }
    UNLOCK(false);
    return false;
}

bool MesonBuildSystem::setup()
{
    LEAVE_IF_BUSY();
    LOCK();
    qCDebug(mesonBuildSystemLog) << "Setup";
    if (m_parser.setup(projectDirectory(), buildConfiguration()->buildDirectory(), configArgs(true)))
        return true;
    UNLOCK(false);
    return false;
}

bool MesonBuildSystem::wipe()
{
    LEAVE_IF_BUSY();
    LOCK();
    qCDebug(mesonBuildSystemLog) << "Wipe";
    if (m_parser.wipe(projectDirectory(), buildConfiguration()->buildDirectory(), configArgs(true)))
        return true;
    UNLOCK(false);
    return false;
}

bool MesonBuildSystem::parseProject()
{
    QTC_ASSERT(buildConfiguration(), return false);
    if (!buildConfiguration()->isActive()) // Never parse if not active
        return false;
    if (!isSetup(buildConfiguration()->buildDirectory()) && settings().autorunMeson())
        return configure();
    LEAVE_IF_BUSY();
    LOCK();
    qCDebug(mesonBuildSystemLog) << "Starting parser";
    if (m_parser.parse(projectDirectory(), buildConfiguration()->buildDirectory()))
        return true;
    UNLOCK(false);
    return false;
}

void MesonBuildSystem::updateKit(ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return );
    m_kitData = createKitData(kit);
    m_parser.setQtVersion(m_kitData.qtVersion);
}

void setupMesonBuildSystem()
{
    static MachineFileManager theMachineFileManager;
}

} // MesonProjectManager::Internal

#ifdef WITH_TESTS

#include "mesoninfoparser.h"

#include <QTest>

namespace MesonProjectManager::Internal {

// What "meson introspect --all" reports for an application, two shared
// libraries and the static library both of them link, plus a static library
// of the same name in a subdirectory and the tool linking that one, and an
// MSVC-style library linking the first archive whole.
static const char introspectionJson[] = R"({
    "targets": [
        {
            "name": "core", "id": "core@sta", "type": "static library",
            "filename": ["/b/libcore.a"],
            "target_sources": [
                {"language": "cpp", "parameters": ["-I/b/libcore.a.p", "-g"],
                 "sources": ["/s/core.cpp"], "generated_sources": []},
                {"linker": ["gcc-ar"], "parameters": ["csrDT"]}
            ]
        },
        {
            "name": "alpha", "id": "alpha@sha", "type": "shared library",
            "filename": ["/b/libalpha.so"],
            "target_sources": [
                {"language": "cpp", "parameters": ["-I/b/libalpha.so.p", "-g"],
                 "sources": ["/s/alpha.cpp"], "generated_sources": []},
                {"linker": ["/usr/bin/g++"],
                 "parameters": ["-shared", "-Wl,-soname,libalpha.so", "libcore.a"]}
            ]
        },
        {
            "name": "beta", "id": "beta@sha", "type": "shared library",
            "filename": ["/b/libbeta.so"],
            "target_sources": [
                {"language": "cpp", "parameters": ["-I/b/libbeta.so.p", "-g"],
                 "sources": ["/s/beta.cpp"], "generated_sources": []},
                {"linker": ["/usr/bin/g++"],
                 "parameters": ["-shared", "-Wl,-soname,libbeta.so", "libcore.a"]}
            ]
        },
        {
            "name": "app", "id": "app@exe", "type": "executable",
            "filename": ["/b/app"],
            "target_sources": [
                {"language": "cpp", "parameters": ["-I/b/app.p", "-g"],
                 "sources": ["/s/main.cpp"], "generated_sources": []},
                {"linker": ["/usr/bin/g++"], "parameters": ["libalpha.so"]}
            ]
        },
        {
            "name": "core", "id": "sub@@core@sta", "type": "static library",
            "filename": ["/b/sub/libcore.a"],
            "target_sources": [
                {"language": "cpp", "parameters": ["-I/b/sub/libcore.a.p", "-g"],
                 "sources": ["/s/sub/core.cpp"], "generated_sources": []},
                {"linker": ["gcc-ar"], "parameters": ["csrDT"]}
            ]
        },
        {
            "name": "tool", "id": "tool@exe", "type": "executable",
            "filename": ["/b/tool"],
            "target_sources": [
                {"language": "cpp", "parameters": ["-I/b/tool.p", "-g"],
                 "sources": ["/s/tool.cpp"], "generated_sources": []},
                {"linker": ["/usr/bin/g++"], "parameters": ["sub/libcore.a"]}
            ]
        },
        {
            "name": "gamma", "id": "gamma@sha", "type": "shared library",
            "filename": ["/b/gamma.dll"],
            "target_sources": [
                {"language": "cpp", "parameters": ["/I/b/gamma.dll.p"],
                 "sources": ["/s/gamma.cpp"], "generated_sources": []},
                {"linker": ["link"], "parameters": ["/DLL", "/WHOLEARCHIVE:libcore.a"]}
            ]
        }
    ],
    "buildoptions": [],
    "projectinfo": {"buildsystem_files": ["/s/meson.build"], "subprojects": []}
})";

class MesonBuildSystemTest final : public QObject
{
    Q_OBJECT

private slots:
    void testBinariesForSourceFile()
    {
        const TargetsList targets = MesonInfoParser::parse(QByteArray(introspectionJson)).targets;
        QCOMPARE(targets.size(), 7u);
        const auto binaries = [&targets](const QString &source) {
            return Utils::sorted(
                linkingBinaries(targets, FilePath("/b"), FilePath::fromString(source)));
        };

        QCOMPARE(binaries("/s/main.cpp"), FilePaths{"/b/app"});
        QCOMPARE(binaries("/s/alpha.cpp"), FilePaths{"/b/libalpha.so"});

        // Code from a static library ends up in both libraries linking it, and
        // in neither the application behind them nor its own archive.
        QCOMPARE(binaries("/s/core.cpp"),
                 FilePaths({"/b/gamma.dll", "/b/libalpha.so", "/b/libbeta.so"}));

        // An archive of the same name elsewhere is a different library.
        QCOMPARE(binaries("/s/sub/core.cpp"), FilePaths{"/b/tool"});

        QCOMPARE(binaries("/s/unknown.cpp"), FilePaths());
    }
};

QObject *createMesonBuildSystemTest()
{
    return new MesonBuildSystemTest;
}

} // namespace MesonProjectManager::Internal

#include "mesonbuildsystem.moc"

#endif // WITH_TESTS
