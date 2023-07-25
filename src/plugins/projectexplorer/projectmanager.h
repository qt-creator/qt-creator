// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <QString>
#include <QObject>

namespace Core { class IEditor; }

#include <functional>

namespace Utils {
class FilePath;
using FilePaths = QList<FilePath>;
class MimeType;
} // Utils

namespace ProjectExplorer {

class BuildSystem;
class Project;
class RunConfiguration;
class Target;

class PROJECTEXPLORER_EXPORT ProjectManager : public QObject
{
    Q_OBJECT

public:
    ProjectManager();
    ~ProjectManager() override;

    static ProjectManager *instance();

public:
    static bool canOpenProjectForMimeType(const Utils::MimeType &mt);
    static Project *openProject(const Utils::MimeType &mt, const Utils::FilePath &fileName);

    template <typename T>
    static void registerProjectType(const QString &mimeType)
    {
        ProjectManager::registerProjectCreator(mimeType, [](const Utils::FilePath &fileName) {
            return new T(fileName);
        });
    }

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

    static Project *startupProject();
    static Target *startupTarget();
    static BuildSystem *startupBuildSystem();
    static RunConfiguration *startupRunConfiguration();

    static const QList<Project *> projects();
    static bool hasProjects();
    static bool hasProject(Project *p);

    // NBS rewrite projectOrder (dependency management)
    static QList<Project *> projectOrder(const Project *project = nullptr);

    static Project *projectForFile(const Utils::FilePath &fileName);
    static Project *projectWithProjectFilePath(const Utils::FilePath &filePath);

    static Utils::FilePaths projectsForSessionName(const QString &session);

signals:
    void targetAdded(ProjectExplorer::Target *target);
    void targetRemoved(ProjectExplorer::Target *target);
    void projectAdded(ProjectExplorer::Project *project);
    void aboutToRemoveProject(ProjectExplorer::Project *project);
    void projectDisplayNameChanged(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);

    void startupProjectChanged(ProjectExplorer::Project *project);

    void dependencyChanged(ProjectExplorer::Project *a, ProjectExplorer::Project *b);

    // for tests only
    void projectFinishedParsing(ProjectExplorer::Project *project);

private:
    static void configureEditor(Core::IEditor *editor, const Utils::FilePath &filePath);
    static void configureEditors(Project *project);

    static void registerProjectCreator(const QString &mimeType,
                                       const std::function<Project *(const Utils::FilePath &)> &);
};

} // namespace ProjectExplorer
