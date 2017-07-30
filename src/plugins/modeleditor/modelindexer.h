/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#pragma once

#include <QObject>

namespace qmt { class Uid; }

namespace ProjectExplorer {
class Project;
class FolderNode;
}

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

    friend uint qHash(const ModelIndexer::QueuedFile &queuedFile);
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
    QString findFirstModel(ProjectExplorer::FolderNode *folderNode);
    void forgetProject(ProjectExplorer::Project *project);
    void removeModelFile(const QString &file, ProjectExplorer::Project *project);
    void removeDiagramReferenceFile(const QString &file, ProjectExplorer::Project *project);

    static const QLoggingCategory &logger();

private:
    ModelIndexerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
