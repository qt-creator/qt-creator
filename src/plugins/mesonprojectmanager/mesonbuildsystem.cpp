// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonbuildsystem.h"

#include "kithelper.h"
#include "machinefilemanager.h"
#include "mesonbuildconfiguration.h"
#include "mesonprojectmanagertr.h"
#include "mesontoolkitaspect.h"
#include "settings.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/taskhub.h>

#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitinformation.h>

#include <QDir>
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

namespace MesonProjectManager {
namespace Internal {
static Q_LOGGING_CATEGORY(mesonBuildSystemLog, "qtc.meson.buildsystem", QtWarningMsg);

MesonBuildSystem::MesonBuildSystem(MesonBuildConfiguration *bc)
    : ProjectExplorer::BuildSystem{bc}
    , m_parser{MesonToolKitAspect::mesonToolId(bc->kit()), bc->environment(), project()}
{
    init();
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
}

ProjectExplorer::Kit *MesonBuildSystem::MesonBuildSystem::kit()
{
    return buildConfiguration()->kit();
}

QStringList MesonBuildSystem::configArgs(bool isSetup)
{
    const QString &params = mesonBuildConfiguration()->parameters();
    if (!isSetup || params.contains("--cross-file") || params.contains("--native-file"))
        return m_pendingConfigArgs + mesonBuildConfiguration()->mesonConfigArgs();
    else {
        return QStringList{
                   QString("--native-file=%1").arg(MachineFileManager::machineFile(kit()).toString())}
               + m_pendingConfigArgs + mesonBuildConfiguration()->mesonConfigArgs();
    }
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

MesonBuildConfiguration *MesonBuildSystem::mesonBuildConfiguration()
{
    return static_cast<MesonBuildConfiguration *>(buildConfiguration());
}

void MesonBuildSystem::init()
{
    qCDebug(mesonBuildSystemLog) << "Init";
    connect(buildConfiguration()->target(), &ProjectExplorer::Target::kitChanged, this, [this] {
        updateKit(kit());
    });
    connect(mesonBuildConfiguration(), &MesonBuildConfiguration::buildDirectoryChanged, this, [this]() {
        updateKit(kit());
        this->triggerParsing();
    });
    connect(mesonBuildConfiguration(), &MesonBuildConfiguration::parametersChanged, this, [this]() {
        updateKit(kit());
        wipe();
    });
    connect(mesonBuildConfiguration(), &MesonBuildConfiguration::environmentChanged, this, [this]() {
        m_parser.setEnvironment(buildConfiguration()->environment());
    });

    connect(project(), &ProjectExplorer::Project::projectFileIsDirty, this, [this]() {
        if (buildConfiguration()->isActive())
            parseProject();
    });
    connect(&m_parser, &MesonProjectParser::parsingCompleted, this, &MesonBuildSystem::parsingCompleted);

    connect(&m_IntroWatcher, &Utils::FileSystemWatcher::fileChanged, this, [this]() {
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

bool MesonBuildSystem::parseProject()
{
    QTC_ASSERT(buildConfiguration(), return false);
    if (!isSetup(buildConfiguration()->buildDirectory()) && Settings::instance()->autorunMeson.value())
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
    m_kitData = KitHelper::kitData(kit);
    m_parser.setQtVersion(m_kitData.qtVersion);
}

} // namespace Internal
} // namespace MesonProjectManager
