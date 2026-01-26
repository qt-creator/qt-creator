// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "task.h"

#include <utils/storekey.h>

#include <QString>
#include <QObject>

#include <functional>

namespace Utils {
class FilePath;
class FilePaths;
class MimeType;
} // Utils

namespace ProjectExplorer {

class BuildConfiguration;
class BuildSystem;
class QmlCodeModelInfo;
class Kit;
class Project;
class Target;

class CustomProjectSettingsHandler
{
public:
    using Loader = std::function<Utils::Result<QVariant>(const Utils::FilePath &)>;
    using Unloader = std::function<void(const QVariant &)>;

    CustomProjectSettingsHandler(
        const QString &fileName, const Loader &loader, const Unloader &unloader)
        : m_fileName(fileName)
        , m_loader(loader)
        , m_unloader(unloader)
    {}

    void load(Project &project) const;
    void unload(const Project &project) const;

private:
    Utils::Key m_key = Utils::Id::generate().toKey();
    QString m_fileName;
    Loader m_loader;
    Unloader m_unloader;
};

class PROJECTEXPLORER_EXPORT ProjectManager : public QObject
{
    Q_OBJECT

public:
    ProjectManager();
    ~ProjectManager() override;

    static ProjectManager *instance();

public:
    static bool canOpenProjectForMimeType(const Utils::MimeType &mt);
    static bool ensurePluginForProjectIsLoaded(const Utils::MimeType &mt);
    static Project *openProject(const Utils::MimeType &mt, const Utils::FilePath &fileName);

    using IssuesGenerator = std::function<Tasks(const Kit *)>;
    template<typename T>
    static void registerProjectType(const QString &mimeType,
                                    const IssuesGenerator &issuesGenerator = {})
    {
        registerProjectCreator(
            mimeType,
            [issuesGenerator](const Utils::FilePath &fileName) {
                auto * const p = new T(fileName);
                p->setIssuesGenerator(issuesGenerator);
                return p;
            },
            issuesGenerator);
    }
    static IssuesGenerator getIssuesGenerator(const Utils::FilePath &projectFilePath);

    static void registerCustomProjectSettingsHandler(const CustomProjectSettingsHandler &handler);
    static void loadCustomProjectSettings(Project &project);
    static void unloadCustomProjectSettings(const Project &project);

    static void closeAllProjects();

    static void addProject(Project *project);
    static void removeProject(Project *project);
    static void removeProjects(const QList<Project *> &remove);

    static void setStartupProject(Project *startupProject);

    static QList<Project *> dependencies(const Project *project);
    static bool hasDependency(const Project *project, const Project *depProject);
    static bool canAddDependency(const Project *project, const Project *depProject);
    static bool addDependency(Project *project, Project *depProject);
    static void removeDependency(Project *project, Project *depProject);

    static bool isProjectConfigurationCascading();
    static void setProjectConfigurationCascading(bool b);

    static bool deployProjectDependencies();
    static void setDeployProjectDependencies(bool deploy);

    static Project *startupProject();
    static Target *startupTarget();

    static const QList<Project *> projects();
    static bool hasProjects();
    static bool hasProject(Project *p);

    // NBS rewrite projectOrder (dependency management)
    static QList<Project *> projectOrder(const Project *project = nullptr);

    static Project *projectForFile(const Utils::FilePath &fileName);
    static QList<Project *> projectsForFile(const Utils::FilePath &fileName);
    static bool isInProjectBuildDir(const Utils::FilePath &filePath, const Project &project);
    static bool isInProjectSourceDir(const Utils::FilePath &filePath, const Project &project);
    static Project *projectWithProjectFilePath(const Utils::FilePath &filePath);
    static bool isKnownFile(const Utils::FilePath &filePath);

    static Utils::FilePaths projectsForSessionName(const QString &session);

    static bool isAnyProjectParsing();

signals:
    void projectAdded(ProjectExplorer::Project *project);
    void aboutToRemoveProject(ProjectExplorer::Project *project);
    void projectDisplayNameChanged(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);

    void startupProjectChanged(ProjectExplorer::Project *project);

    void buildConfigurationAdded(ProjectExplorer::BuildConfiguration *bc);
    void aboutToRemoveBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void buildConfigurationRemoved(ProjectExplorer::BuildConfiguration *bc);
    // bc == activeBuildConfigForActiveProject()
    void activeBuildConfigurationChanged(BuildConfiguration *bc);

    // bc == activeBuildConfigForCurrentProject()
    void currentBuildConfigurationChanged(BuildConfiguration *bc);

    // bs = activeBuildSystemForActiveProject()
    void parsingStartedActive(BuildSystem *bs);
    void parsingFinishedActive(bool success, BuildSystem *bs);

    // bs = activeBuildSystemForCurrentProject()
    void parsingStartedCurrent(BuildSystem *bs);
    void parsingFinishedCurrent(bool success, BuildSystem *bs);

    void dependencyChanged(ProjectExplorer::Project *a, ProjectExplorer::Project *b);

    void projectStartedParsing(ProjectExplorer::Project *project);
    void projectFinishedParsing(ProjectExplorer::Project *project);

    void extraProjectInfoChanged(ProjectExplorer::BuildConfiguration *bc,
                                 const ProjectExplorer::QmlCodeModelInfo &extra);
    void requestCodeModelReset();

private:
    static void registerProjectCreator(const QString &mimeType,
                                       const std::function<Project *(const Utils::FilePath &)> &,
                                       const IssuesGenerator &issuesGenerator);
};

namespace Internal { QObject *createSessionTest(); }

} // namespace ProjectExplorer
