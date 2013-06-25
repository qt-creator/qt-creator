/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "designdocument.h"
#include "designdocumentview.h"

#include <metainfo.h>
#include <qmlobjectnode.h>
#include <rewritingexception.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <modelnodeoperations.h>
#include <qmldesignerplugin.h>
#include <viewmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>

#include <QFileInfo>
#include <QUrl>
#include <QDebug>

#include <QMessageBox>
#include <QPlainTextEdit>
#include <QApplication>

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
        m_inFileComponentModel(Model::create("QtQuick.Item", 1, 0)),
        m_currentModel(m_documentModel.data()),
        m_subComponentManager(new SubComponentManager(m_documentModel.data(), this)),
        m_rewriterView (new RewriterView(RewriterView::Amend, m_documentModel.data())),
        m_documentLoaded(false),
        m_qtVersionId(-1)
{
}

DesignDocument::~DesignDocument()
{
}

Model *DesignDocument::currentModel() const
{
    return m_currentModel.data();
}

Model *DesignDocument::documentModel() const
{
    return m_documentModel.data();
}

QWidget *DesignDocument::centralWidget() const
{
    return qobject_cast<QWidget*>(parent());
}

QString DesignDocument::pathToQt() const
{
    QtSupport::BaseQtVersion *activeQtVersion = QtSupport::QtVersionManager::instance()->version(m_qtVersionId);
    if (activeQtVersion && (activeQtVersion->qtVersion() >= QtSupport::QtVersionNumber(4, 7, 1))
            && (activeQtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)
                || activeQtVersion->type() == QLatin1String(QtSupport::Constants::SIMULATORQT)))
        return activeQtVersion->qmakeProperty("QT_INSTALL_DATA");

    return QString();
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
    bool explicitComponent = componentText.contains("Component");

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

    return new ComponentTextModifier (originalModifier, componentStartOffset, componentEndOffset, rootStartOffset);
}

bool DesignDocument::loadInFileComponent(const ModelNode &componentNode)
{
    QString componentText = rewriterView()->extractText(QList<ModelNode>() << componentNode).value(componentNode);

    if (componentText.isEmpty())
        return false;

    if (!componentNode.isRootNode()) {
        //change to subcomponent model

        m_inFileComponentTextModifier.reset(createComponentTextModifier(m_documentTextModifier.data(), rewriterView(), componentText, componentNode));

        changeToInFileComponentModel();
    }

    return true;
}

QmlModelView *DesignDocument::qmlModelView()
{
    return viewManager().qmlModelView();
}

/*!
  Returns any errors that happened when parsing the latest qml file.
  */
QList<RewriterView::Error> DesignDocument::qmlSyntaxErrors() const
{
    return m_rewriterView->errors();
}

bool DesignDocument::hasQmlSyntaxErrors() const
{
    return m_currentModel->rewriterView() && !m_currentModel->rewriterView()->errors().isEmpty();
}

QString DesignDocument::displayName() const
{
    return fileName();
}

QString DesignDocument::simplfiedDisplayName() const
{
    if (rootModelNode().id().isEmpty()) {
        return rootModelNode().id();
    } else {
        return rootModelNode().simplifiedTypeName();
    }

    QStringList list = displayName().split(QLatin1Char('/'));
    return list.last();
}

void DesignDocument::updateFileName(const QString & /*oldFileName*/, const QString &newFileName)
{
    if (m_documentModel)
        m_documentModel->setFileUrl(QUrl::fromLocalFile(newFileName));

    if (m_inFileComponentModel)
        m_inFileComponentModel->setFileUrl(QUrl::fromLocalFile(newFileName));

    viewManager().setItemLibraryViewResourcePath(QFileInfo(newFileName).absolutePath());

    emit displayNameChanged(displayName());
}

QString DesignDocument::fileName() const
{
    return editor()->document()->fileName();
}

int DesignDocument::qtVersionId() const
{
    return m_qtVersionId;
}

bool DesignDocument::isDocumentLoaded() const
{
    return m_documentLoaded;
}

void DesignDocument::resetToDocumentModel()
{
    m_currentModel = m_documentModel.data();
    m_rewriterView->setTextModifier(m_documentTextModifier.data());
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

    m_documentTextModifier.reset(new BaseTextEditModifier(dynamic_cast<TextEditor::BaseTextEditorWidget*>(plainTextEdit())));

    m_inFileComponentTextModifier.reset();

    //masterModel = Model::create(textModifier, searchPath, errors);

    updateFileName(QString(), fileName());

    m_subComponentManager->update(QUrl::fromLocalFile(fileName()), m_currentModel->imports());

    m_documentLoaded = true;
}

void DesignDocument::changeToDocumentModel()
{
    viewManager().detachRewriterView();
    viewManager().detachViewsExceptRewriterAndComponetView();

    m_currentModel = m_documentModel.data();

    viewManager().attachRewriterView(m_documentTextModifier.data());
    viewManager().attachViewsExceptRewriterAndComponetView();
}

void DesignDocument::changeToInFileComponentModel()
{
    viewManager().detachRewriterView();
    viewManager().detachViewsExceptRewriterAndComponetView();

    m_currentModel = m_inFileComponentModel.data();

    viewManager().attachRewriterView(m_inFileComponentTextModifier.data());
    viewManager().attachViewsExceptRewriterAndComponetView();
}

void DesignDocument::changeToSubComponentAndPushOnCrumblePath(const ModelNode &componentNode)
{
    if (QmlDesignerPlugin::instance()->currentDesignDocument() != this)
        return;

    changeToSubComponent(componentNode);

    QmlDesignerPlugin::instance()->viewManager().pushInFileComponentOnCrambleBar(componentNode.id());
}

void DesignDocument::changeToSubComponent(const ModelNode &componentNode)
{
    Q_ASSERT(m_documentModel);
    QWeakPointer<Model> oldModel = m_currentModel;
    Q_ASSERT(oldModel.data());

    if (m_currentModel.data() == m_inFileComponentModel.data())
        changeToDocumentModel();

    bool subComponentLoaded = loadInFileComponent(componentNode);

    if (subComponentLoaded)
        activateCurrentModel(m_inFileComponentTextModifier.data());

    if (!componentNode.id().isEmpty())
        QmlDesignerPlugin::instance()->viewManager().pushInFileComponentOnCrambleBar(componentNode.id());
}

void DesignDocument::changeToExternalSubComponent(const QString &fileName)
{
    Core::EditorManager::openEditor(fileName, Core::Id(), Core::EditorManager::DoNotMakeVisible);
}

void DesignDocument::goIntoSelectedComponent()
{
    if (!m_currentModel)
        return;

    QList<ModelNode> selectedNodes;
    if (rewriterView())
        selectedNodes = qmlModelView()->selectedModelNodes();

    if (selectedNodes.count() == 1) {
        viewManager().setComponentNode(selectedNodes.first());
        ModelNodeOperations::goIntoComponent(selectedNodes.first());
    }
}

void DesignDocument::activateCurrentModel(TextModifier *textModifier)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    Q_ASSERT(m_documentModel);
    Q_ASSERT(m_currentModel);

    if (!plainTextEdit()->parent()) // hack to prevent changing owner of external text edit
        m_stackedWidget->addWidget(plainTextEdit());

    viewManager().attachRewriterView(textModifier);

    Q_ASSERT(m_documentModel);
    QApplication::restoreOverrideCursor();
}

void DesignDocument::activateDocumentModel()
{
    activateCurrentModel(m_documentTextModifier.data());
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
    m_subComponentManager->update(QUrl::fromLocalFile(fileName()), m_currentModel->imports());
}

void DesignDocument::deleteSelected()
{
    if (!m_currentModel)
        return;

    try {
        RewriterTransaction transaction(rewriterView());
        QList<ModelNode> toDelete = qmlModelView()->selectedModelNodes();
        foreach (ModelNode node, toDelete) {
            if (node.isValid() && !node.isRootNode() && QmlObjectNode(node).isValid())
                QmlObjectNode(node).destroy();
        }

    } catch (RewritingException &e) {
        QMessageBox::warning(0, tr("Error"), e.description());
    }
}

void DesignDocument::copySelected()
{
    QScopedPointer<Model> copyModel(Model::create("QtQuick.Rectangle", 1, 0, currentModel()));
    copyModel->setFileUrl(currentModel()->fileUrl());
    copyModel->changeImports(currentModel()->imports(), QList<Import>());

    Q_ASSERT(copyModel);

    DesignDocumentView view;

    m_currentModel->attachView(&view);

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

        m_currentModel->detachView(&view);

        copyModel->attachView(&view);
        view.replaceModel(selectedNode);

        Q_ASSERT(view.rootModelNode().isValid());
        Q_ASSERT(view.rootModelNode().type() != "empty");

        view.toClipboard();
    } else { //multi items selected
        m_currentModel->detachView(&view);
        copyModel->attachView(&view);

        foreach (ModelNode node, view.rootModelNode().allDirectSubModelNodes()) {
            node.destroy();
        }
        view.changeRootNodeType("QtQuick.Rectangle", 1, 0);
        view.rootModelNode().setId("designer__Selection");

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

static void scatterItem(ModelNode pastedNode, const ModelNode targetNode, int offset = -2000)
{

    bool scatter = false;
    foreach (const ModelNode &childNode, targetNode.allDirectSubModelNodes()) {
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
        pastedNode.variantProperty("x") = int(x);
        pastedNode.variantProperty("y") = int(y);
    } else {
        double x = pastedNode.variantProperty("x").value().toDouble();
        double y = pastedNode.variantProperty("y").value().toDouble();
        x = x + offset;
        y = y + offset;
        pastedNode.variantProperty("x") = int(x);
        pastedNode.variantProperty("y") = int(y);
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

    if (rootNode.id() == "designer__Selection") {
        QList<ModelNode> selectedNodes = rootNode.allDirectSubModelNodes();
        pasteModel->detachView(&view);
        m_currentModel->attachView(&view);

        ModelNode targetNode;

        if (!view.selectedModelNodes().isEmpty())
            targetNode = view.selectedModelNodes().first();

        //In case we copy and paste a selection we paste in the parent item
        if ((view.selectedModelNodes().count() == selectedNodes.count()) && targetNode.isValid() && targetNode.parentProperty().isValid())
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
            RewriterTransaction transaction(rewriterView());

            int offset = double(qrand()) / RAND_MAX * 20 - 10;

            foreach (const ModelNode &node, selectedNodes) {
                PropertyName defaultProperty(targetNode.metaInfo().defaultPropertyName());
                ModelNode pastedNode(view.insertModel(node));
                pastedNodeList.append(pastedNode);
                scatterItem(pastedNode, targetNode, offset);
                targetNode.nodeListProperty(defaultProperty).reparentHere(pastedNode);
            }

            view.setSelectedModelNodes(pastedNodeList);
        } catch (RewritingException &e) {
            qWarning() << e.description(); //silent error
        }
    } else {
        try {
            RewriterTransaction transaction(rewriterView());

            pasteModel->detachView(&view);
            m_currentModel->attachView(&view);
            ModelNode pastedNode(view.insertModel(rootNode));
            ModelNode targetNode;

            if (!view.selectedModelNodes().isEmpty())
                targetNode = view.selectedModelNodes().first();

            if (!targetNode.isValid())
                targetNode = view.rootModelNode();

            if (targetNode.parentProperty().isValid() &&
                (pastedNode.simplifiedTypeName() == targetNode.simplifiedTypeName()) &&
                (pastedNode.variantProperty("width").value() == targetNode.variantProperty("width").value()) &&
                (pastedNode.variantProperty("height").value() == targetNode.variantProperty("height").value()))

                targetNode = targetNode.parentProperty().parentModelNode();

            PropertyName defaultProperty(targetNode.metaInfo().defaultPropertyName());

            scatterItem(pastedNode, targetNode);
            if (targetNode.nodeListProperty(defaultProperty).isValid())
                targetNode.nodeListProperty(defaultProperty).reparentHere(pastedNode);

            transaction.commit();
            NodeMetaInfo::clearCache();

            view.setSelectedModelNodes(QList<ModelNode>() << pastedNode);
        } catch (RewritingException &e) {
            qWarning() << e.description(); //silent error
        }
    }
}

void DesignDocument::selectAll()
{
    if (!m_currentModel)
        return;

    DesignDocumentView view;
    m_currentModel->attachView(&view);


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
    connect(editor->document(),
            SIGNAL(fileNameChanged(QString,QString)),
            SLOT(updateFileName(QString,QString)));

    updateActiveQtVersion();
}

Core::IEditor *DesignDocument::editor() const
{
    return m_textEditor.data();
}

TextEditor::ITextEditor *DesignDocument::textEditor() const
{
    return qobject_cast<TextEditor::ITextEditor*>(editor());
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

static bool isFileInProject(DesignDocument *designDocument, ProjectExplorer::Project *project)
{
    foreach (const QString &fileNameInProject, project->files(ProjectExplorer::Project::ExcludeGeneratedFiles)) {
        if (designDocument->fileName() == fileNameInProject) {
            return true;
        }
    }

    return false;
}

static inline QtSupport::BaseQtVersion *getActiveQtVersion(DesignDocument *designDocument)
{
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *currentProject = projectExplorer->currentProject();

    if (!currentProject)
        return 0;

    if (!isFileInProject(designDocument, currentProject))
        return 0;

    designDocument->disconnect(designDocument,  SLOT(updateActiveQtVersion()));
    designDocument->connect(projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)), designDocument, SLOT(updateActiveQtVersion()));

    designDocument->connect(currentProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)), designDocument, SLOT(updateActiveQtVersion()));


    ProjectExplorer::Target *target = currentProject->activeTarget();

    if (!target)
        return 0;

    designDocument->connect(target, SIGNAL(kitChanged()), designDocument, SLOT(updateActiveQtVersion()));
    return QtSupport::QtKitInformation::qtVersion(target->kit());
}

void DesignDocument::updateActiveQtVersion()
{
    QtSupport::BaseQtVersion *newQtVersion = getActiveQtVersion(this);

    if (!newQtVersion ) {
        m_qtVersionId = -1;
        return;
    }

    if (m_qtVersionId == newQtVersion->uniqueId())
        return;

    m_qtVersionId = newQtVersion->uniqueId();

    viewManager().setNodeInstanceViewQtPath(pathToQt());
}

QString DesignDocument::contextHelpId() const
{
    DesignDocumentView view;
    m_currentModel->attachView(&view);

    QList<ModelNode> nodes = view.selectedModelNodes();
    QString helpId;
    if (!nodes.isEmpty()) {
        helpId = nodes.first().type();
        helpId.replace("QtQuick", "QML");
    }

    return helpId;
}

} // namespace QmlDesigner
