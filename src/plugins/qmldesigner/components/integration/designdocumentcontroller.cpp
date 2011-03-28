/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "designdocumentcontroller.h"
#include "designdocumentcontrollerview.h"
#include "xuifiledialog.h"
#include "componentview.h"
#include "subcomponentmanager.h"
#include "model/viewlogger.h"

#include <allpropertiesbox.h>
#include <itemlibraryview.h>
#include <itemlibrarywidget.h>
#include <navigatorview.h>
#include <stateseditorview.h>
#include <formeditorview.h>
#include <formeditorwidget.h>
#include <basetexteditmodifier.h>
#include <componenttextmodifier.h>
#include <metainfo.h>
#include <invalidargumentexception.h>
#include <componentaction.h>
#include <qmlobjectnode.h>
#include <rewriterview.h>
#include <rewritingexception.h>
#include <nodelistproperty.h>
#include <toolbox.h>
#include <variantproperty.h>
#include <rewritingexception.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QtDebug>
#include <QtCore/QEvent>

#include <QtGui/QBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QErrorMessage>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QUndoStack>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QApplication>

enum {
    debug = false
};

namespace QmlDesigner {

class DesignDocumentControllerPrivate {
public:
    QWeakPointer<FormEditorView> formEditorView;

    QWeakPointer<ItemLibraryView> itemLibraryView;
    QWeakPointer<NavigatorView> navigator;
    QWeakPointer<AllPropertiesBox> allPropertiesBox;
    QWeakPointer<StatesEditorView> statesEditorView;
    QWeakPointer<QStackedWidget> stackedWidget;
    QWeakPointer<NodeInstanceView> nodeInstanceView;

    QWeakPointer<QmlDesigner::Model> model;
    QWeakPointer<QmlDesigner::Model> subComponentModel;
    QWeakPointer<QmlDesigner::Model> masterModel;
    QWeakPointer<QPlainTextEdit> textEdit;
    QWeakPointer<RewriterView> rewriterView;
    QmlDesigner::BaseTextEditModifier *textModifier;
    QmlDesigner::ComponentTextModifier *componentTextModifier;
    QWeakPointer<SubComponentManager> subComponentManager;
    QWeakPointer<Internal::ViewLogger> viewLogger;

    QString fileName;
    QUrl searchPath;
    bool documentLoaded;
    bool syncBlocked;
    QWeakPointer<ComponentAction> componentAction;

};

/**
  \class QmlDesigner::DesignDocumentController

  DesignDocumentController acts as a facade to a model representing a qml document,
  and the different views/widgets accessing it.
  */
DesignDocumentController::DesignDocumentController(QObject *parent) :
        QObject(parent),
        m_d(new DesignDocumentControllerPrivate)
{
    m_d->documentLoaded = false;
    m_d->syncBlocked = false;
}

DesignDocumentController::~DesignDocumentController()
{
    delete m_d->model.data();
    delete m_d->subComponentModel.data();

    delete m_d->rewriterView.data();

    if (m_d->componentTextModifier) //componentTextModifier might not be created
        delete m_d->componentTextModifier;

    delete m_d;
}

Model *DesignDocumentController::model() const
{
    return m_d->model.data();
}

Model *DesignDocumentController::masterModel() const
{
    return m_d->masterModel.data();
}


void DesignDocumentController::detachNodeInstanceView()
{
    if (m_d->nodeInstanceView)
        model()->detachView(m_d->nodeInstanceView.data());
}

void DesignDocumentController::attachNodeInstanceView()
{
    if (m_d->nodeInstanceView)
        model()->attachView(m_d->nodeInstanceView.data());

}

QWidget *DesignDocumentController::centralWidget() const
{
    return qobject_cast<QWidget*>(parent());
}

/*!
  Returns whether the model is automatically updated if the text editor changes.
  */
bool DesignDocumentController::isModelSyncBlocked() const
{
    return m_d->syncBlocked;
}

/*!
  Switches whether the model (and therefore the views) are updated if the text editor
  changes.

  If the synchronization is enabled again, the model is automatically resynchronized
  with the current state of the text editor.
  */
void DesignDocumentController::blockModelSync(bool block)
{
    if (m_d->syncBlocked == block)
        return;

    m_d->syncBlocked = block;

    if (m_d->textModifier) {
        if (m_d->syncBlocked) {
            detachNodeInstanceView();
            m_d->textModifier->deactivateChangeSignals();
        } else {
            attachNodeInstanceView();
            QmlModelState state;
            //We go back to base state (and back again) to avoid side effects from text editing.
            if (m_d->statesEditorView) {
                state = m_d->statesEditorView->currentState();
                m_d->statesEditorView->setCurrentState(m_d->statesEditorView->baseState());

            }

            m_d->textModifier->reactivateChangeSignals();

            if (state.isValid() && m_d->statesEditorView)
                m_d->statesEditorView->setCurrentState(state);
        }
    }
}

/*!
  Returns any errors that happened when parsing the latest qml file.
  */
QList<RewriterView::Error> DesignDocumentController::qmlErrors() const
{
    return m_d->rewriterView->errors();
}

void DesignDocumentController::setItemLibraryView(ItemLibraryView* itemLibraryView)
{
    m_d->itemLibraryView = itemLibraryView;
}

void DesignDocumentController::setNavigator(NavigatorView* navigatorView)
{
    m_d->navigator = navigatorView;
}

void DesignDocumentController::setAllPropertiesBox(AllPropertiesBox* allPropertiesBox)
{
    m_d->allPropertiesBox = allPropertiesBox;
}

void DesignDocumentController::setStatesEditorView(StatesEditorView* statesEditorView)
{
    m_d->statesEditorView = statesEditorView;
}

void DesignDocumentController::setFormEditorView(FormEditorView *formEditorView)
{
    m_d->formEditorView = formEditorView;
}

void DesignDocumentController::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    m_d->nodeInstanceView = nodeInstanceView;
}

QString DesignDocumentController::displayName() const
{
    if (fileName().isEmpty())
        return tr("-New Form-");
    else
        return fileName();
}

QString DesignDocumentController::fileName() const
{
    return m_d->fileName;
}

void DesignDocumentController::setFileName(const QString &fileName)
{
    m_d->fileName = fileName;

    if (QFileInfo(fileName).exists()) {
        m_d->searchPath = QUrl::fromLocalFile(fileName);
    } else {
        m_d->searchPath = QUrl(fileName);
    }

    if (m_d->model)
        m_d->model->setFileUrl(m_d->searchPath);

    if (m_d->itemLibraryView)
        m_d->itemLibraryView->widget()->setResourcePath(QFileInfo(fileName).absolutePath());
    emit displayNameChanged(displayName());
}

QList<RewriterView::Error> DesignDocumentController::loadMaster(QPlainTextEdit *edit)
{
    Q_CHECK_PTR(edit);

    m_d->textEdit = edit;

    connect(edit, SIGNAL(undoAvailable(bool)),
            this, SIGNAL(undoAvailable(bool)));
    connect(edit, SIGNAL(redoAvailable(bool)),
            this, SIGNAL(redoAvailable(bool)));
    connect(edit, SIGNAL(modificationChanged(bool)),
            this, SIGNAL(dirtyStateChanged(bool)));

    m_d->textModifier = new BaseTextEditModifier(dynamic_cast<TextEditor::BaseTextEditorWidget*>(m_d->textEdit.data()));

    m_d->componentTextModifier = 0;

    //m_d->masterModel = Model::create(m_d->textModifier, m_d->searchPath, errors);

    m_d->masterModel = Model::create("QtQuick.Rectangle", 1, 0);

#if defined(VIEWLOGGER)
    m_d->viewLogger = new Internal::ViewLogger(m_d->model.data());
    m_d->masterModel->attachView(m_d->viewLogger.data());
#endif

    m_d->masterModel->setFileUrl(m_d->searchPath);

    m_d->subComponentModel = Model::create("QtQuick.Rectangle", 1, 0);
    m_d->subComponentModel->setFileUrl(m_d->searchPath);

    m_d->rewriterView = new RewriterView(RewriterView::Amend, m_d->masterModel.data());
    m_d->rewriterView->setTextModifier( m_d->textModifier);
    connect(m_d->rewriterView.data(), SIGNAL(errorsChanged(const QList<RewriterView::Error> &)),
            this, SIGNAL(qmlErrorsChanged(const QList<RewriterView::Error> &)));

    m_d->masterModel->attachView(m_d->rewriterView.data());
    m_d->model = m_d->masterModel;

    m_d->subComponentManager = new SubComponentManager(m_d->masterModel->metaInfo(), this);
    m_d->subComponentManager->update(m_d->searchPath, m_d->model->imports());

    loadCurrentModel();

    return m_d->rewriterView->errors();
}

void DesignDocumentController::changeCurrentModelTo(const ModelNode &componentNode)
{
    Q_ASSERT(m_d->masterModel);
    QWeakPointer<Model> oldModel = m_d->model;
    Q_ASSERT(oldModel.data());

    if (m_d->model == m_d->subComponentModel) {
        //change back to master model
        m_d->model->detachView(m_d->rewriterView.data());
        m_d->rewriterView->setTextModifier(m_d->textModifier);
        m_d->model = m_d->masterModel;
        m_d->model->attachView(m_d->rewriterView.data());
    }
    if (!componentNode.isRootNode()) {
        Q_ASSERT(m_d->model == m_d->masterModel);
        Q_ASSERT(componentNode.isValid());
        //change to subcomponent model
        ModelNode rootModelNode = componentNode.view()->rootModelNode();
        Q_ASSERT(rootModelNode.isValid());
        if (m_d->componentTextModifier)
            delete m_d->componentTextModifier;

        int componentStartOffset = m_d->rewriterView->firstDefinitionInsideOffset(componentNode);
        int componentEndOffset = componentStartOffset + m_d->rewriterView->firstDefinitionInsideLength(componentNode);
        int rootStartOffset = m_d->rewriterView->nodeOffset(rootModelNode);

        m_d->componentTextModifier = new ComponentTextModifier (m_d->textModifier, componentStartOffset, componentEndOffset, rootStartOffset);


        m_d->model->detachView(m_d->rewriterView.data());

        m_d->rewriterView->setTextModifier(m_d->componentTextModifier);

        m_d->subComponentModel->attachView(m_d->rewriterView.data());

        Q_ASSERT(m_d->rewriterView->rootModelNode().isValid());

        m_d->model = m_d->subComponentModel;
    }

    Q_ASSERT(m_d->masterModel);
    Q_ASSERT(m_d->model);

    loadCurrentModel();
}

void DesignDocumentController::loadCurrentModel()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    Q_ASSERT(m_d->masterModel);
    Q_ASSERT(m_d->model);
    m_d->model->setMasterModel(m_d->masterModel.data());

    m_d->model->attachView(m_d->nodeInstanceView.data());
    m_d->model->attachView(m_d->navigator.data());
    m_d->itemLibraryView->widget()->setResourcePath(QFileInfo(m_d->fileName).absolutePath());

    if (!m_d->componentAction) {
        m_d->componentAction = new ComponentAction(m_d->formEditorView->widget());
        m_d->componentAction->setModel(m_d->model.data());
        connect(m_d->componentAction.data(), SIGNAL(currentComponentChanged(ModelNode)), SLOT(changeCurrentModelTo(ModelNode)));
        m_d->formEditorView->widget()->toolBox()->addAction(m_d->componentAction.data());
    }
    // Disable switching between in file components for the time being
    m_d->componentAction->setVisible(false);

    connect(m_d->itemLibraryView->widget(), SIGNAL(itemActivated(const QString&)), m_d->formEditorView.data(), SLOT(activateItemCreator(const QString&)));

    m_d->model->attachView(m_d->formEditorView.data());
    m_d->model->attachView(m_d->itemLibraryView.data());


    if (!m_d->textEdit->parent()) // hack to prevent changing owner of external text edit
        m_d->stackedWidget->addWidget(m_d->textEdit.data());

    // Will call setCurrentState (formEditorView etc has to be constructed first)
    m_d->model->attachView(m_d->statesEditorView.data());

    m_d->allPropertiesBox->setModel(m_d->model.data());

    m_d->documentLoaded = true;
    Q_ASSERT(m_d->masterModel);
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
    QFileInfo oldFileInfo(m_d->fileName);
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
    if (m_d->textEdit)
        return m_d->textEdit->document()->isModified();
    else
        return false;
}

bool DesignDocumentController::isUndoAvailable() const
{

    if (m_d->textEdit)
        return m_d->textEdit->document()->isUndoAvailable();
    return false;
}

bool DesignDocumentController::isRedoAvailable() const
{
    if (m_d->textEdit)
        return m_d->textEdit->document()->isRedoAvailable();
    return false;
}

void DesignDocumentController::close()
{
    m_d->documentLoaded = false;
    emit designDocumentClosed();
}

void DesignDocumentController::deleteSelected()
{
    if (!m_d->model)
        return;

    try {
        if (m_d->formEditorView) {
            RewriterTransaction transaction(m_d->formEditorView.data());
            QList<ModelNode> toDelete = m_d->formEditorView->selectedModelNodes();
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
    QScopedPointer<Model> model(Model::create("Qt/Rectangle"));
    model->setMetaInfo(m_d->model->metaInfo());
    model->setFileUrl(m_d->model->fileUrl());
    model->changeImports(m_d->model->imports(), QList<Import>());

    Q_ASSERT(model);

    DesignDocumentControllerView view;

    m_d->model->attachView(&view);

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

        m_d->model->detachView(&view);

        model->attachView(&view);
        view.replaceModel(selectedNode);

        Q_ASSERT(view.rootModelNode().isValid());
        Q_ASSERT(view.rootModelNode().type() != "empty");

        view.toClipboard();
    } else { //multi items selected
        m_d->model->detachView(&view);
        model->attachView(&view);

        foreach (ModelNode node, view.rootModelNode().allDirectSubModelNodes()) {
            node.destroy();
        }
        view.changeRootNodeType("Qt/Rectangle", 4, 7);
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
    QScopedPointer<Model> model(Model::create("empty"));
    model->setMetaInfo(m_d->model->metaInfo());
    model->setFileUrl(m_d->model->fileUrl());
    model->changeImports(m_d->model->imports(), QList<Import>());

    Q_ASSERT(model);

    if (!m_d->model)
        return;

    DesignDocumentControllerView view;
    model->attachView(&view);

    view.fromClipboard();

    ModelNode rootNode(view.rootModelNode());

    if (rootNode.type() == "empty")
        return;

    if (rootNode.id() == "designer__Selection") {
        QList<ModelNode> selectedNodes = rootNode.allDirectSubModelNodes();
        qDebug() << rootNode;
        qDebug() << selectedNodes;
        model->detachView(&view);
        m_d->model->attachView(&view);

        ModelNode targetNode;

        if (!view.selectedModelNodes().isEmpty())
            targetNode = view.selectedModelNodes().first();

        //In case we copy and paste a selection we paste in the parent item
        if ((view.selectedModelNodes().count() == selectedNodes.count()) && targetNode.isValid() && targetNode.parentProperty().isValid()) {
            targetNode = targetNode.parentProperty().parentModelNode();
        }

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
            RewriterTransaction transaction(m_d->formEditorView.data());

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
            RewriterTransaction transaction(m_d->formEditorView.data());

            model->detachView(&view);
            m_d->model->attachView(&view);
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
            if (targetNode.nodeListProperty(defaultProperty).isValid()) {
                targetNode.nodeListProperty(defaultProperty).reparentHere(pastedNode);
            }

            view.setSelectedModelNodes(QList<ModelNode>() << pastedNode);
        } catch (RewritingException &e) { 
            qWarning() << e.description(); //silent error
        }
    }
}

void DesignDocumentController::selectAll()
{
    if (!m_d->model)
        return;

    DesignDocumentControllerView view;
    m_d->model->attachView(&view);


    QList<ModelNode> allNodesExceptRootNode(view.allModelNodes());
    allNodesExceptRootNode.removeOne(view.rootModelNode());
    view.setSelectedModelNodes(allNodesExceptRootNode);
}

void DesignDocumentController::showError(const QString &message, QWidget *parent) const
{
    if (!parent)
        parent = m_d->stackedWidget.data();

    QMessageBox msgBox(parent);
    msgBox.setWindowFlags(Qt::Sheet | Qt::MSWindowsFixedSizeDialogHint);
    msgBox.setWindowTitle("Invalid qml");
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}

RewriterView *DesignDocumentController::rewriterView() const
{
    return m_d->rewriterView.data();
}

void DesignDocumentController::undo()
{
    if (m_d->rewriterView && !m_d->rewriterView->modificationGroupActive())
        m_d->textEdit->undo();
}

void DesignDocumentController::redo()
{
    if (m_d->rewriterView && !m_d->rewriterView->modificationGroupActive())
        m_d->textEdit->redo();
}

#ifdef ENABLE_TEXT_VIEW
void DesignDocumentController::showText()
{
    m_d->stackedWidget->setCurrentWidget(m_d->textEdit.data());
}
#endif // ENABLE_TEXT_VIEW

#ifdef ENABLE_TEXT_VIEW
void DesignDocumentController::showForm()
{
    m_d->stackedWidget->setCurrentWidget(m_d->formEditorView->widget());
}
#endif // ENABLE_TEXT_VIEW

bool DesignDocumentController::save(QWidget *parent)
{
    //    qDebug() << "Saving document to file \"" << m_d->fileName << "\"...";
    //
    if (m_d->fileName.isEmpty()) {
        saveAs(parent);
        return true;
    }
    QFile file(m_d->fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        showError(tr("Cannot write file: \"%1\".").arg(m_d->fileName), parent);
        return false;
    }

    QString errorMessage;
    bool result = save(&file, &errorMessage);
    if (!result)
        showError(errorMessage, parent);
    return result;
}

bool DesignDocumentController::save(QIODevice *device, QString * /*errorMessage*/)
{
    if (m_d->model) {
        QByteArray data = m_d->textEdit->toPlainText().toLatin1();
        device->write(data);
        m_d->textEdit->setPlainText(data); // clear undo/redo history
    }
    return false;
}


QString DesignDocumentController::contextHelpId() const
{
    DesignDocumentControllerView view;
    m_d->model->attachView(&view);

    QList<ModelNode> nodes = view.selectedModelNodes();
    QString helpId;
    if (!nodes.isEmpty()) {
        helpId = nodes.first().type();
        helpId.replace("QtQuick", "QML");
    }

    return helpId;
}

} // namespace QmlDesigner
