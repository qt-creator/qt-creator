// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonbuildsystem.h"

#include "kitdata.h"
#include "mesonbuildconfiguration.h"
#include "mesonprojectmanagertr.h"
#include "mesontoolkitaspect.h"
#include "settings.h"

#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <optional>

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
    data.qtVersionStr = expander->expand(QString("%{Qt:Version}"));
    data.qtVersion = Utils::QtMajorVersion::None;
    auto version = Version::fromString(data.qtVersionStr);
    if (version.isValid) {
        switch (version.major) {
        case 4:
            data.qtVersion = Utils::QtMajorVersion::Qt4;
            break;
        case 5:
            data.qtVersion = Utils::QtMajorVersion::Qt5;
            break;
        case 6:
            data.qtVersion = Utils::QtMajorVersion::Qt6;
            break;
        default:
            data.qtVersion = Utils::QtMajorVersion::Unknown;
        }
    }
    return data;
}

static FilePath machineFilesDir()
{
    return Core::ICore::userResourcePath("Meson-machine-files");
}

FilePath MachineFileManager::machineFile(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    auto baseName
        = QString("%1%2%3").arg(MACHINE_FILE_PREFIX).arg(kit->id().toString()).arg(MACHINE_FILE_EXT);
    baseName = baseName.remove('{').remove('}');
    return machineFilesDir().pathAppended(baseName);
}

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
    if (kitData.qtVersion == QtMajorVersion::Qt4)
        ba += entry("qmake-qt4", kitData.qmakePath);
    else if (kitData.qtVersion == QtMajorVersion::Qt5)
        ba += entry("qmake-qt5", kitData.qmakePath);
    else if (kitData.qtVersion == QtMajorVersion::Qt6)
        ba += entry("qmake-qt6", kitData.qmakePath);
    ba += entry("cmake", kitData.cmakePath);

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

MesonBuildSystem::MesonBuildSystem(MesonBuildConfiguration *bc)
    : BuildSystem(bc)
    , m_parser(MesonToolKitAspect::mesonToolId(bc->kit()), bc->environment(), project())
{
    qCDebug(mesonBuildSystemLog) << "Init";
    connect(bc->target(), &ProjectExplorer::Target::kitChanged, this, [this] {
        updateKit(kit());
    });
    connect(bc, &MesonBuildConfiguration::buildDirectoryChanged, this, [this] {
        updateKit(kit());
        this->triggerParsing();
    });
    connect(bc, &MesonBuildConfiguration::parametersChanged, this, [this] {
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

    connect(&m_IntroWatcher, &Utils::FileSystemWatcher::fileChanged, this, [this] {
        if (buildConfiguration()->isActive())
            parseProject();
    });

    updateKit(kit());
    // as specified here https://mesonbuild.com/IDE-integration.html#ide-integration
    // meson-info.json is the last written file, which ensure that all others introspection
    // files are ready when a modification is detected on this one.
    m_IntroWatcher.addFile(buildConfiguration()
                               ->buildDirectory()
                               .pathAppended(Constants::MESON_INFO_DIR)
                               .pathAppended(Constants::MESON_INFO),
                           Utils::FileSystemWatcher::WatchModifiedDate);
}

MesonBuildSystem::~MesonBuildSystem()
{
    qCDebug(mesonBuildSystemLog) << "dtor";
}

void MesonBuildSystem::triggerParsing()
{
    qCDebug(mesonBuildSystemLog) << "Trigger parsing";
    parseProject();
}

bool MesonBuildSystem::needsSetup()
{
    const Utils::FilePath &buildDir = buildConfiguration()->buildDirectory();
    return (!isSetup(buildDir) || !m_parser.usesSameMesonVersion(buildDir)
            || !m_parser.matchesKit(m_kitData));
}

void MesonBuildSystem::parsingCompleted(bool success)
{
    if (success) {
        setRootProjectNode(m_parser.takeProjectNode());
        if (kit() && buildConfiguration()) {
            ProjectExplorer::KitInfo kitInfo{kit()};
            m_cppCodeModelUpdater.update(
                {project(),
                 QtSupport::CppKitInfo(kit()),
                 buildConfiguration()->environment(),
                 m_parser.buildProjectParts(kitInfo.cxxToolChain, kitInfo.cToolChain)});
        }
        setApplicationTargets(m_parser.appsTargets());
        UNLOCK(true);
        emitBuildSystemUpdated();
    } else {
        TaskHub::addTask(BuildSystemTask(Task::Error, Tr::tr("Meson build: Parsing failed")));
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

    return QStringList{
               QString("--native-file=%1").arg(MachineFileManager::machineFile(kit()).toString())}
           + m_pendingConfigArgs + bc->mesonConfigArgs();
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

} // MesonProjectManager::Internal
