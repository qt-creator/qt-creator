/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "mesonbuildsystem.h"
#include "mesonbuildconfiguration.h"
#include <kithelper/kithelper.h>
#include <machinefiles/machinefilemanager.h>
#include <settings/general/settings.h>
#include <settings/tools/kitaspect/mesontoolkitaspect.h>
#include <projectexplorer/buildconfiguration.h>

#include <QDir>
#include <QLoggingCategory>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitinformation.h>

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

namespace MesonProjectManager {
namespace Internal {
static Q_LOGGING_CATEGORY(mesonBuildSystemLog, "qtc.meson.buildsystem", QtDebugMsg);

MesonBuildSystem::MesonBuildSystem(MesonBuildConfiguration *bc)
    : ProjectExplorer::BuildSystem{bc}
    , m_parser{MesonToolKitAspect::mesonToolId(bc->target()->kit()),
               bc->environment(),
               project()}
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
        UNLOCK(false);
        emitBuildSystemUpdated();
    }
}

ProjectExplorer::Kit *MesonBuildSystem::MesonBuildSystem::kit()
{
    return buildConfiguration()->target()->kit();
}

QStringList MesonBuildSystem::configArgs(bool isSetup)
{
    if (!isSetup)
        return m_pendingConfigArgs + mesonBuildConfiguration()->mesonConfigArgs();
    else {
        return QStringList{QString("--native-file=%1")
                               .arg(MachineFileManager::machineFile(kit()).toString())}
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
    connect(mesonBuildConfiguration(),
            &MesonBuildConfiguration::buildDirectoryChanged,
            this,
            [this]() {
                updateKit(kit());
                this->triggerParsing();
            });
    connect(mesonBuildConfiguration(), &MesonBuildConfiguration::environmentChanged, this, [this]() {
        m_parser.setEnvironment(buildConfiguration()->environment());
    });

    connect(project(), &ProjectExplorer::Project::projectFileIsDirty, this, [this]() {
        if (buildConfiguration()->isActive())
            parseProject();
    });
    connect(&m_parser,
            &MesonProjectParser::parsingCompleted,
            this,
            &MesonBuildSystem::parsingCompleted);
    updateKit(kit());
}

bool MesonBuildSystem::parseProject()
{
    QTC_ASSERT(buildConfiguration(), return false);
    if (!isSetup(buildConfiguration()->buildDirectory()) && Settings::autorunMeson())
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
