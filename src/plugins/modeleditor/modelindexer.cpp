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

#include "modelindexer.h"

#include "modeleditor_constants.h"

#include "qmt/infrastructure/uid.h"

#include "qmt/serializer/projectserializer.h"

#include "qmt/project/project.h"
#include "qmt/model_controller/mvoidvisitor.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mdiagram.h"

#include "qmt/tasks/findrootdiagramvisitor.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/projectnodes.h>

#include <utils/mimetypes/mimetype.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QQueue>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QDateTime>

#include <QLoggingCategory>
#include <QDebug>

namespace ModelEditor {
namespace Internal {

class ModelIndexer::QueuedFile
{
    friend uint qHash(const ModelIndexer::QueuedFile &queuedFile);
    friend bool operator==(const ModelIndexer::QueuedFile &lhs,
                           const ModelIndexer::QueuedFile &rhs);

public:
    QueuedFile() = default;

    QueuedFile(const QString &file, ProjectExplorer::Project *project)
        : m_file(file),
          m_project(project)
    {
    }

    QueuedFile(const QString &file, ProjectExplorer::Project *project,
               const QDateTime &lastModified)
        : m_file(file),
          m_project(project),
          m_lastModified(lastModified)
    {
    }

    bool isValid() const { return !m_file.isEmpty() && m_project; }
    QString file() const { return m_file; }
    ProjectExplorer::Project *project() const { return m_project; }
    QDateTime lastModified() const { return m_lastModified; }

private:
    QString m_file;
    ProjectExplorer::Project *m_project = 0;
    QDateTime m_lastModified;
};

bool operator==(const ModelIndexer::QueuedFile &lhs, const ModelIndexer::QueuedFile &rhs)
{
    return lhs.m_file == rhs.m_file && lhs.m_project == rhs.m_project;
}

uint qHash(const ModelIndexer::QueuedFile &queuedFile)
{
    return qHash(queuedFile.m_project) + qHash(queuedFile.m_project);
}

class ModelIndexer::IndexedModel
{
public:
    IndexedModel(const QString &modelFile, const QDateTime &lastModified)
        : m_modelFile(modelFile),
          m_lastModified(lastModified)
    {
    }

    void reset(const QDateTime &lastModified)
    {
        m_lastModified = lastModified;
        m_modelUid = qmt::Uid::invalidUid();
        m_diagrams.clear();
    }

    QString file() const { return m_modelFile; }
    QDateTime lastModified() const { return m_lastModified; }
    QSet<ProjectExplorer::Project *> owningProjects() const { return m_owningProjects; }
    void addOwningProject(ProjectExplorer::Project *project) { m_owningProjects.insert(project); }
    void removeOwningProject(ProjectExplorer::Project *project)
    {
        m_owningProjects.remove(project);
    }
    qmt::Uid modelUid() const { return m_modelUid; }
    void setModelUid(const qmt::Uid &modelUid) { m_modelUid = modelUid; }
    QSet<qmt::Uid> diagrams() const { return m_diagrams; }
    void addDiagram(const qmt::Uid &diagram) { m_diagrams.insert(diagram); }

private:
    QString m_modelFile;
    QDateTime m_lastModified;
    QSet<ProjectExplorer::Project *> m_owningProjects;
    qmt::Uid m_modelUid;
    QSet<qmt::Uid> m_diagrams;
};

class ModelIndexer::IndexedDiagramReference
{
public:
    IndexedDiagramReference(const QString &file, const QDateTime &lastModified)
        : m_file(file),
          m_lastModified(lastModified)
    {
    }

    void reset(const QDateTime &lastModified)
    {
        m_lastModified = lastModified;
        m_modelUid = qmt::Uid::invalidUid();
        m_diagramUid = qmt::Uid::invalidUid();
    }

    QString file() const { return m_file; }
    QDateTime lastModified() const { return m_lastModified; }
    QSet<ProjectExplorer::Project *> owningProjects() const { return m_owningProjects; }
    void addOwningProject(ProjectExplorer::Project *project) { m_owningProjects.insert(project); }
    void removeOwningProject(ProjectExplorer::Project *project)
    {
        m_owningProjects.remove(project);
    }
    qmt::Uid modelUid() const { return m_modelUid; }
    void setModelUid(const qmt::Uid &modelUid) { m_modelUid = modelUid; }
    qmt::Uid diagramUid() const { return m_diagramUid; }
    void setDiagramUid(const qmt::Uid &diagramUid) { m_diagramUid = diagramUid; }

private:
    QString m_file;
    QDateTime m_lastModified;
    QSet<ProjectExplorer::Project *> m_owningProjects;
    qmt::Uid m_modelUid;
    qmt::Uid m_diagramUid;
};

class ModelIndexer::IndexerThread :
        public QThread
{
public:
    IndexerThread(ModelIndexer *indexer)
        : QThread(),
          m_indexer(indexer)
    {
    }

    void onQuitIndexerThread();
    void onFilesQueued();

private:
    ModelIndexer *m_indexer;
};

class ModelIndexer::DiagramsCollectorVisitor :
        public qmt::MVoidConstVisitor
{
public:
    DiagramsCollectorVisitor(ModelIndexer::IndexedModel *indexedModel);

    void visitMObject(const qmt::MObject *object);
    void visitMDiagram(const qmt::MDiagram *diagram);

private:
    ModelIndexer::IndexedModel *m_indexedModel;
};

ModelIndexer::DiagramsCollectorVisitor::DiagramsCollectorVisitor(IndexedModel *indexedModel)
    : qmt::MVoidConstVisitor(),
      m_indexedModel(indexedModel)
{
}

void ModelIndexer::DiagramsCollectorVisitor::visitMObject(const qmt::MObject *object)
{
    foreach (const qmt::Handle<qmt::MObject> &child, object->children()) {
        if (child.hasTarget())
            child.target()->accept(this);
    }
    visitMElement(object);
}

void ModelIndexer::DiagramsCollectorVisitor::visitMDiagram(const qmt::MDiagram *diagram)
{
    qCDebug(logger) << "add diagram " << diagram->name() << " to index";
    m_indexedModel->addDiagram(diagram->uid());
    visitMObject(diagram);
}

class ModelIndexer::ModelIndexerPrivate
{
public:
    ~ModelIndexerPrivate()
    {
        QTC_CHECK(filesQueue.isEmpty());
        QTC_CHECK(queuedFilesSet.isEmpty());
        QTC_CHECK(indexedModels.isEmpty());
        QTC_CHECK(indexedModelsByUid.isEmpty());
        QTC_CHECK(indexedDiagramReferences.isEmpty());
        QTC_CHECK(indexedDiagramReferencesByDiagramUid.isEmpty());
        delete indexerThread;
    }

    QMutex indexerMutex;

    QQueue<ModelIndexer::QueuedFile> filesQueue;
    QSet<ModelIndexer::QueuedFile> queuedFilesSet;
    QSet<ModelIndexer::QueuedFile> defaultModelFiles;

    QHash<QString, ModelIndexer::IndexedModel *> indexedModels;
    QHash<qmt::Uid, QSet<ModelIndexer::IndexedModel *> > indexedModelsByUid;

    QHash<QString, ModelIndexer::IndexedDiagramReference *> indexedDiagramReferences;
    QHash<qmt::Uid, QSet<ModelIndexer::IndexedDiagramReference *> > indexedDiagramReferencesByDiagramUid;

    ModelIndexer::IndexerThread *indexerThread = 0;
};

void ModelIndexer::IndexerThread::onQuitIndexerThread()
{
    QThread::exit(0);
}

void ModelIndexer::IndexerThread::onFilesQueued()
{
    QMutexLocker locker(&m_indexer->d->indexerMutex);

    while (!m_indexer->d->filesQueue.isEmpty()) {
        ModelIndexer::QueuedFile queuedFile = m_indexer->d->filesQueue.takeFirst();
        m_indexer->d->queuedFilesSet.remove(queuedFile);
        qCDebug(logger) << "handle queued file " << queuedFile.file()
                        << "from project " << queuedFile.project()->displayName();

        bool scanModel = false;
        IndexedModel *indexedModel = m_indexer->d->indexedModels.value(queuedFile.file());
        if (!indexedModel) {
            qCDebug(logger) << "create new indexed model";
            indexedModel = new IndexedModel(queuedFile.file(),
                                            queuedFile.lastModified());
            indexedModel->addOwningProject(queuedFile.project());
            m_indexer->d->indexedModels.insert(queuedFile.file(), indexedModel);
            scanModel = true;
        } else if (queuedFile.lastModified() > indexedModel->lastModified()) {
            qCDebug(logger) << "update indexed model";
            indexedModel->addOwningProject(queuedFile.project());
            indexedModel->reset(queuedFile.lastModified());
            scanModel = true;
        }
        if (scanModel) {
            locker.unlock();
            // load model file
            qmt::ProjectSerializer projectSerializer;
            qmt::Project project;
            projectSerializer.load(queuedFile.file(), &project);
            locker.relock();
            indexedModel->setModelUid(project.uid());
            // add indexedModel to set of indexedModelsByUid
            QSet<IndexedModel *> indexedModels = m_indexer->d->indexedModelsByUid.value(project.uid());
            indexedModels.insert(indexedModel);
            m_indexer->d->indexedModelsByUid.insert(project.uid(), indexedModels);
            // collect all diagrams of model
            DiagramsCollectorVisitor visitor(indexedModel);
            project.rootPackage()->accept(&visitor);
            if (m_indexer->d->defaultModelFiles.contains(queuedFile)) {
                m_indexer->d->defaultModelFiles.remove(queuedFile);
                // check if model has a diagram which could be opened
                qmt::FindRootDiagramVisitor diagramVisitor;
                project.rootPackage()->accept(&diagramVisitor);
                if (diagramVisitor.diagram())
                    emit m_indexer->openDefaultModel(project.uid());
            }
        }
    }
}

ModelIndexer::ModelIndexer(QObject *parent)
    : QObject(parent),
      d(new ModelIndexerPrivate())
{
    d->indexerThread = new IndexerThread(this);
    connect(this, &ModelIndexer::quitIndexerThread,
            d->indexerThread, &ModelIndexer::IndexerThread::onQuitIndexerThread);
    connect(this, &ModelIndexer::filesQueued,
            d->indexerThread, &ModelIndexer::IndexerThread::onFilesQueued);
    d->indexerThread->start();
    connect(ProjectExplorer::SessionManager::instance(), &ProjectExplorer::SessionManager::projectAdded,
            this, &ModelIndexer::onProjectAdded);
    connect(ProjectExplorer::SessionManager::instance(), &ProjectExplorer::SessionManager::aboutToRemoveProject,
            this, &ModelIndexer::onAboutToRemoveProject);
}

ModelIndexer::~ModelIndexer()
{
    emit quitIndexerThread();
    d->indexerThread->wait();
    delete d;
}

QString ModelIndexer::findModel(const qmt::Uid &modelUid)
{
    QMutexLocker locker(&d->indexerMutex);
    QSet<IndexedModel *> indexedModels = d->indexedModelsByUid.value(modelUid);
    if (indexedModels.isEmpty())
        return QString();
    IndexedModel *indexedModel = *indexedModels.cbegin();
    QTC_ASSERT(indexedModel, return QString());
    return indexedModel->file();
}

QString ModelIndexer::findDiagram(const qmt::Uid &modelUid, const qmt::Uid &diagramUid)
{
    Q_UNUSED(modelUid); // avoid warning in release mode

    QMutexLocker locker(&d->indexerMutex);
    QSet<IndexedDiagramReference *> indexedDiagramReferences = d->indexedDiagramReferencesByDiagramUid.value(diagramUid);
    if (indexedDiagramReferences.isEmpty())
        return QString();
    IndexedDiagramReference *indexedDiagramReference = *indexedDiagramReferences.cbegin();
    QTC_ASSERT(indexedDiagramReference, return QString());
    QTC_ASSERT(indexedDiagramReference->modelUid() == modelUid, return QString());
    return indexedDiagramReference->file();
}

void ModelIndexer::onProjectAdded(ProjectExplorer::Project *project)
{
    connect(project, &ProjectExplorer::Project::fileListChanged,
            this, [=]() { this->onProjectFileListChanged(project); });
    scanProject(project);
}

void ModelIndexer::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    disconnect(project, &ProjectExplorer::Project::fileListChanged, this, 0);
    forgetProject(project);
}

void ModelIndexer::onProjectFileListChanged(ProjectExplorer::Project *project)
{
    scanProject(project);
}

void ModelIndexer::scanProject(ProjectExplorer::Project *project)
{
    if (!project->rootProjectNode())
        return;

    // TODO harmonize following code with findFirstModel()?
    QStringList files = project->files(ProjectExplorer::Project::SourceFiles);
    QQueue<QueuedFile> filesQueue;
    QSet<QueuedFile> filesSet;

    foreach (const QString &file, files) {
        QFileInfo fileInfo(file);
        Utils::MimeType mimeType = Utils::mimeTypeForFile(fileInfo);
        if (mimeType.name() == QLatin1String(Constants::MIME_TYPE_MODEL)) {
            QueuedFile queuedFile(file, project, fileInfo.lastModified());
            filesQueue.append(queuedFile);
            filesSet.insert(queuedFile);
        }
    }

    QString defaultModelFile = findFirstModel(project->rootProjectNode());

    bool filesAreQueued = false;
    {
        QMutexLocker locker(&d->indexerMutex);

        // remove deleted files from queue
        for (int i = 0; i < d->filesQueue.size();) {
            if (d->filesQueue.at(i).project() == project) {
                if (filesSet.contains(d->filesQueue.at(i))) {
                    ++i;
                } else {
                    d->queuedFilesSet.remove(d->filesQueue.at(i));
                    d->filesQueue.removeAt(i);
                }
            }
        }

        // remove deleted files from indexed models
        foreach (const QString &file, d->indexedModels.keys()) {
            if (!filesSet.contains(QueuedFile(file, project)))
                removeModelFile(file, project);
        }

        // remove deleted files from indexed diagrams
        foreach (const QString &file, d->indexedDiagramReferences.keys()) {
            if (!filesSet.contains(QueuedFile(file, project)))
                removeDiagramReferenceFile(file, project);
        }

        // queue files
        while (!filesQueue.isEmpty()) {
            QueuedFile queuedFile = filesQueue.takeFirst();
            if (!d->queuedFilesSet.contains(queuedFile)) {
                QTC_CHECK(!d->filesQueue.contains(queuedFile));
                d->filesQueue.append(queuedFile);
                d->queuedFilesSet.insert(queuedFile);
                filesAreQueued = true;
            }
        }

        // auto-open model file only if project is already configured
        if (!defaultModelFile.isEmpty() && !project->targets().isEmpty()) {
            d->defaultModelFiles.insert(QueuedFile(defaultModelFile, project, QDateTime()));
        }
    }

    if (filesAreQueued)
        emit filesQueued();
}

QString ModelIndexer::findFirstModel(ProjectExplorer::FolderNode *folderNode)
{
    foreach (ProjectExplorer::FileNode *fileNode, folderNode->fileNodes()) {
        Utils::MimeType mimeType = Utils::mimeTypeForFile(fileNode->filePath().toFileInfo());
        if (mimeType.name() == QLatin1String(Constants::MIME_TYPE_MODEL))
            return fileNode->filePath().toString();
    }
    foreach (ProjectExplorer::FolderNode *subFolderNode, folderNode->folderNodes()) {
        QString modelFileName = findFirstModel(subFolderNode);
        if (!modelFileName.isEmpty())
            return modelFileName;
    }
    return QString();
}

void ModelIndexer::forgetProject(ProjectExplorer::Project *project)
{
    QStringList files = project->files(ProjectExplorer::Project::SourceFiles);

    QMutexLocker locker(&d->indexerMutex);
    foreach (const QString &file, files) {
        // remove file from queue
        QueuedFile queuedFile(file, project);
        if (d->queuedFilesSet.contains(queuedFile)) {
            QTC_CHECK(d->filesQueue.contains(queuedFile));
            d->filesQueue.removeOne(queuedFile);
            QTC_CHECK(!d->filesQueue.contains(queuedFile));
            d->queuedFilesSet.remove(queuedFile);
        }
        removeModelFile(file, project);
        removeDiagramReferenceFile(file, project);
    }
}

void ModelIndexer::removeModelFile(const QString &file, ProjectExplorer::Project *project)
{
    IndexedModel *indexedModel = d->indexedModels.value(file);
    if (indexedModel && indexedModel->owningProjects().contains(project)) {
        qCDebug(logger) << "remove model file " << file
                        << " from project " << project->displayName();
        indexedModel->removeOwningProject(project);
        if (indexedModel->owningProjects().isEmpty()) {
            qCDebug(logger) << "delete indexed model " << project->displayName();
            d->indexedModels.remove(file);

            // remove indexedModel from set of indexedModelsByUid
            QTC_CHECK(d->indexedModelsByUid.contains(indexedModel->modelUid()));
            QSet<IndexedModel *> indexedModels = d->indexedModelsByUid.value(indexedModel->modelUid());
            QTC_CHECK(indexedModels.contains(indexedModel));
            indexedModels.remove(indexedModel);
            if (indexedModels.isEmpty())
                d->indexedModelsByUid.remove(indexedModel->modelUid());
            else
                d->indexedModelsByUid.insert(indexedModel->modelUid(), indexedModels);

            delete indexedModel;
        }
    }
}

void ModelIndexer::removeDiagramReferenceFile(const QString &file,
                                              ProjectExplorer::Project *project)
{
    IndexedDiagramReference *indexedDiagramReference = d->indexedDiagramReferences.value(file);
    if (indexedDiagramReference) {
        QTC_CHECK(indexedDiagramReference->owningProjects().contains(project));
        qCDebug(logger) << "remove diagram reference file "
                        << file << " from project " << project->displayName();
        indexedDiagramReference->removeOwningProject(project);
        if (indexedDiagramReference->owningProjects().isEmpty()) {
            qCDebug(logger) << "delete indexed diagram reference from " << file;
            d->indexedDiagramReferences.remove(file);

            // remove indexedDiagramReference from set of indexedDiagramReferecesByDiagramUid
            QTC_CHECK(d->indexedDiagramReferencesByDiagramUid.contains(indexedDiagramReference->diagramUid()));
            QSet<IndexedDiagramReference *> indexedDiagramReferences = d->indexedDiagramReferencesByDiagramUid.value(indexedDiagramReference->diagramUid());
            QTC_CHECK(indexedDiagramReferences.contains(indexedDiagramReference));
            indexedDiagramReferences.remove(indexedDiagramReference);
            if (indexedDiagramReferences.isEmpty()) {
                d->indexedDiagramReferencesByDiagramUid.remove(
                            indexedDiagramReference->diagramUid());
            } else {
                d->indexedDiagramReferencesByDiagramUid.insert(
                            indexedDiagramReference->diagramUid(), indexedDiagramReferences);
            }

            delete indexedDiagramReference;
        }
    }
}

const QLoggingCategory &ModelIndexer::logger()
{
    static const QLoggingCategory category("qtc.modeleditor.modelindexer");
    return category;
}

} // namespace Internal
} // namespace ModelEditor
