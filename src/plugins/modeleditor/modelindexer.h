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

namespace ModelEditor {
namespace Internal {

class ModelIndexer :
        public QObject
{
    Q_OBJECT
    class QueuedFile;
    class IndexedModel;
    class IndexedDiagramReference;
    class IndexerThread;
    class DiagramsCollectorVisitor;
    class ModelIndexerPrivate;

    friend size_t qHash(const ModelIndexer::QueuedFile &queuedFile);
    friend bool operator==(const ModelIndexer::QueuedFile &lhs,
                           const ModelIndexer::QueuedFile &rhs);

public:
    ModelIndexer(QObject *parent = nullptr);
    ~ModelIndexer();

signals:
    void quitIndexerThread();
    void filesQueued();
    void openDefaultModel(const qmt::Uid &modelUid);

public:
    QString findModel(const qmt::Uid &modelUid);
    QString findDiagram(const qmt::Uid &modelUid, const qmt::Uid &diagramUid);

private:
    void onProjectAdded(ProjectExplorer::Project *project);
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onProjectFileListChanged(ProjectExplorer::Project *project);

private:
    void scanProject(ProjectExplorer::Project *project);
    QString findFirstModel(ProjectExplorer::FolderNode *folderNode,
                           const Utils::MimeType &mimeType);
    void forgetProject(ProjectExplorer::Project *project);
    void removeModelFile(const QString &file, ProjectExplorer::Project *project);
    void removeDiagramReferenceFile(const QString &file, ProjectExplorer::Project *project);

    static const QLoggingCategory &logger();

private:
    ModelIndexerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
