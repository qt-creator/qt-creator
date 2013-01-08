/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "designdocumentcontroller.h"
#include "designdocumentcontrollerview.h"
#include "xuifiledialog.h"
#include "componentview.h"

#include <itemlibrarywidget.h>
#include <formeditorwidget.h>
#include <toolbox.h>
#include <metainfo.h>
#include <invalidargumentexception.h>
#include <componentaction.h>
#include <designeractionmanager.h>
#include <qmlobjectnode.h>
#include <rewritingexception.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <rewritingexception.h>
#include <modelnodeoperations.h>
#include <designmodewidget.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/crumblepath.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>
#include <QEvent>

#include <QBoxLayout>
#include <QComboBox>
#include <QErrorMessage>
#include <QFileDialog>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QUndoStack>
#include <QPlainTextEdit>
#include <QApplication>

enum {
    debug = false
};

namespace QmlDesigner {


bool DesignDocumentController::s_clearCrumblePath = true;
bool DesignDocumentController::s_pushCrumblePath = true;


/**
  \class QmlDesigner::DesignDocumentController

  DesignDocumentController acts as a facade to a model representing a qml document,
  and the different views/widgets accessing it.
  */
DesignDocumentController::DesignDocumentController(QObject *parent) :
        QObject(parent)
{
    m_documentLoaded = false;
    m_syncBlocked = false;

    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)), this, SLOT(activeQtVersionChanged()));
    activeQtVersionChanged();
}

DesignDocumentController::~DesignDocumentController()
{
    delete m_model.data();
    delete m_subComponentModel.data();

    delete m_rewriterView.data();

    if (m_componentTextModifier) //componentTextModifier might not be created
        delete m_componentTextModifier;
}

Model *DesignDocumentController::model() const
{
    return m_model.data();
}

Model *DesignDocumentController::masterModel() const
{
    return m_masterModel.data();
}


void DesignDocumentController::detachNodeInstanceView()
{
    if (m_nodeInstanceView)
        model()->detachView(m_nodeInstanceView.data());
}

void DesignDocumentController::attachNodeInstanceView()
{
    if (m_nodeInstanceView)
        model()->attachView(m_nodeInstanceView.data());
    if (m_formEditorView)
        m_formEditorView->resetView();
}

void DesignDocumentController::changeToMasterModel()
{
    m_model->detachView(m_rewriterView.data());
    m_rewriterView->setTextModifier(m_textModifier);
    m_model = m_masterModel;
    m_model->attachView(m_rewriterView.data());
    m_componentNode = m_rewriterView->rootModelNode();
}

QVariant DesignDocumentController::createCrumbleBarInfo()
{
    CrumbleBarInfo info;
    info.fileName = fileName();
    info.modelNode = m_componentNode;
    return QVariant::fromValue<CrumbleBarInfo>(info);
}

QWidget *DesignDocumentController::centralWidget() const
{
    return qobject_cast<QWidget*>(parent());
}

QString DesignDocumentController::pathToQt() const
{
    QtSupport::BaseQtVersion *activeQtVersion = QtSupport::QtVersionManager::instance()->version(m_qt_versionId);
    if (activeQtVersion && (activeQtVersion->qtVersion().majorVersion > 3)
            && (activeQtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)
                || activeQtVersion->type() == QLatin1String(QtSupport::Constants::SIMULATORQT)))
        return activeQtVersion->qmakeProperty("QT_INSTALL_DATA");
    return QString();
}

/*!
  Returns whether the model is automatically updated if the text editor changes.
  */
bool DesignDocumentController::isModelSyncBlocked() const
{
    return m_syncBlocked;
}

/*!
  Switches whether the model (and therefore the views) are updated if the text editor
  changes.

  If the synchronization is enabled again, the model is automatically resynchronized
  with the current state of the text editor.
  */
void DesignDocumentController::blockModelSync(bool block)
{
    if (m_syncBlocked == block)
        return;

    m_syncBlocked = block;

    if (m_textModifier) {
        if (m_syncBlocked) {
            detachNodeInstanceView();
            m_textModifier->deactivateChangeSignals();
        } else {
            activeQtVersionChanged();
            changeToMasterModel();
            QmlModelState state;
            //We go back to base state (and back again) to avoid side effects from text editing.
            if (m_statesEditorView && m_statesEditorView->model()) {
                state = m_statesEditorView->currentState();
                m_statesEditorView->setCurrentState(m_statesEditorView->baseState());

            }

            m_textModifier->reactivateChangeSignals();

            if (state.isValid() && m_statesEditorView)
                m_statesEditorView->setCurrentState(state);
            attachNodeInstanceView();
            if (m_propertyEditorView)
                m_propertyEditorView->resetView();
            if (m_formEditorView)
                m_formEditorView->resetView();
        }
    }
}

/*!
  Returns any errors that happened when parsing the latest qml file.
  */
QList<RewriterView::Error> DesignDocumentController::qmlErrors() const
{
    return m_rewriterView->errors();
}

void DesignDocumentController::setItemLibraryView(ItemLibraryView* itemLibraryView)
{
    m_itemLibraryView = itemLibraryView;
}

void DesignDocumentController::setNavigator(NavigatorView* navigatorView)
{
    m_navigator = navigatorView;
}

void DesignDocumentController::setPropertyEditorView(PropertyEditor *propertyEditor)
{
    m_propertyEditorView = propertyEditor;
}

void DesignDocumentController::setStatesEditorView(StatesEditorView* statesEditorView)
{
    m_statesEditorView = statesEditorView;
}

void DesignDocumentController::setFormEditorView(FormEditorView *formEditorView)
{
    m_formEditorView = formEditorView;
}

void DesignDocumentController::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    m_nodeInstanceView = nodeInstanceView;
}

void DesignDocumentController::setComponentView(ComponentView *componentView)
{
    m_componentView = componentView;
    connect(componentView->action(), SIGNAL(currentComponentChanged(ModelNode)), SLOT(changeCurrentModelTo(ModelNode)));
}

static inline bool compareCrumbleBarInfo(const CrumbleBarInfo &crumbleBarInfo1, const CrumbleBarInfo &crumbleBarInfo2)
{
    return crumbleBarInfo1.fileName == crumbleBarInfo2.fileName && crumbleBarInfo1.modelNode == crumbleBarInfo2.modelNode;
}

void DesignDocumentController::setCrumbleBarInfo(const CrumbleBarInfo &crumbleBarInfo)
{
    s_clearCrumblePath = false;
    s_pushCrumblePath = false;
    while (!compareCrumbleBarInfo(m_formEditorView->crumblePath()->dataForLastIndex().value<CrumbleBarInfo>(), crumbleBarInfo))
        m_formEditorView->crumblePath()->popElement();
    Core::EditorManager::openEditor(crumbleBarInfo.fileName);
    s_pushCrumblePath = true;
    Internal::DesignModeWidget::instance()->currentDesignDocumentController()->changeToSubComponent(crumbleBarInfo.modelNode);
    s_clearCrumblePath = true;
}

void DesignDocumentController::setBlockCrumbleBar(bool b)
{
    s_clearCrumblePath = !b;
    s_pushCrumblePath = !b;
}

QString DesignDocumentController::displayName() const
{
    if (fileName().isEmpty())
        return tr("-New Form-");
    else
        return fileName();
}

QString DesignDocumentController::simplfiedDisplayName() const
{
    if (!m_componentNode.isRootNode()) {
        if (m_componentNode.id().isEmpty()) {
            if (m_formEditorView->rootModelNode().id().isEmpty())
                return m_formEditorView->rootModelNode().simplifiedTypeName();
            return m_formEditorView->rootModelNode().id();
        }
        return m_componentNode.id();
    }

    QStringList list = displayName().split(QLatin1Char('/'));
    return list.last();
}

QString DesignDocumentController::fileName() const
{
    return m_fileName;
}

void DesignDocumentController::setFileName(const QString &fileName)
{
    m_fileName = fileName;

    if (QFileInfo(fileName).exists())
        m_searchPath = QUrl::fromLocalFile(fileName);
    else
        m_searchPath = QUrl(fileName);

    if (m_model)
        m_model->setFileUrl(m_searchPath);

    if (m_itemLibraryView)
        m_itemLibraryView->widget()->setResourcePath(QFileInfo(fileName).absolutePath());
    emit displayNameChanged(displayName());
}

QList<RewriterView::Error> DesignDocumentController::loadMaster(QPlainTextEdit *edit)
{
    Q_CHECK_PTR(edit);

    m_textEdit = edit;

    connect(edit, SIGNAL(undoAvailable(bool)),
            this, SIGNAL(undoAvailable(bool)));
    connect(edit, SIGNAL(redoAvailable(bool)),
            this, SIGNAL(redoAvailable(bool)));
    connect(edit, SIGNAL(modificationChanged(bool)),
            this, SIGNAL(dirtyStateChanged(bool)));

    m_textModifier = new BaseTextEditModifier(dynamic_cast<TextEditor::BaseTextEditorWidget*>(m_textEdit.data()));

    m_componentTextModifier = 0;

    //masterModel = Model::create(textModifier, searchPath, errors);

    m_masterModel = Model::create("QtQuick.Rectangle", 1, 0);

#if defined(VIEWLOGGER)
    m_viewLogger = new Internal::ViewLogger(m_model.data());
    m_masterModel->attachView(m_viewLogger.data());
#endif

    m_masterModel->setFileUrl(m_searchPath);

    m_subComponentModel = Model::create("QtQuick.Rectangle", 1, 0);
    m_subComponentModel->setFileUrl(m_searchPath);

    m_rewriterView = new RewriterView(RewriterView::Amend, m_masterModel.data());
    m_rewriterView->setTextModifier( m_textModifier);
    connect(m_rewriterView.data(), SIGNAL(errorsChanged(QList<RewriterView::Error>)),
            this, SIGNAL(qmlErrorsChanged(QList<RewriterView::Error>)));

    m_masterModel->attachView(m_rewriterView.data());
    m_model = m_masterModel;
    m_componentNode = m_rewriterView->rootModelNode();

    m_subComponentManager = new SubComponentManager(m_masterModel.data(), this);
    m_subComponentManager->update(m_searchPath, m_model->imports());

    loadCurrentModel();

    m_masterModel->attachView(m_componentView.data());

    return m_rewriterView->errors();
}

void DesignDocumentController::changeCurrentModelTo(const ModelNode &node)
{
    if (m_componentNode == node)
        return;
    if (Internal::DesignModeWidget::instance()->currentDesignDocumentController() != this)
        return;
    s_clearCrumblePath = false;
    while (m_formEditorView->crumblePath()->dataForLastIndex().value<CrumbleBarInfo>().modelNode.isValid() &&
        !m_formEditorView->crumblePath()->dataForLastIndex().value<CrumbleBarInfo>().modelNode.isRootNode())
        m_formEditorView->crumblePath()->popElement();
    if (node.isRootNode() && m_formEditorView->crumblePath()->dataForLastIndex().isValid())
        m_formEditorView->crumblePath()->popElement();
    changeToSubComponent(node);
    s_clearCrumblePath = true;
}

void DesignDocumentController::changeToSubComponent(const ModelNode &componentNode)
{
    Q_ASSERT(m_masterModel);
    QWeakPointer<Model> oldModel = m_model;
    Q_ASSERT(oldModel.data());

    if (m_model == m_subComponentModel)
        changeToMasterModel();

    QString componentText = m_rewriterView->extractText(QList<ModelNode>() << componentNode).value(componentNode);

    if (componentText.isEmpty())
        return;

    bool explicitComponent = false;
    if (componentText.contains("Component")) { //explicit component
        explicitComponent = true;
    }

    m_componentNode = componentNode;
    if (!componentNode.isRootNode()) {
        Q_ASSERT(m_model == m_masterModel);
        Q_ASSERT(componentNode.isValid());
        //change to subcomponent model
        ModelNode rootModelNode = componentNode.view()->rootModelNode();
        Q_ASSERT(rootModelNode.isValid());
        if (m_componentTextModifier)
            delete m_componentTextModifier;


        int componentStartOffset;
        int componentEndOffset;

        int rootStartOffset = m_rewriterView->nodeOffset(rootModelNode);

        if (explicitComponent) { //the component is explciit we have to find the first definition inside
            componentStartOffset = m_rewriterView->firstDefinitionInsideOffset(componentNode);
            componentEndOffset = componentStartOffset + m_rewriterView->firstDefinitionInsideLength(componentNode);
        } else { //the component is implicit
            componentStartOffset = m_rewriterView->nodeOffset(componentNode);
            componentEndOffset = componentStartOffset + m_rewriterView->nodeLength(componentNode);
        }

        m_componentTextModifier = new ComponentTextModifier (m_textModifier, componentStartOffset, componentEndOffset, rootStartOffset);


        m_model->detachView(m_rewriterView.data());

        m_rewriterView->setTextModifier(m_componentTextModifier);

        m_subComponentModel->attachView(m_rewriterView.data());

        Q_ASSERT(m_rewriterView->rootModelNode().isValid());

        m_model = m_subComponentModel;
    }

    Q_ASSERT(m_masterModel);
    Q_ASSERT(m_model);

    loadCurrentModel();
    m_componentView->setComponentNode(componentNode);
}

void DesignDocumentController::changeToExternalSubComponent(const QString &fileName)
{
    s_clearCrumblePath = false;
    Core::EditorManager::openEditor(fileName);
    s_clearCrumblePath = true;
}

void DesignDocumentController::goIntoComponent()
{
    if (!m_model)
        return;

    QList<ModelNode> selectedNodes;
    if (m_formEditorView)
        selectedNodes = m_formEditorView->selectedModelNodes();

    s_clearCrumblePath = false;
    if (selectedNodes.count() == 1)
        ModelNodeOperations::goIntoComponent(selectedNodes.first());
    s_clearCrumblePath = true;
}

void DesignDocumentController::loadCurrentModel()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    Q_ASSERT(m_masterModel);
    Q_ASSERT(m_model);
    m_model->setMasterModel(m_masterModel.data());
    m_masterModel->attachView(m_componentView.data());

    m_nodeInstanceView->setPathToQt(pathToQt());
    m_model->attachView(m_nodeInstanceView.data());
    m_model->attachView(m_navigator.data());
    m_itemLibraryView->widget()->setResourcePath(QFileInfo(m_fileName).absolutePath());

    m_model->attachView(m_formEditorView.data());
    m_model->attachView(m_itemLibraryView.data());

    if (!m_textEdit->parent()) // hack to prevent changing owner of external text edit
        m_stackedWidget->addWidget(m_textEdit.data());

    // Will call setCurrentState (formEditorView etc has to be constructed first)
    m_model->attachView(m_statesEditorView.data());

    m_model->attachView(m_propertyEditorView.data());

    m_model->attachView(DesignerActionManager::view());

    if (s_clearCrumblePath)
        m_formEditorView->crumblePath()->clear();

    if (s_pushCrumblePath &&
            !compareCrumbleBarInfo(m_formEditorView->crumblePath()->dataForLastIndex().value<CrumbleBarInfo>(),
                                   createCrumbleBarInfo().value<CrumbleBarInfo>()))
        m_formEditorView->crumblePath()->pushElement(simplfiedDisplayName(), createCrumbleBarInfo());

    m_documentLoaded = true;
    m_subComponentManager->update(m_searchPath, m_model->imports());
    Q_ASSERT(m_masterModel);
    QApplication::restoreOverrideCursor();
}

QList<RewriterView::Error> DesignDocumentController::loadMaster(const QByteArray &qml)
{
    QPlainTextEdit *textEdit = new QPlainTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setPlainText(QString(qml));
    return loadMaster(textEdit);
}

void DesignDocumentController::saveAs(QWidget *parent)
{
    QFileInfo oldFileInfo(m_fileName);
    XUIFileDialog::runSaveFileDialog(oldFileInfo.path(), parent, this, SLOT(doRealSaveAs(QString)));
}

void DesignDocumentController::doRealSaveAs(const QString &fileName)
{
    if (fileName.isNull())
        return;

    QFileInfo fileInfo(fileName);
    if (fileInfo.exists() && !fileInfo.isWritable()) {
        QMessageBox msgBox(centralWidget());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Cannot save to file \"%1\": permission denied.").arg(fileInfo.baseName()));
        msgBox.exec();
        return;
    } else if (!fileInfo.exists() && !fileInfo.dir().exists()) {
        QMessageBox msgBox(centralWidget());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Parent folder \"%1\" for file \"%2\" does not exist.")
                       .arg(fileInfo.dir().dirName())
                       .arg(fileInfo.baseName()));
        msgBox.exec();
        return;
    }

    setFileName(fileName);
    save(centralWidget());
}

bool DesignDocumentController::isDirty() const
{
    if (m_textEdit)
        return m_textEdit->document()->isModified();
    else
        return false;
}

bool DesignDocumentController::isUndoAvailable() const
{

    if (m_textEdit)
        return m_textEdit->document()->isUndoAvailable();
    return false;
}

bool DesignDocumentController::isRedoAvailable() const
{
    if (m_textEdit)
        return m_textEdit->document()->isRedoAvailable();
    return false;
}

void DesignDocumentController::close()
{
    m_documentLoaded = false;
    emit designDocumentClosed();
}

void DesignDocumentController::deleteSelected()
{
    if (!m_model)
        return;

    try {
        if (m_formEditorView) {
            RewriterTransaction transaction(m_formEditorView.data());
            QList<ModelNode> toDelete = m_formEditorView->selectedModelNodes();
            foreach (ModelNode node, toDelete) {
                if (node.isValid() && !node.isRootNode() && QmlObjectNode(node).isValid())
                    QmlObjectNode(node).destroy();
            }
        }
    } catch (RewritingException &e) {
        QMessageBox::warning(0, tr("Error"), e.description());
    }
}

void DesignDocumentController::copySelected()
{
    QScopedPointer<Model> copyModel(Model::create("QtQuick.Rectangle", 1, 0, model()));
    copyModel->setFileUrl(model()->fileUrl());
    copyModel->changeImports(model()->imports(), QList<Import>());

    Q_ASSERT(copyModel);

    DesignDocumentControllerView view;

    m_model->attachView(&view);

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

        m_model->detachView(&view);

        copyModel->attachView(&view);
        view.replaceModel(selectedNode);

        Q_ASSERT(view.rootModelNode().isValid());
        Q_ASSERT(view.rootModelNode().type() != "empty");

        view.toClipboard();
    } else { //multi items selected
        m_model->detachView(&view);
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

void DesignDocumentController::cutSelected()
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

void DesignDocumentController::paste()
{
    QScopedPointer<Model> pasteModel(Model::create("empty", 1, 0, model()));
    pasteModel->setFileUrl(model()->fileUrl());
    pasteModel->changeImports(model()->imports(), QList<Import>());

    Q_ASSERT(pasteModel);

    if (!pasteModel)
        return;

    DesignDocumentControllerView view;
    pasteModel->attachView(&view);

    view.fromClipboard();

    ModelNode rootNode(view.rootModelNode());

    if (rootNode.type() == "empty")
        return;

    if (rootNode.id() == "designer__Selection") {
        QList<ModelNode> selectedNodes = rootNode.allDirectSubModelNodes();
        qDebug() << rootNode;
        qDebug() << selectedNodes;
        pasteModel->detachView(&view);
        m_model->attachView(&view);

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
            RewriterTransaction transaction(m_formEditorView.data());

            int offset = double(qrand()) / RAND_MAX * 20 - 10;

            foreach (const ModelNode &node, selectedNodes) {
                QString defaultProperty(targetNode.metaInfo().defaultPropertyName());
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
            RewriterTransaction transaction(m_formEditorView.data());

            pasteModel->detachView(&view);
            m_model->attachView(&view);
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

            QString defaultProperty(targetNode.metaInfo().defaultPropertyName());

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

void DesignDocumentController::selectAll()
{
    if (!m_model)
        return;

    DesignDocumentControllerView view;
    m_model->attachView(&view);


    QList<ModelNode> allNodesExceptRootNode(view.allModelNodes());
    allNodesExceptRootNode.removeOne(view.rootModelNode());
    view.setSelectedModelNodes(allNodesExceptRootNode);
}

RewriterView *DesignDocumentController::rewriterView() const
{
    return m_rewriterView.data();
}

void DesignDocumentController::undo()
{
    if (m_rewriterView && !m_rewriterView->modificationGroupActive())
        m_textEdit->undo();
    m_propertyEditorView->resetView();
}

void DesignDocumentController::redo()
{
    if (m_rewriterView && !m_rewriterView->modificationGroupActive())
        m_textEdit->redo();
    m_propertyEditorView->resetView();
}

static inline QtSupport::BaseQtVersion *getActiveQtVersion(DesignDocumentController *controller)
{
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *currentProject = projectExplorer->currentProject();

    if (!currentProject)
        return 0;

    controller->disconnect(controller,  SLOT(activeQtVersionChanged()));
    controller->connect(projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)), controller, SLOT(activeQtVersionChanged()));

    controller->connect(currentProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)), controller, SLOT(activeQtVersionChanged()));


    ProjectExplorer::Target *target = currentProject->activeTarget();

    if (!target)
        return 0;

    controller->connect(target, SIGNAL(kitChanged()), controller, SLOT(activeQtVersionChanged()));
    return QtSupport::QtKitInformation::qtVersion(target->kit());
}

void DesignDocumentController::activeQtVersionChanged()
{
    QtSupport::BaseQtVersion *newQtVersion = getActiveQtVersion(this);

    if (!newQtVersion ) {
        m_qt_versionId = -1;
        return;
    }

    if (m_qt_versionId == newQtVersion->uniqueId())
        return;

    m_qt_versionId = newQtVersion->uniqueId();

    if (m_nodeInstanceView)
        m_nodeInstanceView->setPathToQt(pathToQt());
}

#ifdef ENABLE_TEXT_VIEW
void DesignDocumentController::showText()
{
    m_stackedWidget->setCurrentWidget(m_textEdit.data());
}
#endif // ENABLE_TEXT_VIEW

#ifdef ENABLE_TEXT_VIEW
void DesignDocumentController::showForm()
{
    m_stackedWidget->setCurrentWidget(m_formEditorView->widget());
}
#endif // ENABLE_TEXT_VIEW

bool DesignDocumentController::save(QWidget *parent)
{
    //    qDebug() << "Saving document to file \"" << fileName << "\"...";
    //
    if (m_fileName.isEmpty()) {
        saveAs(parent);
        return true;
    }
    Utils::FileSaver saver(m_fileName, QIODevice::Text);
    if (m_model)
        saver.write(m_textEdit->toPlainText().toLatin1());
    if (!saver.finalize(parent ? parent : m_stackedWidget.data()))
        return false;
    if (m_model)
        m_textEdit->setPlainText(m_textEdit->toPlainText()); // clear undo/redo history

    return true;
}


QString DesignDocumentController::contextHelpId() const
{
    DesignDocumentControllerView view;
    m_model->attachView(&view);

    QList<ModelNode> nodes = view.selectedModelNodes();
    QString helpId;
    if (!nodes.isEmpty()) {
        helpId = nodes.first().type();
        helpId.replace("QtQuick", "QML");
    }

    return helpId;
}

} // namespace QmlDesigner
