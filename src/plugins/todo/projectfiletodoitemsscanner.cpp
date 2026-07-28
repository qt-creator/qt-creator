// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectfiletodoitemsscanner.h"

#include <coreplugin/documentmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/filepath.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Todo {
namespace Internal {

ProjectFileTodoItemsScanner::ProjectFileTodoItemsScanner(const KeywordList &keywordList,
                                                         QObject *parent)
    : TodoItemsScanner(keywordList, parent)
{
    const auto watchProject = [this](Project *project) {
        connect(project, &Project::anyParsingFinished, this, [this, project] {
            scanProject(project);
        });
    };

    for (Project *project : ProjectManager::projects())
        watchProject(project);

    connect(ProjectManager::instance(), &ProjectManager::projectAdded,
            this, [this, watchProject](Project *project) {
        watchProject(project);
        scanProject(project);
    });

    // Editing and saving a project file does not necessarily trigger a reparse,
    // so also react to plain saves.
    connect(Core::DocumentManager::instance(), &Core::DocumentManager::filesChangedInternally,
            this, [this](const FilePaths &filePaths) {
        for (const FilePath &filePath : filePaths) {
            if (isProjectFile(filePath) && !ProjectManager::projectsForFile(filePath).isEmpty())
                processFile(filePath);
        }
    });

    setParams(keywordList);
}

void ProjectFileTodoItemsScanner::scannerParamsChanged()
{
    scanAllProjects();
}

void ProjectFileTodoItemsScanner::scanAllProjects()
{
    for (Project *project : ProjectManager::projects())
        scanProject(project);
}

void ProjectFileTodoItemsScanner::scanProject(Project *project)
{
    const FilePaths files = project->files(Project::SourceFiles);
    for (const FilePath &filePath : files) {
        if (isProjectFile(filePath))
            processFile(filePath);
    }
}

void ProjectFileTodoItemsScanner::processFile(const FilePath &filePath)
{
    const Result<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return;

    QList<TodoItem> itemList;

    // qmake and CMake both use '#' for a comment running to the end of the line.
    const QStringList lines = QString::fromUtf8(*contents).split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const int hashIndex = lines.at(i).indexOf('#');
        if (hashIndex == -1)
            continue;
        const QString comment = lines.at(i).mid(hashIndex + 1);
        processCommentLine(filePath.toUrlishString(), comment, i + 1, itemList);
    }

    emit itemsFetched(filePath.toUrlishString(), itemList);
}

bool ProjectFileTodoItemsScanner::isProjectFile(const FilePath &filePath)
{
    static const QStringList qmakeSuffixes = {"pro", "pri", "prf"};
    const QString suffix = filePath.suffix();
    if (qmakeSuffixes.contains(suffix))
        return true;
    return suffix == "cmake" || filePath.fileName() == "CMakeLists.txt";
}

} // namespace Internal
} // namespace Todo
