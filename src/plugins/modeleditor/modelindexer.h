/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MODELINDEXER_H
#define MODELINDEXER_H

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

    friend uint qHash(const ModelIndexer::QueuedFile &queued_file);
    friend bool operator==(const ModelIndexer::QueuedFile &lhs,
                           const ModelIndexer::QueuedFile &rhs);

public:
    ModelIndexer(QObject *parent = 0);
    ~ModelIndexer();

signals:
    void quitIndexerThread();
    void filesQueued();
    void openDefaultModel(const qmt::Uid &model_uid);

public:
    QString findModel(const qmt::Uid &modelUid);
    QString findDiagram(const qmt::Uid &modelUid, const qmt::Uid &diagramUid);

private slots:
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

#endif // MODELINDEXER_H
