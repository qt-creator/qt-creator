/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "designdocument.h"
#include "designdocumentview.h"
#include "documentmanager.h"

#include <metainfo.h>
#include <qmlobjectnode.h>
#include <rewritingexception.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <qmldesignerplugin.h>
#include <viewmanager.h>
#include <nodeinstanceview.h>

#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kit.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <coreplugin/idocument.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QFileInfo>
#include <QUrl>
#include <QDebug>

#include <QPlainTextEdit>
#include <QApplication>

using namespace ProjectExplorer;

enum {
    debug = false
};

namespace QmlDesigner {


/**
  \class QmlDesigner::DesignDocument

  DesignDocument acts as a facade to a model representing a qml document,
  and the different views/widgets accessing it.
  */
DesignDocument::DesignDocument(QObject *parent) :
        QObject(parent),
        m_documentModel(Model::create("QtQuick.Item", 1, 0)),
        m_subComponentManager(new SubComponentManager(m_documentModel.data(), this)),
        m_rewriterView (new RewriterView(RewriterView::Amend, m_documentModel.data())),
        m_documentLoaded(false),
        m_currentKit(0)
{
}

DesignDocument::~DesignDocument()
{
}

Model *DesignDocument::currentModel() const
{
    if (m_inFileComponentModel)
        return m_inFileComponentModel.data();

    return m_documentModel.data();
}

Model *DesignDocument::documentModel() const
{
    return m_documentModel.data();
}

QWidget *DesignDocument::centralWidget() const
{
    return qobject_cast<QWidget*>(parent());
}

const ViewManager &DesignDocument::viewManager() const
{
    return QmlDesignerPlugin::instance()->viewManager();
}

ViewManager &DesignDocument::viewManager()
{
    return QmlDesignerPlugin::instance()->viewManager();
}

static ComponentTextModifier *createComponentTextModifier(TextModifier *originalModifier,
                                                          RewriterView *rewriterView,
                                                          const QString &componentText,
                                                          const ModelNode &componentNode)
{
    bool explicitComponent = componentText.contains(QLatin1String("Component"));

    ModelNode rootModelNode = rewriterView->rootModelNode();

    int componentStartOffset;
    int componentEndOffset;

    int rootStartOffset = rewriterView->nodeOffset(rootModelNode);

    if (explicitComponent) { //the component is explciit we have to find the first definition inside
        componentStartOffset = rewriterView->firstDefinitionInsideOffset(componentNode);
        componentEndOffset = componentStartOffset + rewriterView->firstDefinitionInsideLength(componentNode);
    } else { //the component is implicit
        componentStartOffset = rewriterView->nodeOffset(componentNode);
        componentEndOffset = componentStartOffset + rewriterView->nodeLength(componentNode);
    }

    return new ComponentTextModifier(originalModifier, componentStartOffset, componentEndOffset, rootStartOffset);
}

bool DesignDocument::loadInFileComponent(const ModelNode &componentNode)
{
    QString componentText = rewriterView()->extractText(QList<ModelNode>() << componentNode).value(componentNode);

    if (componentText.isEmpty())
        return false;

    if (!componentNode.isRootNode()) {
        //change to subcomponent model
        changeToInFileComponentModel(createComponentTextModifier(m_documentTextModifier.data(), rewriterView(), componentText, componentNode));
    }

    return true;
}

AbstractView *DesignDocument::view() const
{
    return viewManager().nodeInstanceView();
}

Model* DesignDocument::createInFileComponentModel()
{
    Model *model = Model::create("QtQuick.Item", 1, 0);
    model->setFileUrl(m_documentModel->fileUrl());

    return model;
}

QList<DocumentMessage> DesignDocument::qmlParseWarnings() const
{
    return m_rewriterView->warnings();
}

bool DesignDocument::hasQmlParseWarnings() const
{
    return !m_rewriterView->warnings().isEmpty();
}

QList<DocumentMessage> DesignDocument::qmlParseErrors() const
{
    return m_rewriterView->errors();
}

bool DesignDocument::hasQmlParseErrors() const
{
    return !m_rewriterView->errors().isEmpty();
}

QString DesignDocument::displayName() const
{
    return fileName().toString();
}

QString DesignDocument::simplfiedDisplayName() const
{
    if (rootModelNode().id().isEmpty())
        return rootModelNode().id();
    else
        return rootModelNode().simplifiedTypeName();
}

void DesignDocument::updateFileName(const Utils::FileName & /*oldFileName*/, const Utils::FileName &newFileName)
{
    if (m_documentModel)
        m_documentModel->setFileUrl(QUrl::fromLocalFile(newFileName.toString()));

    if (m_inFileComponentModel)
        m_inFileComponentModel->setFileUrl(QUrl::fromLocalFile(newFileName.toString()));

    viewManager().setItemLibraryViewResourcePath(newFileName.toFileInfo().absolutePath());

    emit displayNameChanged(displayName());
}

Utils::FileName DesignDocument::fileName() const
{
    return editor()->document()->filePath();
}

Kit *DesignDocument::currentKit() const
{
    return m_currentKit;
}

bool DesignDocument::isDocumentLoaded() const
{
    return m_documentLoaded;
}

void DesignDocument::resetToDocumentModel()
{
    m_inFileComponentModel.reset();
}

void DesignDocument::loadDocument(QPlainTextEdit *edit)
{
    Q_CHECK_PTR(edit);

    connect(edit, SIGNAL(undoAvailable(bool)),
            this, SIGNAL(undoAvailable(bool)));
    connect(edit, SIGNAL(redoAvailable(bool)),
            this, SIGNAL(redoAvailable(bool)));
    connect(edit, SIGNAL(modificationChanged(bool)),
            this, SIGNAL(dirtyStateChanged(bool)));

    m_documentTextModifier.reset(new BaseTextEditModifier(dynamic_cast<TextEditor::TextEditorWidget*>(plainTextEdit())));

    connect(m_documentTextModifier.data(), &TextModifier::textChanged, this, &DesignDocument::updateQrcFiles);

    m_documentModel->setTextModifier(m_documentTextModifier.data());

    m_inFileComponentTextModifier.reset();

    updateFileName(Utils::FileName(), fileName());

    updateQrcFiles();

    m_documentLoaded = true;
}

void DesignDocument::changeToDocumentModel()
{
    viewManager().detachRewriterView();
    viewManager().detachViewsExceptRewriterAndComponetView();


    m_inFileComponentModel.reset();

    viewManager().attachRewriterView();
    viewManager().attachViewsExceptRewriterAndComponetView();
}

void DesignDocument::changeToInFileComponentModel(ComponentTextModifier *textModifer)
{
    m_inFileComponentTextModifier.reset(textModifer);
    viewManager().detachRewriterView();
    viewManager().detachViewsExceptRewriterAndComponetView();

    m_inFileComponentModel.reset(createInFileComponentModel());
    m_inFileComponentModel->setTextModifier(m_inFileComponentTextModifier.data());

    viewManager().attachRewriterView();
    viewManager().attachViewsExceptRewriterAndComponetView();
}

void DesignDocument::updateQrcFiles()
{
    ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::projectForFile(fileName());

    if (currentProject) {
        foreach (const QString &fileName, currentProject->files(ProjectExplorer::Project::SourceFiles)) {
            if (fileName.endsWith(".qrc"))
                QmlJS::ModelManagerInterface::instance()->updateQrcFile(fileName);
        }
    }
}

void DesignDocument::changeToSubComponent(const ModelNode &componentNode)
{
    if (QmlDesignerPlugin::instance()->currentDesignDocument() != this)
        return;

    if (m_inFileComponentModel)
        changeToDocumentModel();

    bool subComponentLoaded = loadInFileComponent(componentNode);

    if (subComponentLoaded)
        attachRewriterToModel();

    QmlDesignerPlugin::instance()->viewManager().pushInFileComponentOnCrumbleBar(componentNode);
    QmlDesignerPlugin::instance()->viewManager().setComponentNode(componentNode);
}

void DesignDocument::changeToMaster()
{
    if (QmlDesignerPlugin::instance()->currentDesignDocument() != this)
        return;

    if (m_inFileComponentModel)
        changeToDocumentModel();

    QmlDesignerPlugin::instance()->viewManager().pushFileOnCrumbleBar(fileName());
    QmlDesignerPlugin::instance()->viewManager().setComponentNode(rootModelNode());
}

void DesignDocument::attachRewriterToModel()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    Q_ASSERT(m_documentModel);

    viewManager().attachRewriterView();

    Q_ASSERT(m_documentModel);
    QApplication::restoreOverrideCursor();
}

bool DesignDocument::isUndoAvailable() const
{
    if (plainTextEdit())
        return plainTextEdit()->document()->isUndoAvailable();
    return false;
}

bool DesignDocument::isRedoAvailable() const
{
    if (plainTextEdit())
        return plainTextEdit()->document()->isRedoAvailable();
    return false;
}

void DesignDocument::close()
{
    m_documentLoaded = false;
    emit designDocumentClosed();
}

void DesignDocument::updateSubcomponentManager()
{
    Q_ASSERT(m_subComponentManager);
    m_subComponentManager->update(QUrl::fromLocalFile(fileName().toString()), currentModel()->imports());
}

void DesignDocument::deleteSelected()
{
    if (!currentModel())
        return;

    try {
        RewriterTransaction transaction(rewriterView(), QByteArrayLiteral("DesignDocument::deleteSelected"));
        QList<ModelNode> toDelete = view()->selectedModelNodes();
        foreach (ModelNode node, toDelete) {
            if (node.isValid() && !node.isRootNode() && QmlObjectNode::isValidQmlObjectNode(node))
                QmlObjectNode(node).destroy();
        }

        transaction.commit();
    } catch (const RewritingException &e) {
        e.showException();
    }
}

void DesignDocument::copySelected()
{
    QScopedPointer<Model> copyModel(Model::create("QtQuick.Rectangle", 1, 0, currentModel()));
    copyModel->setFileUrl(currentModel()->fileUrl());
    copyModel->changeImports(currentModel()->imports(), QList<Import>());

    Q_ASSERT(copyModel);

    DesignDocumentView view;

    currentModel()->attachView(&view);

    if (view.selectedModelNodes().isEmpty())
        return;

    QList<ModelNode> selectedNodes(view.selectedModelNodes());

    foreach (const ModelNode &node, selectedNodes) {
        foreach (const ModelNode &node2, selectedNodes) {
            if (node.isAncestorOf(node2))
                selectedNodes.removeAll(node2);
        }
    }

    if (selectedNodes.count() == 1) {
        ModelNode selectedNode(selectedNodes.first());

        if (!selectedNode.isValid())
            return;

        currentModel()->detachView(&view);

        copyModel->attachView(&view);
        view.replaceModel(selectedNode);

        Q_ASSERT(view.rootModelNode().isValid());
        Q_ASSERT(view.rootModelNode().type() != "empty");

        view.toClipboard();
    } else { //multi items selected
        currentModel()->detachView(&view);
        copyModel->attachView(&view);

        foreach (ModelNode node, view.rootModelNode().directSubModelNodes()) {
            node.destroy();
        }
        view.changeRootNodeType("QtQuick.Rectangle", 1, 0);
        view.rootModelNode().setIdWithRefactoring(QLatin1String("designer__Selection"));

        foreach (const ModelNode &selectedNode, selectedNodes) {
            ModelNode newNode(view.insertModel(selectedNode));
            view.rootModelNode().nodeListProperty("data").reparentHere(newNode);
        }

        view.toClipboard();
    }
}

void DesignDocument::cutSelected()
{
    copySelected();
    deleteSelected();
}

static void scatterItem(const ModelNode &pastedNode, const ModelNode &targetNode, int offset = -2000)
{
    if (targetNode.metaInfo().isValid() && targetNode.metaInfo().isLayoutable())
        return;

    bool scatter = false;
    foreach (const ModelNode &childNode, targetNode.directSubModelNodes()) {
        if ((childNode.variantProperty("x").value() == pastedNode.variantProperty("x").value()) &&
            (childNode.variantProperty("y").value() == pastedNode.variantProperty("y").value()))
            scatter = true;
    }
    if (!scatter)
        return;

    if (offset == -2000) {
        double x = pastedNode.variantProperty("x").value().toDouble();
        double y = pastedNode.variantProperty("y").value().toDouble();
        double targetWidth = 20;
        double targetHeight = 20;
        x = x + double(qrand()) / RAND_MAX * targetWidth - targetWidth / 2;
        y = y + double(qrand()) / RAND_MAX * targetHeight - targetHeight / 2;
        pastedNode.variantProperty("x").setValue(int(x));
        pastedNode.variantProperty("y").setValue(int(y));
    } else {
        double x = pastedNode.variantProperty("x").value().toDouble();
        double y = pastedNode.variantProperty("y").value().toDouble();
        x = x + offset;
        y = y + offset;
        pastedNode.variantProperty("x").setValue(int(x));
        pastedNode.variantProperty("y").setValue(int(y));
    }
}

void DesignDocument::paste()
{
    QScopedPointer<Model> pasteModel(Model::create("empty", 1, 0, currentModel()));
    pasteModel->setFileUrl(currentModel()->fileUrl());
    pasteModel->changeImports(currentModel()->imports(), QList<Import>());

    Q_ASSERT(pasteModel);

    if (!pasteModel)
        return;

    DesignDocumentView view;
    pasteModel->attachView(&view);

    view.fromClipboard();

    ModelNode rootNode(view.rootModelNode());

    if (rootNode.type() == "empty")
        return;

    if (rootNode.id() == QLatin1String("designer__Selection")) {
        QList<ModelNode> selectedNodes = rootNode.directSubModelNodes();
        pasteModel->detachView(&view);
        currentModel()->attachView(&view);

        ModelNode targetNode;

        if (!view.selectedModelNodes().isEmpty())
            targetNode = view.selectedModelNodes().first();

        //In case we copy and paste a selection we paste in the parent item
        if ((view.selectedModelNodes().count() == selectedNodes.count()) && targetNode.isValid() && targetNode.hasParentProperty())
            targetNode = targetNode.parentProperty().parentModelNode();

        if (!targetNode.isValid())
            targetNode = view.rootModelNode();

        foreach (const ModelNode &node, selectedNodes) {
            foreach (const ModelNode &node2, selectedNodes) {
                if (node.isAncestorOf(node2))
                    selectedNodes.removeAll(node2);
            }
        }

        QList<ModelNode> pastedNodeList;

        try {
            RewriterTransaction transaction(rewriterView(), QByteArrayLiteral("DesignDocument::paste1"));

            int offset = double(qrand()) / RAND_MAX * 20 - 10;

            foreach (const ModelNode &node, selectedNodes) {
                PropertyName defaultProperty(targetNode.metaInfo().defaultPropertyName());
                ModelNode pastedNode(view.insertModel(node));
                pastedNodeList.append(pastedNode);
                scatterItem(pastedNode, targetNode, offset);
                targetNode.nodeListProperty(defaultProperty).reparentHere(pastedNode);
            }

            view.setSelectedModelNodes(pastedNodeList);
            transaction.commit();
        } catch (const RewritingException &e) {
            qWarning() << e.description(); //silent error
        }
    } else {
        try {
            RewriterTransaction transaction(rewriterView(), QByteArrayLiteral("DesignDocument::paste2"));

            pasteModel->detachView(&view);
            currentModel()->attachView(&view);
            ModelNode pastedNode(view.insertModel(rootNode));
            ModelNode targetNode;

            if (!view.selectedModelNodes().isEmpty())
                targetNode = view.selectedModelNodes().first();

            if (!targetNode.isValid())
                targetNode = view.rootModelNode();

            if (targetNode.hasParentProperty() &&
                (pastedNode.simplifiedTypeName() == targetNode.simplifiedTypeName()) &&
                (pastedNode.variantProperty("width").value() == targetNode.variantProperty("width").value()) &&
                (pastedNode.variantProperty("height").value() == targetNode.variantProperty("height").value()))

                targetNode = targetNode.parentProperty().parentModelNode();

            PropertyName defaultProperty(targetNode.metaInfo().defaultPropertyName());

            scatterItem(pastedNode, targetNode);
            if (targetNode.metaInfo().propertyIsListProperty(defaultProperty)) {
                targetNode.nodeListProperty(defaultProperty).reparentHere(pastedNode);
            } else {
                qWarning() << "Cannot reparent to" << targetNode;
            }

            transaction.commit();
            NodeMetaInfo::clearCache();

            view.setSelectedModelNodes(QList<ModelNode>() << pastedNode);
            transaction.commit();
        } catch (const RewritingException &e) {
            qWarning() << e.description(); //silent error
        }
    }
}

void DesignDocument::selectAll()
{
    if (!currentModel())
        return;

    DesignDocumentView view;
    currentModel()->attachView(&view);


    QList<ModelNode> allNodesExceptRootNode(view.allModelNodes());
    allNodesExceptRootNode.removeOne(view.rootModelNode());
    view.setSelectedModelNodes(allNodesExceptRootNode);
}

RewriterView *DesignDocument::rewriterView() const
{
    return m_rewriterView.data();
}

void DesignDocument::setEditor(Core::IEditor *editor)
{
    m_textEditor = editor;
    connect(editor->document(), &Core::IDocument::filePathChanged,
            this, &DesignDocument::updateFileName);

    updateActiveQtVersion();
    updateCurrentProject();
}

Core::IEditor *DesignDocument::editor() const
{
    return m_textEditor.data();
}

TextEditor::BaseTextEditor *DesignDocument::textEditor() const
{
    return qobject_cast<TextEditor::BaseTextEditor*>(editor());
}

QPlainTextEdit *DesignDocument::plainTextEdit() const
{
    if (editor())
        return qobject_cast<QPlainTextEdit*>(editor()->widget());

    return 0;
}

ModelNode DesignDocument::rootModelNode() const
{
    return rewriterView()->rootModelNode();
}

void DesignDocument::undo()
{
    if (rewriterView() && !rewriterView()->modificationGroupActive())
        plainTextEdit()->undo();

    viewManager().resetPropertyEditorView();
}

void DesignDocument::redo()
{
    if (rewriterView() && !rewriterView()->modificationGroupActive())
        plainTextEdit()->redo();

    viewManager().resetPropertyEditorView();
}

static inline Kit *getActiveKit(DesignDocument *designDocument)
{
    ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::projectForFile(designDocument->fileName());

    if (!currentProject)
        currentProject = ProjectExplorer::ProjectTree::currentProject();

    if (!currentProject)
        return 0;


    QObject::connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
                     designDocument, &DesignDocument::updateActiveQtVersion, Qt::UniqueConnection);

    QObject::connect(currentProject, &Project::activeTargetChanged,
                     designDocument, &DesignDocument::updateActiveQtVersion, Qt::UniqueConnection);

    QObject::connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
                     designDocument, &DesignDocument::updateCurrentProject, Qt::UniqueConnection);

    QObject::connect(currentProject, &Project::activeTargetChanged,
                     designDocument, &DesignDocument::updateCurrentProject, Qt::UniqueConnection);


    Target *target = currentProject->activeTarget();

    if (!target)
        return 0;

    if (!target->kit()->isValid())
        return 0;
    QObject::connect(target, &Target::kitChanged,
                     designDocument, &DesignDocument::updateActiveQtVersion, Qt::UniqueConnection);

    return target->kit();
}

void DesignDocument::updateActiveQtVersion()
{
    m_currentKit = getActiveKit(this);
    viewManager().setNodeInstanceViewKit(m_currentKit);
}

void DesignDocument::updateCurrentProject()
{
    ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::projectForFile(fileName());
    viewManager().setNodeInstanceViewProject(currentProject);
}

QString DesignDocument::contextHelpId() const
{
    if (view())
        return view()->contextHelpId();

    return QString();
}

} // namespace QmlDesigner
