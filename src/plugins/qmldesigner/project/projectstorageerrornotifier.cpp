// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorageerrornotifier.h"

#include <qmldesigner/qmldesignertr.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <sourcepathstorage/sourcepathcache.h>

namespace QmlDesigner {

namespace {

void logIssue(ProjectExplorer::Task::TaskType type, const QString &message, const SourcePath &sourcePath)
{
    const Utils::Id category = ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM;

    Utils::FilePath filePath = Utils::FilePath::fromUserInput(sourcePath.toQString());
    ProjectExplorer::Task task(type, message, filePath, -1, category);
    ProjectExplorer::TaskHub::addTask(task);
    ProjectExplorer::TaskHub::requestPopup();
}

void logIssue(ProjectExplorer::Task::TaskType type, const QString &message, QStringView sourcePath)
{
    const Utils::Id category = ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM;

    Utils::FilePath filePath = Utils::FilePath::fromPathPart(sourcePath);
    ProjectExplorer::Task task(type, message, filePath, -1, category);
    ProjectExplorer::TaskHub::addTask(task);
    ProjectExplorer::TaskHub::requestPopup();
}

} // namespace

void ProjectStorageErrorNotifier::typeNameCannotBeResolved(Utils::SmallStringView typeName,
                                                           SourceId sourceId)
{
    const QString typeNameString{typeName};

    logIssue(ProjectExplorer::Task::Warning,
             Tr::tr("Missing type %1 name.").arg(typeNameString),
             m_pathCache.sourcePath(sourceId));
}

void ProjectStorageErrorNotifier::missingDefaultProperty(Utils::SmallStringView typeName,
                                                         Utils::SmallStringView propertyName,
                                                         SourceId sourceId)

{
    const QString typeNameString{typeName};
    const QString propertyNameString{propertyName};

    logIssue(ProjectExplorer::Task::Warning,
             Tr::tr("Missing default property: %1 in type %2.").arg(propertyNameString).arg(typeNameString),
             m_pathCache.sourcePath(sourceId));
}

void ProjectStorageErrorNotifier::propertyNameDoesNotExists(Utils::SmallStringView propertyName,
                                                            SourceId sourceId)
{
    const QString propertyNameString{propertyName};

    logIssue(ProjectExplorer::Task::Warning,
             Tr::tr("Missing property %1 in type %2.").arg(propertyNameString),
             m_pathCache.sourcePath(sourceId));
}

void ProjectStorageErrorNotifier::qmlDocumentDoesNotExistsForQmldirEntry(Utils::SmallStringView typeName,
                                                                         Storage::Version,
                                                                         SourceId qmlDocumentSourceId,
                                                                         SourceId qmldirSourceId)
{
    const QString typeNameString{typeName};
    const QString missingPath = m_pathCache.sourcePath(qmlDocumentSourceId).toQString();
    logIssue(ProjectExplorer::Task::Warning,
             Tr::tr("Not existing Qml Document %1 for type %2.").arg(missingPath).arg(typeNameString),
             m_pathCache.sourcePath(qmldirSourceId));
}

void ProjectStorageErrorNotifier::qmltypesFileMissing(QStringView qmltypesPath)
{
    logIssue(ProjectExplorer::Task::Warning,
             Tr::tr("Not existing Qmltypes File %1.").arg(qmltypesPath),
             qmltypesPath);
}

} // namespace QmlDesigner
