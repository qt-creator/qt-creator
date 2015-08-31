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

#include "modelsmanager.h"

#include "diagramsviewmanager.h"
#include "modeldocument.h"
#include "diagramdocument.h"
#include "diagrameditor.h"
#include "extdocumentcontroller.h"
#include "modelindexer.h"
#include "pxnodecontroller.h"
#include "modeleditor_constants.h"

#include "qmt/config/configcontroller.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_ui/diagramsmanager.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/project_controller/projectcontroller.h"
#include "qmt/project/project.h"
#include "qmt/serializer/diagramreferenceserializer.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <utils/fileutils.h>

#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QAction>

namespace ModelEditor {
namespace Internal {

class ModelsManager::ManagedModel
{
public:
    ManagedModel() = default;
    ManagedModel(ExtDocumentController *m_documentController,
                 DiagramsViewManager *m_diagramsViewManager, ModelDocument *m_modelDocument);

    int m_counter = 0;
    ExtDocumentController *m_documentController = 0;
    DiagramsViewManager *m_diagramsViewManager = 0;
    ModelDocument *m_modelDocument = 0;
    QList<DiagramDocument *> m_diagramDocuments;
};

ModelsManager::ManagedModel::ManagedModel(ExtDocumentController *documentController,
                                          DiagramsViewManager *diagramsViewManager,
                                          ModelDocument *modelDocument)
    : m_counter(1),
      m_documentController(documentController),
      m_diagramsViewManager(diagramsViewManager),
      m_modelDocument(modelDocument)
{
}

class ModelsManager::ModelsManagerPrivate
{
public:
    ~ModelsManagerPrivate()
    {
    }

    QList<ModelsManager::ManagedModel> managedModels;
    ModelIndexer *modelIndexer = 0;
    QList<Core::IDocument *> documentsToBeClosed;

    QAction *openDiagramContextMenuItem = 0;
    ProjectExplorer::Node *contextMenuOwnerNode = 0;
};

ModelsManager::ModelsManager(QObject *parent)
    : QObject(parent),
      d(new ModelsManagerPrivate())
{
    d->modelIndexer = new ModelIndexer(this);
#ifdef OPEN_DEFAULT_MODEL // disable feature - needs setting; does not work with qbs
    connect(d->modelIndexer, &ModelIndexer::openDefaultModel,
            this, &ModelsManager::onOpenDefaultModel, Qt::QueuedConnection);
#endif

    Core::Context projecTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);
    Core::ActionContainer *folderContainer = Core::ActionManager::actionContainer(
                ProjectExplorer::Constants::M_FOLDERCONTEXT);
    folderContainer->insertGroup(ProjectExplorer::Constants::G_FOLDER_FILES,
                                 Constants::EXPLORER_GROUP_MODELING);
    d->openDiagramContextMenuItem = new QAction(tr("Open Diagram"), this);
    Core::Command *cmd = Core::ActionManager::registerAction(
                d->openDiagramContextMenuItem, Constants::ACTION_EXPLORER_OPEN_DIAGRAM,
                projecTreeContext);
    folderContainer->addAction(cmd, Constants::EXPLORER_GROUP_MODELING);
    connect(d->openDiagramContextMenuItem, &QAction::triggered,
            this, &ModelsManager::onOpenDiagram);
    connect(ProjectExplorer::ProjectTree::instance(), &ProjectExplorer::ProjectTree::aboutToShowContextMenu,
            this, &ModelsManager::onAboutToShowContextMenu);
}

ModelsManager::~ModelsManager()
{
    QTC_CHECK(d->managedModels.isEmpty());
    delete d;
}

bool ModelsManager::isDiagramOpen(const qmt::Uid &modelUid, const qmt::Uid &diagramUid) const
{
    foreach (const ManagedModel &managedModel, d->managedModels) {
        if (managedModel.m_documentController->getProjectController()->getProject()->getUid() == modelUid) {
            foreach (const DiagramDocument *diagramDocument, managedModel.m_diagramDocuments) {
                if (diagramDocument->diagramUid() == diagramUid)
                    return true;
            }
        }
    }
    return false;
}

ExtDocumentController *ModelsManager::createModel(ModelDocument *modelDocument)
{
    auto documentController = new ExtDocumentController(this);
    QDir dir;
    dir.setPath(Core::ICore::resourcePath() + QLatin1String("/modeleditor"));
    // TODO error output on reading definition files
    documentController->getConfigController()->readStereotypeDefinitions(dir.path());

    auto diagramsViewManager = new DiagramsViewManager(this);
    connect(diagramsViewManager, &DiagramsViewManager::someDiagramOpened,
            documentController->getDiagramsManager(), &qmt::DiagramsManager::someDiagramOpened);
    connect(diagramsViewManager, &DiagramsViewManager::openEditor,
            this, [=](const qmt::MDiagram *diagram) { this->onOpenEditor(documentController, diagram); });
    connect(diagramsViewManager, &DiagramsViewManager::closeEditor,
            this, [=](const qmt::MDiagram *diagram) { this->onCloseEditor(documentController, diagram); });
    connect(diagramsViewManager, &DiagramsViewManager::diagramRenamed,
            this, [=](const qmt::MDiagram *diagram) { this->onDiagramRenamed(documentController, diagram); });
    d->managedModels.append(ManagedModel(documentController, diagramsViewManager, modelDocument));
    return documentController;
}

ExtDocumentController *ModelsManager::findModelByFileName(const QString &fileName,
                                                          ModelDocument *modelDocument)
{
    Utils::FileName fileName1 = Utils::FileName::fromString(fileName);
    for (int i = 0; i < d->managedModels.size(); ++i) {
        ManagedModel *managedModel = &d->managedModels[i];
        Utils::FileName fileName2 = Utils::FileName::fromString(managedModel->m_documentController->getProjectController()->getProject()->getFileName());
        if (fileName1 == fileName2) {
            if (managedModel->m_modelDocument != modelDocument) {
                QTC_CHECK(!managedModel->m_modelDocument);
                managedModel->m_modelDocument = modelDocument;
                ++managedModel->m_counter;
            }
            return managedModel->m_documentController;
        }
    }
    return 0;
}

ExtDocumentController *ModelsManager::findOrLoadModel(const qmt::Uid &modelUid,
                                                      DiagramDocument *diagramDocument)
{
    for (int i = 0; i < d->managedModels.size(); ++i) {
        ManagedModel *managedModel = &d->managedModels[i];
        if (managedModel->m_documentController->getProjectController()->getProject()->getUid() == modelUid) {
            ++managedModel->m_counter;
            QTC_CHECK(!managedModel->m_diagramDocuments.contains(diagramDocument));
            managedModel->m_diagramDocuments.append(diagramDocument);
            return managedModel->m_documentController;
        }
    }

    QString modelFile = d->modelIndexer->findModel(modelUid);
    if (modelFile.isEmpty())
        return 0;
    qmt::DocumentController *documentController = createModel(0);
    documentController->loadProject(modelFile);

    for (int i = 0; i < d->managedModels.size(); ++i) {
        ManagedModel *managedModel = &d->managedModels[i];
        if (managedModel->m_documentController->getProjectController()->getProject()->getUid() == modelUid) {
            // the counter was already set to 1 in createModel(0)
            QTC_CHECK(managedModel->m_counter == 1);
            QTC_CHECK(!managedModel->m_diagramDocuments.contains(diagramDocument));
            managedModel->m_diagramDocuments.append(diagramDocument);
            return managedModel->m_documentController;
        }
    }

    QTC_ASSERT(false, return 0);
    return 0;
}

void ModelsManager::release(ExtDocumentController *documentController, ModelDocument *modelDocument)
{
    Q_UNUSED(modelDocument); // avoid warning in release mode

    for (int i = 0; i < d->managedModels.size(); ++i) {
        ManagedModel *managedModel = &d->managedModels[i];
        if (managedModel->m_documentController == documentController) {
            QTC_CHECK(managedModel->m_counter > 0);
            --managedModel->m_counter;
            QTC_CHECK(managedModel->m_modelDocument == modelDocument);
            managedModel->m_modelDocument = 0;
            if (!managedModel->m_counter) {
                delete managedModel->m_diagramsViewManager;
                delete managedModel->m_documentController;
                d->managedModels.removeAt(i);
            }
            return;
        }
    }
    QTC_CHECK(false);
}

void ModelsManager::release(ExtDocumentController *documentController,
                            DiagramDocument *diagramDocument)
{
    for (int i = 0; i < d->managedModels.size(); ++i) {
        ManagedModel *managedModel = &d->managedModels[i];
        if (managedModel->m_documentController == documentController) {
            QTC_CHECK(managedModel->m_counter > 0);
            --managedModel->m_counter;
            QTC_CHECK(managedModel->m_diagramDocuments.contains(diagramDocument));
            managedModel->m_diagramDocuments.removeOne(diagramDocument);
            if (!managedModel->m_counter) {
                delete managedModel->m_diagramsViewManager;
                delete managedModel->m_documentController;
                d->managedModels.removeAt(i);
            }
            return;
        }
    }
    QTC_CHECK(false);
}

DiagramsViewManager *ModelsManager::findDiagramsViewManager(
        ExtDocumentController *documentController) const
{
    foreach (const ManagedModel &managedModel, d->managedModels) {
        if (managedModel.m_documentController == documentController)
            return managedModel.m_diagramsViewManager;
    }
    QTC_CHECK(false);
    return 0;
}

ModelDocument *ModelsManager::findModelDocument(
        ExtDocumentController *documentController) const
{
    foreach (const ManagedModel &managedModel, d->managedModels) {
        if (managedModel.m_documentController == documentController)
            return managedModel.m_modelDocument;
    }
    QTC_CHECK(false);
    return 0;
}

QList<ExtDocumentController *> ModelsManager::collectAllDocumentControllers() const
{
    QList<ExtDocumentController *> documents;
    foreach (const ManagedModel &managedModel, d->managedModels) {
        QTC_ASSERT(managedModel.m_documentController, continue);
        documents.append(managedModel.m_documentController);
    }
    return documents;
}

void ModelsManager::openDiagram(const qmt::Uid &modelUid, const qmt::Uid &diagramUid)
{
    foreach (const ManagedModel &managedModel, d->managedModels) {
        if (managedModel.m_documentController->getProjectController()->getProject()->getUid() == modelUid) {
            qmt::MDiagram *diagram = managedModel.m_documentController->getModelController()->findObject<qmt::MDiagram>(diagramUid);
            QTC_ASSERT(diagram, continue);
            onOpenEditor(managedModel.m_documentController, diagram);
        }
    }
}

void ModelsManager::onOpenEditor(ExtDocumentController *documentController,
                                 const qmt::MDiagram *diagram)
{
    foreach (const ManagedModel &managedModel, d->managedModels) {
        if (managedModel.m_documentController == documentController) {
            if (managedModel.m_modelDocument
                    && managedModel.m_modelDocument->diagramUid() == diagram->getUid()) {
                Core::EditorManager::activateEditorForDocument(managedModel.m_modelDocument);
                return;
            } else {
                foreach (DiagramDocument *diagramDocument, managedModel.m_diagramDocuments) {
                    if (diagramDocument->diagramUid() == diagram->getUid()) {
                        Core::EditorManager::activateEditorForDocument(diagramDocument);
                        return;
                    }
                }
            }
        }
    }

    // if diagram is root diagram open model file editor
    if (diagram == documentController->findRootDiagram()) {
        foreach (const ManagedModel &managedModel, d->managedModels) {
            if (managedModel.m_documentController == documentController) {
                QTC_CHECK(!managedModel.m_modelDocument);
                Core::EditorManager::openEditor(documentController->getProjectController()->getProject()->getFileName());
                return;
            }
        }
        QTC_CHECK(false);
    }

    // search diagram in model index and open that file
    QString documentReferenceFile =
            d->modelIndexer->findDiagram(
                documentController->getProjectController()->getProject()->getUid(),
                diagram->getUid());
    if (!documentReferenceFile.isEmpty()) {
        Core::EditorManager::openEditor(documentReferenceFile);
    } else {
        // open new temporary diagram editor
        qmt::DiagramReferenceSerializer serializer;
        QByteArray contents = serializer.save(
                    documentController->getProjectController()->getProject(), diagram);
        Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
                    Core::Id(Constants::DIAGRAM_EDITOR_ID), 0, contents);
        // bring editor to front
        Core::EditorManager::activateEditor(editor);
    }
}

void ModelsManager::onCloseEditor(ExtDocumentController *documentController,
                                  const qmt::MDiagram *diagram)
{
    bool closeDocument = false;
    foreach (const ManagedModel &managedModel, d->managedModels) {
        if (managedModel.m_documentController == documentController) {
            if (managedModel.m_modelDocument
                    && managedModel.m_modelDocument->diagramUid() == diagram->getUid()) {
                d->documentsToBeClosed.append(managedModel.m_modelDocument);
                closeDocument = true;
            } else {
                foreach (DiagramDocument *diagramDocument, managedModel.m_diagramDocuments) {
                    if (diagramDocument->diagramUid() == diagram->getUid()) {
                        d->documentsToBeClosed.append(diagramDocument);
                        closeDocument = true;
                    }
                }
            }
        }
    }
    if (closeDocument) {
        // TODO remove this or fix asynchronous call
        // Closing documents later one must garuantee that no remaining events are executed
        // maybe by using some deleteLater() trick? (create and deleteLater a QObject that
        // will call onCloseDocumentsLater())
        //QTimer::singleShot(0, this, SLOT(onCloseDocumentsLater()));
        onCloseDocumentsLater();
    }
}

void ModelsManager::onCloseDocumentsLater()
{
    QList<Core::IDocument *> documents = d->documentsToBeClosed;
    d->documentsToBeClosed.clear();
    Core::EditorManager::closeDocuments(documents);
}

void ModelsManager::onDiagramRenamed(ExtDocumentController *documentController,
                                     const qmt::MDiagram *diagram)
{
    foreach (const ManagedModel &managedModel, d->managedModels) {
        if (managedModel.m_documentController == documentController) {
            if (managedModel.m_modelDocument
                    && managedModel.m_modelDocument->diagramUid() == diagram->getUid()) {
                managedModel.m_modelDocument->onDiagramRenamed();
            } else {
                foreach (DiagramDocument *diagramDocument, managedModel.m_diagramDocuments) {
                    if (diagramDocument->diagramUid() == diagram->getUid())
                        diagramDocument->onDiagramRenamed();
                }
            }
        }
    }
}

void ModelsManager::onAboutToShowContextMenu(ProjectExplorer::Project *project,
                                             ProjectExplorer::Node *node)
{
    Q_UNUSED(project);

    bool canOpenDiagram = false;

    foreach (const ManagedModel &managedModel, d->managedModels) {
        if (managedModel.m_documentController->pxNodeController()->hasDiagramForExplorerNode(node)) {
            canOpenDiagram = true;
            break;
        }
    }

    if (canOpenDiagram)
        d->contextMenuOwnerNode = node;
    else
        d->contextMenuOwnerNode = 0;
    d->openDiagramContextMenuItem->setVisible(canOpenDiagram);
}

void ModelsManager::onOpenDiagram()
{
    if (ProjectExplorer::ProjectTree::instance()->currentNode() == d->contextMenuOwnerNode) {
        qmt::MDiagram *diagram = 0;
        foreach (const ManagedModel &managedModel, d->managedModels) {
            if ((diagram = managedModel.m_documentController->pxNodeController()->findDiagramForExplorerNode(d->contextMenuOwnerNode))) {
                onOpenEditor(managedModel.m_documentController, diagram);
                break;
            }
        }
    }
}

void ModelsManager::onOpenDefaultModel(const qmt::Uid &modelUid)
{
    QString modelFile = d->modelIndexer->findModel(modelUid);
    if (!modelFile.isEmpty())
        Core::EditorManager::openEditor(modelFile);
}

} // namespace Internal
} // namespace ModelEditor
