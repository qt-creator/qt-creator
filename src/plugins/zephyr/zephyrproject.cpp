// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrproject.h"

#include "zephyrconstants.h"
#include "zephyrtr.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Zephyr::Internal {

const QLatin1StringView WEST_MANIFEST_MIMETYPE{"text/x-west-manifest"};

// ZephyrBuildSystem

class ZephyrBuildSystem final : public BuildSystem
{
public:
    explicit ZephyrBuildSystem(BuildConfiguration *bc)
        : BuildSystem(bc)
    {
        connect(project(), &Project::projectFileIsDirty, this, [this] {
            if (target()->activeBuildConfiguration() == buildConfiguration())
                requestDelayedParse();
        });
        requestDelayedParse();
    }

    static QString name() { return "zephyr"; }

    void triggerParsing() final
    {
        ParseGuard guard = guardParsingRun();
        const FilePath root = projectDirectory();

        static const QStringList skipDirs = {".git", ".west", "build"};

        FilePaths files;
        root.iterateDirectory(
            [&](const FilePath &path) {
                const QString pathStr = path.path();
                for (const QString &skip : skipDirs) {
                    if (pathStr.contains('/' + skip + '/') || pathStr.endsWith('/' + skip))
                        return IterationPolicy::Continue;
                }
                files.append(path);
                return IterationPolicy::Continue;
            },
            {{"*.c", "*.h", "*.cpp", "*.hpp", "*.cmake", "*.conf", "*.yml", "*.yaml",
              "CMakeLists.txt"},
             QDir::Files,
             QDirIterator::Subdirectories});

        auto newRoot = std::make_unique<ProjectNode>(root);
        for (const FilePath &f : files)
            newRoot->addNestedNode(std::make_unique<FileNode>(f, FileNode::fileTypeForFileName(f)));
        setRootProjectNode(std::move(newRoot));

        guard.markAsSuccess();
        emitBuildSystemUpdated();
    }
};

// ZephyrBuildConfiguration

class ZephyrBuildConfiguration final : public BuildConfiguration
{
public:
    ZephyrBuildConfiguration(Target *target, Id id)
        : BuildConfiguration(target, id)
    {
        setBuildDirectoryHistoryCompleter("Zephyr.BuildDir.History");
        setConfigWidgetDisplayName(Tr::tr("Zephyr"));
        appendInitialBuildStep(Constants::WEST_BUILD_STEP_ID);
    }
};

class ZephyrBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    ZephyrBuildConfigurationFactory()
    {
        registerBuildConfiguration<ZephyrBuildConfiguration>("Zephyr.ZephyrBuildConfiguration");
        setSupportedProjectType(Constants::ZEPHYR_PROJECT_ID);
        setSupportedProjectMimeTypeName(WEST_MANIFEST_MIMETYPE);
        setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
            BuildInfo info;
            info.typeName = Tr::tr("Build");
            info.buildDirectory = forSetup ? projectPath.parentDir() / "build" : projectPath;
            info.buildSystemName = "zephyr";
            if (forSetup)
                info.displayName = Tr::tr("Default");
            return QList<BuildInfo>{info};
        });
    }
};

// ZephyrProject

class ZephyrProject final : public Project
{
public:
    explicit ZephyrProject(const FilePath &file)
        : Project(WEST_MANIFEST_MIMETYPE, file)
    {
        setType(Constants::ZEPHYR_PROJECT_ID);
        setProjectLanguages(Core::Context(ProjectExplorer::Constants::C_LANGUAGE_ID));
        setDisplayName(projectDirectory().fileName());
        setBuildSystemCreator<ZephyrBuildSystem>();
    }
};

void setupZephyrProject()
{
    static ZephyrBuildConfigurationFactory theBuildConfigurationFactory;
    ProjectManager::registerProjectType<ZephyrProject>(WEST_MANIFEST_MIMETYPE);
}

} // namespace Zephyr::Internal
