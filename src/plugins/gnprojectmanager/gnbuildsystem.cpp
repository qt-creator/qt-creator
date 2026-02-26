// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnbuildsystem.h"

#include "gnbuildconfiguration.h"
#include "gnkitaspect.h"
#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"
#include "gntools.h"

#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectupdater.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/qtcassert.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace GNProjectManager::Internal {

GNBuildSystem::GNBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
    , m_parser(project())
    , m_cppCodeModelUpdater(ProjectUpdaterFactory::createCppProjectUpdater())
{
    connect(bc, &BuildConfiguration::buildDirectoryChanged, this, &GNBuildSystem::triggerParsing);

    connect(bc, &BuildConfiguration::environmentChanged, this, &GNBuildSystem::triggerParsing);

    connect(project(),
            &ProjectExplorer::Project::projectFileIsDirty,
            this,
            &GNBuildSystem::triggerParsing);

    connect(&m_argsWatcher, &FileSystemWatcher::fileChanged, this, &GNBuildSystem::triggerParsing);

    connect(&m_parser, &GNProjectParser::started, this, [this] {
        m_argsWatcher.clear();
        m_parseGuard = guardParsingRun();
    });
    connect(&m_parser, &GNProjectParser::completed, this, [this](bool success) {
        parsingCompleted(success);
    });
}

GNBuildSystem::~GNBuildSystem()
{
    m_parseGuard = {};
}

void GNBuildSystem::triggerParsing()
{
    QTC_ASSERT(buildConfiguration(), return);
    if (!buildConfiguration()->isActive())
        return;

    if (m_parseGuard.guardsProject())
        return;

    generate();
}

void GNBuildSystem::parsingCompleted(bool success)
{
    if (success) {
        setRootProjectNode(m_parser.takeProjectNode());
        if (kit() && buildConfiguration()) {
            KitInfo kitInfo{kit()};
            m_cppCodeModelUpdater->update(
                {project(),
                 QtSupport::CppKitInfo(kit()),
                 buildConfiguration()->environment(),
                 m_parser.buildProjectParts(kitInfo.cxxToolchain, kitInfo.cToolchain)});
        }
        setApplicationTargets(m_parser.appsTargets());
        const FilePaths bsFiles = m_parser.buildSystemFiles();
        QSet<FilePath> projectFiles(bsFiles.begin(), bsFiles.end());
        const auto args = argsPath();
        projectFiles.remove(argsPath());
        project()->setExtraProjectFiles(projectFiles);
        m_argsWatcher.addFile(args, FileSystemWatcher::WatchMode::WatchAllChanges);
        m_parseGuard.markAsSuccess();
        emitBuildSystemUpdated();
    } else {
        TaskHub::addTask<BuildSystemTask>(Task::Error, Tr::tr("GN build: Parsing failed"));
        emitBuildSystemUpdated();
    }
    emit buildConfiguration()->enabledChanged();
    m_parseGuard = {};
}

bool GNBuildSystem::generate()
{
    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    project()->clearIssues();

    auto *bc = static_cast<GNBuildConfiguration *>(buildConfiguration());

    // Try kit's GN tool first, then fall back to PATH lookup
    FilePath gnExe;
    if (auto tool = GNKitAspect::gnTool(kit()))
        gnExe = tool->exe();
    if (gnExe.isEmpty())
        gnExe = bc->gnExecutable();

    if (gnExe.isEmpty()) {
        TaskHub::addTask<BuildSystemTask>(Task::Error, Tr::tr("No GN executable configured."));
        return false;
    }

    if (m_parser.generate(gnExe,
                          projectDirectory(),
                          buildConfiguration()->buildDirectory(),
                          buildConfiguration()->environment(),
                          bc->gnGenArgs())) {
        return true;
    }
    return false;
}

FilePath GNBuildSystem::argsPath() const
{
    return buildConfiguration()->buildDirectory().pathAppended(Constants::PROJECT_AGS);
}

GNProject *GNBuildSystem::project() const
{
    return static_cast<GNProject *>(ProjectExplorer::BuildSystem::project());
}

} // namespace GNProjectManager::Internal
