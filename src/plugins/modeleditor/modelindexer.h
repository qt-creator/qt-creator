// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace qmt { class Uid; }

namespace ProjectExplorer {
class Project;
class FolderNode;
}

namespace Utils { class MimeType; }

namespace ModelEditor::Internal {

class QueuedFile;

class ModelIndexer : public QObject
{
    Q_OBJECT

public:
    ModelIndexer(QObject *parent = nullptr);
    ~ModelIndexer();

    QString findModel(const qmt::Uid &modelUid);
    QString findDiagram(const qmt::Uid &modelUid, const qmt::Uid &diagramUid);

signals:
    void openDefaultModel(const qmt::Uid &modelUid);

private:
    friend size_t qHash(const QueuedFile &queuedFile);
    friend bool operator==(const QueuedFile &lhs, const QueuedFile &rhs);

    void onProjectAdded(ProjectExplorer::Project *project);
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onProjectFileListChanged(ProjectExplorer::Project *project);

    void scanProject(ProjectExplorer::Project *project);
    QString findFirstModel(ProjectExplorer::FolderNode *folderNode, const Utils::MimeType &mimeType);
    void forgetProject(ProjectExplorer::Project *project);
    void removeModelFile(const QString &file, ProjectExplorer::Project *project);
    void removeDiagramReferenceFile(const QString &file, ProjectExplorer::Project *project);
    void handleQueuedFiles();

private:
    class ModelIndexerPrivate *d;
};

} // namespace ModelEditor::Internal
