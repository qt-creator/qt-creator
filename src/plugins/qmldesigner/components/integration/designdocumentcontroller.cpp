/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designdocumentcontroller.h"
#include "designdocumentcontrollerview.h"
#include "xuifiledialog.h"
#include "componentview.h"
#include "subcomponentmanager.h"
#include "model/viewlogger.h"

#include <allpropertiesbox.h>
#include <itemlibrary.h>
#include <navigatorview.h>
#include <stateseditorwidget.h>
#include <formeditorview.h>
#include <formeditorwidget.h>
#include <basetexteditmodifier.h>
#include <componenttextmodifier.h>
#include <metainfo.h>
#include <invalidargumentexception.h>
#include <componentaction.h>
#include <qmlobjectnode.h>
#include <rewriterview.h>
#include <nodelistproperty.h>
#include <toolbox.h>

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

enum {
    debug = false
};

namespace QmlDesigner {

class DesignDocumentControllerPrivate {
public:
    QWeakPointer<FormEditorView> formEditorView;

    QWeakPointer<ItemLibrary> itemLibrary;
    QWeakPointer<NavigatorView> navigator;
    QWeakPointer<AllPropertiesBox> allPropertiesBox;
    QWeakPointer<StatesEditorWidget> statesEditorWidget;
    QWeakPointer<QStackedWidget> stackedWidget;

    QWeakPointer<QmlDesigner::Model> model;
    QWeakPointer<QmlDesigner::Model> subComponentModel;
    QWeakPointer<QmlDesigner::Model> masterModel;
    QWeakPointer<QPlainTextEdit> textEdit;
    QWeakPointer<RewriterView> rewriterView;
    QmlDesigner::BaseTextEditModifier *textModifier;
    QmlDesigner::ComponentTextModifier *componentTextModifier;
    QWeakPointer<SubComponentManager> subComponentManager;
    QWeakPointer<Internal::ViewLogger> viewLogger;

    QWeakPointer<QProcess> previewProcess;
    QWeakPointer<QProcess> previewWithDebugProcess;

    QString fileName;
    QUrl searchPath;
    bool documentLoaded;
    bool syncBlocked;

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
    m_d->itemLibrary = new ItemLibrary;
    m_d->navigator = new NavigatorView(this);
    m_d->allPropertiesBox = new AllPropertiesBox;
    m_d->statesEditorWidget = new StatesEditorWidget;
    m_d->stackedWidget = new QStackedWidget;

    m_d->documentLoaded = false;
    m_d->syncBlocked = false;
}

DesignDocumentController::~DesignDocumentController()
{
    delete m_d->formEditorView.data();

    // deleting the widgets should be safe because these are QWeakPointer's
    delete m_d->itemLibrary.data();
    delete m_d->allPropertiesBox.data();
    delete m_d->statesEditorWidget.data();
    delete m_d->stackedWidget.data();

    delete m_d->model.data();
    delete m_d->subComponentModel.data();

    delete m_d->rewriterView.data();
    // m_d->textEdit is child of stackedWidget or owned by external widget
    delete m_d->textModifier;

    if (m_d->componentTextModifier) //componentTextModifier might not be created
        delete m_d->componentTextModifier;

    delete m_d->previewProcess.data();
    delete m_d->previewWithDebugProcess.data();

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

QWidget *DesignDocumentController::documentWidget() const
{
    return m_d->stackedWidget.data();
}

void DesignDocumentController::togglePreview(bool visible)
{
    if (!m_d->documentLoaded)
        return;

    if (visible) {
        Q_ASSERT(!m_d->previewProcess);

        QString directory;
        if (!m_d->fileName.isEmpty()) {
            directory = QFileInfo(m_d->fileName).absoluteDir().path();
        } else {
            directory = QDir::tempPath();
        }
        m_d->previewProcess = createPreviewProcess(directory);
        connect(m_d->previewProcess.data(), SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(emitPreviewVisibilityChanged()));
    } else {
        Q_ASSERT(m_d->previewProcess);

        delete m_d->previewProcess.data();
    }
}

void DesignDocumentController::toggleWithDebugPreview(bool visible)
{
    if (!m_d->documentLoaded)
        return;

    if (visible) {
        Q_ASSERT(!m_d->previewWithDebugProcess);

        QString directory;
        if (!m_d->fileName.isEmpty()) {
            directory = QFileInfo(m_d->fileName).absoluteDir().path();
        } else {
            directory = QDir::tempPath();
        }
        m_d->previewWithDebugProcess = createPreviewWithDebugProcess(directory);
        connect(m_d->previewWithDebugProcess.data(), SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(emitPreviewWithDebugVisibilityChanged()));
    } else {
        Q_ASSERT(m_d->previewWithDebugProcess);

        delete m_d->previewWithDebugProcess.data();
    }
}

bool DesignDocumentController::previewVisible() const
{
    if (!m_d->documentLoaded)
        return false;

    return m_d->previewProcess;
}

bool DesignDocumentController::previewWithDebugVisible() const
{
    if (!m_d->documentLoaded)
        return false;

    return m_d->previewWithDebugProcess;
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
            m_d->textModifier->deactivateChangeSignals();
        } else {
            m_d->textModifier->reactivateChangeSignals();
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

void DesignDocumentController::emitPreviewVisibilityChanged()
{
    emit previewVisibilityChanged(false);
}

void DesignDocumentController::emitPreviewWithDebugVisibilityChanged()
{
    emit previewWithDebugVisibilityChanged(false);
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

    if (m_d->itemLibrary)
        m_d->itemLibrary->setResourcePath(QFileInfo(fileName).absolutePath());
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

    m_d->textModifier = new BaseTextEditModifier(dynamic_cast<TextEditor::BaseTextEditor*>(m_d->textEdit.data()));

    m_d->componentTextModifier = 0;

    //m_d->masterModel = Model::create(m_d->textModifier, m_d->searchPath, errors);

    m_d->masterModel = Model::create("Qt/Rectangle", 4, 6);
    m_d->masterModel->setFileUrl(m_d->searchPath);

    m_d->subComponentModel = Model::create("Qt/Rectangle", 4, 6);
    m_d->subComponentModel->setFileUrl(m_d->searchPath);

    m_d->subComponentManager = new SubComponentManager(m_d->masterModel->metaInfo(), this);
    m_d->subComponentManager->update(m_d->searchPath, m_d->textModifier->text().toUtf8());

    m_d->rewriterView = new RewriterView(RewriterView::Amend, m_d->masterModel.data());
    m_d->rewriterView->setTextModifier( m_d->textModifier);
    connect(m_d->rewriterView.data(), SIGNAL(errorsChanged(const QList<RewriterView::Error> &)),
            this, SIGNAL(qmlErrorsChanged(const QList<RewriterView::Error> &)));

    m_d->masterModel->attachView(m_d->rewriterView.data());
    m_d->model = m_d->masterModel;

#if defined(VIEWLOGGER)
    m_d->viewLogger = new Internal::ViewLogger(m_d->model.data());
    m_d->masterModel->attachView(m_d->viewLogger.data());
#endif

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
    Q_ASSERT(m_d->masterModel);
    Q_ASSERT(m_d->model);
    m_d->model->setMasterModel(m_d->masterModel.data());

    m_d->allPropertiesBox->setModel(m_d->model.data());

    m_d->model->attachView(m_d->navigator.data());
    m_d->itemLibrary->setMetaInfo(m_d->model->metaInfo());

    if (m_d->formEditorView .isNull()) {
        m_d->formEditorView = new FormEditorView(this);
        m_d->stackedWidget->addWidget(m_d->formEditorView->widget());
        ComponentAction *componentAction = new ComponentAction(m_d->formEditorView->widget());
        componentAction->setModel(m_d->model.data());
        m_d->formEditorView->widget()->lowerToolBox()->addAction(componentAction);
        connect(componentAction, SIGNAL(currentComponentChanged(ModelNode)), SLOT(changeCurrentModelTo(ModelNode))); //TODO component action
        connect(m_d->itemLibrary.data(), SIGNAL(itemActivated(const QString&)), m_d->formEditorView.data(), SLOT(activateItemCreator(const QString&)));
    }

    m_d->model->attachView(m_d->formEditorView.data());


    if (!m_d->textEdit->parent()) // hack to prevent changing owner of external text edit
        m_d->stackedWidget->addWidget(m_d->textEdit.data());

    // Will call setCurrentState (formEditorView etc has to be constructed first)
    m_d->statesEditorWidget->setup(m_d->model.data());

    m_d->documentLoaded = true;
    Q_ASSERT(m_d->masterModel);
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
        QMessageBox msgBox(documentWidget());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Cannot save to file \"%1\": permission denied.").arg(fileInfo.baseName()));
        msgBox.exec();
        return;
    } else if (!fileInfo.exists() && !fileInfo.dir().exists()) {
        QMessageBox msgBox(documentWidget());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Parent folder \"%1\" for file \"%2\" does not exist.")
                       .arg(fileInfo.dir().dirName())
                       .arg(fileInfo.baseName()));
        msgBox.exec();
        return;
    }

    setFileName(fileName);
    save(documentWidget());
}

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

bool DesignDocumentController::isDirty() const
{
    if (m_d->textEdit)
        return m_d->textEdit->document()->isModified();
    else
        return false;
}

bool DesignDocumentController::isUndoAvailable() const
{
    return m_d->textEdit->document()->isUndoAvailable();
}

bool DesignDocumentController::isRedoAvailable() const
{
    return m_d->textEdit->document()->isRedoAvailable();
}

void DesignDocumentController::close()
{
    documentWidget()->close();
    m_d->documentLoaded = false;
    emit designDocumentClosed();
}

void DesignDocumentController::deleteSelected()
{
    if (!m_d->model)
        return;

    if (m_d->formEditorView) {
        QList<ModelNode> toDelete = m_d->formEditorView->selectedModelNodes();
        foreach (ModelNode node, toDelete) {
            if (node.isValid() && !node.isRootNode() && QmlObjectNode(node).isValid())
                QmlObjectNode(node).destroy();
        }
    }
}

void DesignDocumentController::copySelected()
{
    QScopedPointer<Model> model(Model::create("Qt/Rectangle"));
    model->setMetaInfo(m_d->model->metaInfo());

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
        view.changeRootNodeType("Qt/Rectangle", 4, 6);
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

void DesignDocumentController::paste()
{
    QScopedPointer<Model> model(Model::create("empty"));
    model->setMetaInfo(m_d->model->metaInfo());
    model->setFileUrl(m_d->model->fileUrl());
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
        model->detachView(&view);
        m_d->model->attachView(&view);

        ModelNode targetNode;

        if (!view.selectedModelNodes().isEmpty())
            targetNode = view.selectedModelNodes().first();

        if (!targetNode.isValid())
            targetNode = view.rootModelNode();


        if (targetNode.parentProperty().isValid())
            targetNode = targetNode.parentProperty().parentModelNode();

        foreach (const ModelNode &node, selectedNodes) {
            foreach (const ModelNode &node2, selectedNodes) {
                if (node.isAncestorOf(node2))
                    selectedNodes.removeAll(node2);
            }

        }

        QList<ModelNode> pastedNodeList;

        foreach (const ModelNode &node, selectedNodes) {
            QString defaultProperty(targetNode.metaInfo().defaultProperty());
            ModelNode pastedNode(view.insertModel(node));
            pastedNodeList.append(pastedNode);
            targetNode.nodeListProperty(defaultProperty).reparentHere(pastedNode);
        }

        view.setSelectedModelNodes(pastedNodeList);
    } else {
        model->detachView(&view);
        m_d->model->attachView(&view);
        ModelNode pastedNode(view.insertModel(rootNode));
        ModelNode targetNode;

        if (!view.selectedModelNodes().isEmpty())
            targetNode = view.selectedModelNodes().first();

        if (!targetNode.isValid())
            targetNode = view.rootModelNode();

        if (targetNode.parentProperty().isValid())
            targetNode = targetNode.parentProperty().parentModelNode();

        QString defaultProperty(targetNode.metaInfo().defaultProperty());
        targetNode.nodeListProperty(defaultProperty).reparentHere(pastedNode);

        view.setSelectedModelNodes(QList<ModelNode>() << pastedNode);
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

QWidget *DesignDocumentController::itemLibrary() const
{
    return m_d->itemLibrary.data();
}

QWidget *DesignDocumentController::navigator() const
{
    return m_d->navigator->widget();
}

QWidget *DesignDocumentController::allPropertiesBox() const
{
    return m_d->allPropertiesBox.data();
}

QWidget *DesignDocumentController::statesEditorWidget() const
{
    return m_d->statesEditorWidget.data();
}

RewriterView *DesignDocumentController::rewriterView() const
{
    return m_d->rewriterView.data();
}

void DesignDocumentController::undo()
{
    m_d->textEdit->undo();
}

void DesignDocumentController::redo()
{
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

QProcess *DesignDocumentController::createPreviewProcess(const QString &dirPath)
{
    Q_ASSERT(QDir(dirPath).exists());

    QProcess *previewProcess = new QProcess(this);
    previewProcess->setWorkingDirectory(dirPath);

    QTemporaryFile *temporaryFile = new QTemporaryFile(QDir(dirPath).absoluteFilePath("bauhaus_tmp"));
    temporaryFile->setParent(previewProcess);

    temporaryFile->open();

    m_d->textModifier->save(temporaryFile);

    QStringList qmlViewerArgumentList;
//        qmlViewerArgumentList.append("-L");
//        qmlViewerArgumentList.append(dirPath);
    previewProcess->setWorkingDirectory(dirPath);
    qmlViewerArgumentList.append(temporaryFile->fileName());

    temporaryFile->close();

    previewProcess->start(QString("%1/qmlviewer").arg(QCoreApplication::applicationDirPath()), qmlViewerArgumentList);
    if (!previewProcess->waitForStarted())
        previewProcess->start("qmlviewer", qmlViewerArgumentList);
    if (!previewProcess->waitForStarted())
        QMessageBox::warning(documentWidget(), "qmlviewer not found", "qmlviewer should be in the PATH or in the same directory like the bauhaus binary.");

    return previewProcess;
}

QProcess *DesignDocumentController::createPreviewWithDebugProcess(const QString &dirPath)
{
    Q_ASSERT(QDir(dirPath).exists());

    QProcess *previewProcess = new QProcess(this);
    previewProcess->setWorkingDirectory(dirPath);

    QProcess *debugger = new QProcess(previewProcess);

    QTemporaryFile *temporaryFile = new QTemporaryFile(QDir(dirPath).absoluteFilePath("bauhaus_tmp"));
    temporaryFile->setParent(previewProcess);

    temporaryFile->open();

    m_d->textModifier->save(temporaryFile);

    QStringList environmentList = QProcess::systemEnvironment();
    environmentList.append("QML_DEBUG_SERVER_PORT=3768");
    previewProcess->setEnvironment(environmentList);

    QStringList qmlViewerArgumentList;
//        qmlViewerArgumentList.append("-L");
//        qmlViewerArgumentList.append(libraryPath);
    qmlViewerArgumentList.append(temporaryFile->fileName());

    temporaryFile->close();

    debugger->setWorkingDirectory(dirPath);
    debugger->start(QString("%1/qmldebugger").arg(QCoreApplication::applicationDirPath()));
    if (!debugger->waitForStarted())
        debugger->start("qmldebugger", qmlViewerArgumentList);
    if (!debugger->waitForStarted())
        QMessageBox::warning(documentWidget(), "qmldebugger not found", "qmldebugger should in the PATH or in the same directory like the bauhaus binary.");

    previewProcess->setWorkingDirectory(dirPath);
    previewProcess->start(QString("%1/qmlviewer").arg(QCoreApplication::applicationDirPath()), qmlViewerArgumentList);
    if (!previewProcess->waitForStarted())
        previewProcess->start("qmlviewer", qmlViewerArgumentList);
    if (!previewProcess->waitForStarted())
        QMessageBox::warning(documentWidget(), "qmlviewer not found", "qmlviewer should in the PATH or in the same directory like the bauhaus binary.");

    return previewProcess;
}

bool DesignDocumentController::save(QIODevice *device, QString * /*errorMessage*/)
{
    if (m_d->model) {
        QByteArray data = m_d->textEdit->toPlainText().toLatin1();
        device->write(data);
        m_d->textEdit->setPlainText(data); // clear undo/redo history
        return true;
    } else {
        return false;
    }
}

} // namespace QmlDesigner
