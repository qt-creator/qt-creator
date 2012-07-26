/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "designdocumentcontroller.h"
#include "designdocumentcontrollerview.h"
#include "xuifiledialog.h"
#include "componentview.h"
#include "subcomponentmanager.h"
#include "model/viewlogger.h"

#include <itemlibraryview.h>
#include <itemlibrarywidget.h>
#include <navigatorview.h>
#include <stateseditorview.h>
#include <formeditorview.h>
#include <propertyeditor.h>
#include <formeditorwidget.h>
#include <toolbox.h>
#include <basetexteditmodifier.h>
#include <componenttextmodifier.h>
#include <metainfo.h>
#include <invalidargumentexception.h>
#include <componentview.h>
#include <componentaction.h>
#include <qmlobjectnode.h>
#include <rewriterview.h>
#include <rewritingexception.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <rewritingexception.h>
#include <model/modelnodecontextmenu.h>
#include <designmodewidget.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <qtsupport/qtprofileinformation.h>
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
#include <QtDebug>
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

class DesignDocumentControllerPrivate {
public:
    QWeakPointer<FormEditorView> formEditorView;

    QWeakPointer<ItemLibraryView> itemLibraryView;
    QWeakPointer<NavigatorView> navigator;
    QWeakPointer<PropertyEditor> propertyEditorView;
    QWeakPointer<StatesEditorView> statesEditorView;
    QWeakPointer<QStackedWidget> stackedWidget;
    QWeakPointer<NodeInstanceView> nodeInstanceView;
    QWeakPointer<ComponentView> componentView;

    QWeakPointer<QmlDesigner::Model> model;
    QWeakPointer<QmlDesigner::Model> subComponentModel;
    QWeakPointer<QmlDesigner::Model> masterModel;
    QWeakPointer<QPlainTextEdit> textEdit;
    QWeakPointer<RewriterView> rewriterView;
    QmlDesigner::BaseTextEditModifier *textModifier;
    QmlDesigner::ComponentTextModifier *componentTextModifier;
    QWeakPointer<SubComponentManager> subComponentManager;
    QWeakPointer<Internal::ViewLogger> viewLogger;
    ModelNode componentNode;

    QString fileName;
    QUrl searchPath;
    bool documentLoaded;
    bool syncBlocked;
    int qt_versionId;
    static bool clearCrumblePath;
    static bool pushCrumblePath;
};


bool DesignDocumentControllerPrivate::clearCrumblePath = true;
bool DesignDocumentControllerPrivate::pushCrumblePath = true;


/**
  \class QmlDesigner::DesignDocumentController

  DesignDocumentController acts as a facade to a model representing a qml document,
  and the different views/widgets accessing it.
  */
DesignDocumentController::DesignDocumentController(QObject *parent) :
        QObject(parent),
        d(new DesignDocumentControllerPrivate)
{
    d->documentLoaded = false;
    d->syncBlocked = false;

    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)), this, SLOT(activeQtVersionChanged()));
    activeQtVersionChanged();
}

DesignDocumentController::~DesignDocumentController()
{
    delete d->model.data();
    delete d->subComponentModel.data();

    delete d->rewriterView.data();

    if (d->componentTextModifier) //componentTextModifier might not be created
        delete d->componentTextModifier;

    delete d;
}

Model *DesignDocumentController::model() const
{
    return d->model.data();
}

Model *DesignDocumentController::masterModel() const
{
    return d->masterModel.data();
}


void DesignDocumentController::detachNodeInstanceView()
{
    if (d->nodeInstanceView)
        model()->detachView(d->nodeInstanceView.data());
}

void DesignDocumentController::attachNodeInstanceView()
{
    if (d->nodeInstanceView) {
        model()->attachView(d->nodeInstanceView.data());
    }
    if (d->formEditorView) {
        d->formEditorView->resetView();
    }
}

void DesignDocumentController::changeToMasterModel()
{
    d->model->detachView(d->rewriterView.data());
    d->rewriterView->setTextModifier(d->textModifier);
    d->model = d->masterModel;
    d->model->attachView(d->rewriterView.data());
    d->componentNode = d->rewriterView->rootModelNode();
}

QVariant DesignDocumentController::createCrumbleBarInfo()
{
    CrumbleBarInfo info;
    info.fileName = fileName();
    info.modelNode = d->componentNode;
    return QVariant::fromValue<CrumbleBarInfo>(info);
}

QWidget *DesignDocumentController::centralWidget() const
{
    return qobject_cast<QWidget*>(parent());
}

QString DesignDocumentController::pathToQt() const
{
    QtSupport::BaseQtVersion *activeQtVersion = QtSupport::QtVersionManager::instance()->version(d->qt_versionId);
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
    return d->syncBlocked;
}

/*!
  Switches whether the model (and therefore the views) are updated if the text editor
  changes.

  If the synchronization is enabled again, the model is automatically resynchronized
  with the current state of the text editor.
  */
void DesignDocumentController::blockModelSync(bool block)
{
    if (d->syncBlocked == block)
        return;

    d->syncBlocked = block;

    if (d->textModifier) {
        if (d->syncBlocked) {
            detachNodeInstanceView();
            d->textModifier->deactivateChangeSignals();
        } else {
            activeQtVersionChanged();
            changeToMasterModel();
            QmlModelState state;
            //We go back to base state (and back again) to avoid side effects from text editing.
            if (d->statesEditorView && d->statesEditorView->model()) {
                state = d->statesEditorView->currentState();
                d->statesEditorView->setCurrentState(d->statesEditorView->baseState());

            }

            d->textModifier->reactivateChangeSignals();

            if (state.isValid() && d->statesEditorView)
                d->statesEditorView->setCurrentState(state);
            attachNodeInstanceView();
            if (d->propertyEditorView)
                d->propertyEditorView->resetView();
            if (d->formEditorView)
                d->formEditorView->resetView();
        }
    }
}

/*!
  Returns any errors that happened when parsing the latest qml file.
  */
QList<RewriterView::Error> DesignDocumentController::qmlErrors() const
{
    return d->rewriterView->errors();
}

void DesignDocumentController::setItemLibraryView(ItemLibraryView* itemLibraryView)
{
    d->itemLibraryView = itemLibraryView;
}

void DesignDocumentController::setNavigator(NavigatorView* navigatorView)
{
    d->navigator = navigatorView;
}

void DesignDocumentController::setPropertyEditorView(PropertyEditor *propertyEditor)
{
    d->propertyEditorView = propertyEditor;
}

void DesignDocumentController::setStatesEditorView(StatesEditorView* statesEditorView)
{
    d->statesEditorView = statesEditorView;
}

void DesignDocumentController::setFormEditorView(FormEditorView *formEditorView)
{
    d->formEditorView = formEditorView;
}

void DesignDocumentController::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    d->nodeInstanceView = nodeInstanceView;
}

void DesignDocumentController::setComponentView(ComponentView *componentView)
{
    d->componentView = componentView;
    connect(d->componentView->action(), SIGNAL(currentComponentChanged(ModelNode)), SLOT(changeCurrentModelTo(ModelNode)));
}

static inline bool compareCrumbleBarInfo(const CrumbleBarInfo &crumbleBarInfo1, const CrumbleBarInfo &crumbleBarInfo2)
{
    return crumbleBarInfo1.fileName == crumbleBarInfo2.fileName && crumbleBarInfo1.modelNode == crumbleBarInfo2.modelNode;
}

void DesignDocumentController::setCrumbleBarInfo(const CrumbleBarInfo &crumbleBarInfo)
{
    DesignDocumentControllerPrivate::clearCrumblePath = false;
    DesignDocumentControllerPrivate::pushCrumblePath = false;
    while (!compareCrumbleBarInfo(d->formEditorView->crumblePath()->dataForLastIndex().value<CrumbleBarInfo>(), crumbleBarInfo))
        d->formEditorView->crumblePath()->popElement();
    Core::EditorManager::openEditor(crumbleBarInfo.fileName);
    DesignDocumentControllerPrivate::pushCrumblePath = true;
    Internal::DesignModeWidget::instance()->currentDesignDocumentController()->changeToSubComponent(crumbleBarInfo.modelNode);
    DesignDocumentControllerPrivate::clearCrumblePath = true;
}

void DesignDocumentController::setBlockCrumbleBar(bool b)
{
    DesignDocumentControllerPrivate::clearCrumblePath = !b;
    DesignDocumentControllerPrivate::pushCrumblePath = !b;
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
    if (!d->componentNode.isRootNode()) {
        if (d->componentNode.id().isEmpty()) {
            if (d->formEditorView->rootModelNode().id().isEmpty()) {
                return d->formEditorView->rootModelNode().simplifiedTypeName();
            }
            return d->formEditorView->rootModelNode().id();
        }
        return d->componentNode.id();
    }

    QStringList list = displayName().split(QLatin1Char('/'));
    return list.last();
}

QString DesignDocumentController::fileName() const
{
    return d->fileName;
}

void DesignDocumentController::setFileName(const QString &fileName)
{
    d->fileName = fileName;

    if (QFileInfo(fileName).exists()) {
        d->searchPath = QUrl::fromLocalFile(fileName);
    } else {
        d->searchPath = QUrl(fileName);
    }

    if (d->model)
        d->model->setFileUrl(d->searchPath);

    if (d->itemLibraryView)
        d->itemLibraryView->widget()->setResourcePath(QFileInfo(fileName).absolutePath());
    emit displayNameChanged(displayName());
}

QList<RewriterView::Error> DesignDocumentController::loadMaster(QPlainTextEdit *edit)
{
    Q_CHECK_PTR(edit);

    d->textEdit = edit;

    connect(edit, SIGNAL(undoAvailable(bool)),
            this, SIGNAL(undoAvailable(bool)));
    connect(edit, SIGNAL(redoAvailable(bool)),
            this, SIGNAL(redoAvailable(bool)));
    connect(edit, SIGNAL(modificationChanged(bool)),
            this, SIGNAL(dirtyStateChanged(bool)));

    d->textModifier = new BaseTextEditModifier(dynamic_cast<TextEditor::BaseTextEditorWidget*>(d->textEdit.data()));

    d->componentTextModifier = 0;

    //d->masterModel = Model::create(d->textModifier, d->searchPath, errors);

    d->masterModel = Model::create("QtQuick.Rectangle", 1, 0);

#if defined(VIEWLOGGER)
    d->viewLogger = new Internal::ViewLogger(d->model.data());
    d->masterModel->attachView(d->viewLogger.data());
#endif

    d->masterModel->setFileUrl(d->searchPath);

    d->subComponentModel = Model::create("QtQuick.Rectangle", 1, 0);
    d->subComponentModel->setFileUrl(d->searchPath);

    d->rewriterView = new RewriterView(RewriterView::Amend, d->masterModel.data());
    d->rewriterView->setTextModifier( d->textModifier);
    connect(d->rewriterView.data(), SIGNAL(errorsChanged(QList<RewriterView::Error>)),
            this, SIGNAL(qmlErrorsChanged(QList<RewriterView::Error>)));

    d->masterModel->attachView(d->rewriterView.data());
    d->model = d->masterModel;
    d->componentNode = d->rewriterView->rootModelNode();

    d->subComponentManager = new SubComponentManager(d->masterModel.data(), this);
    d->subComponentManager->update(d->searchPath, d->model->imports());

    loadCurrentModel();

    d->masterModel->attachView(d->componentView.data());

    return d->rewriterView->errors();
}

void DesignDocumentController::changeCurrentModelTo(const ModelNode &node)
{
    if (d->componentNode == node)
        return;
    if (Internal::DesignModeWidget::instance()->currentDesignDocumentController() != this)
        return;
    DesignDocumentControllerPrivate::clearCrumblePath = false;
    while (d->formEditorView->crumblePath()->dataForLastIndex().value<CrumbleBarInfo>().modelNode.isValid() &&
        !d->formEditorView->crumblePath()->dataForLastIndex().value<CrumbleBarInfo>().modelNode.isRootNode())
        d->formEditorView->crumblePath()->popElement();
    if (node.isRootNode() && d->formEditorView->crumblePath()->dataForLastIndex().isValid())
        d->formEditorView->crumblePath()->popElement();
    changeToSubComponent(node);
    DesignDocumentControllerPrivate::clearCrumblePath = true;
}

void DesignDocumentController::changeToSubComponent(const ModelNode &componentNode)
{
    Q_ASSERT(d->masterModel);
    QWeakPointer<Model> oldModel = d->model;
    Q_ASSERT(oldModel.data());

    if (d->model == d->subComponentModel) {
        changeToMasterModel();
    }

    QString componentText = d->rewriterView->extractText(QList<ModelNode>() << componentNode).value(componentNode);

    if (componentText.isEmpty())
        return;

    bool explicitComponent = false;
    if (componentText.contains("Component")) { //explicit component
        explicitComponent = true;
    }

    d->componentNode = componentNode;
    if (!componentNode.isRootNode()) {
        Q_ASSERT(d->model == d->masterModel);
        Q_ASSERT(componentNode.isValid());
        //change to subcomponent model
        ModelNode rootModelNode = componentNode.view()->rootModelNode();
        Q_ASSERT(rootModelNode.isValid());
        if (d->componentTextModifier)
            delete d->componentTextModifier;


        int componentStartOffset;
        int componentEndOffset;

        int rootStartOffset = d->rewriterView->nodeOffset(rootModelNode);

        if (explicitComponent) { //the component is explciit we have to find the first definition inside
            componentStartOffset = d->rewriterView->firstDefinitionInsideOffset(componentNode);
            componentEndOffset = componentStartOffset + d->rewriterView->firstDefinitionInsideLength(componentNode);
        } else { //the component is implicit
            componentStartOffset = d->rewriterView->nodeOffset(componentNode);
            componentEndOffset = componentStartOffset + d->rewriterView->nodeLength(componentNode);
        }

        d->componentTextModifier = new ComponentTextModifier (d->textModifier, componentStartOffset, componentEndOffset, rootStartOffset);


        d->model->detachView(d->rewriterView.data());

        d->rewriterView->setTextModifier(d->componentTextModifier);

        d->subComponentModel->attachView(d->rewriterView.data());

        Q_ASSERT(d->rewriterView->rootModelNode().isValid());

        d->model = d->subComponentModel;
    }

    Q_ASSERT(d->masterModel);
    Q_ASSERT(d->model);

    loadCurrentModel();
    d->componentView->setComponentNode(componentNode);
}

void DesignDocumentController::changeToExternalSubComponent(const QString &fileName)
{
    DesignDocumentControllerPrivate::clearCrumblePath = false;
    Core::EditorManager::openEditor(fileName);
    DesignDocumentControllerPrivate::clearCrumblePath = true;
}

void DesignDocumentController::goIntoComponent()
{
    if (!d->model)
        return;

    QList<ModelNode> selectedNodes;
    if (d->formEditorView)
        selectedNodes = d->formEditorView->selectedModelNodes();

    DesignDocumentControllerPrivate::clearCrumblePath = false;
    if (selectedNodes.count() == 1)
        ModelNodeAction::goIntoComponent(selectedNodes.first());
    DesignDocumentControllerPrivate::clearCrumblePath = true;
}

void DesignDocumentController::loadCurrentModel()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    Q_ASSERT(d->masterModel);
    Q_ASSERT(d->model);
    d->model->setMasterModel(d->masterModel.data());
    d->masterModel->attachView(d->componentView.data());

    d->nodeInstanceView->setPathToQt(pathToQt());
    d->model->attachView(d->nodeInstanceView.data());
    d->model->attachView(d->navigator.data());
    d->itemLibraryView->widget()->setResourcePath(QFileInfo(d->fileName).absolutePath());

    d->model->attachView(d->formEditorView.data());
    d->model->attachView(d->itemLibraryView.data());

    if (!d->textEdit->parent()) // hack to prevent changing owner of external text edit
        d->stackedWidget->addWidget(d->textEdit.data());

    // Will call setCurrentState (formEditorView etc has to be constructed first)
    d->model->attachView(d->statesEditorView.data());

    d->model->attachView(d->propertyEditorView.data());


    if (DesignDocumentControllerPrivate::clearCrumblePath)
        d->formEditorView->crumblePath()->clear();

    if (DesignDocumentControllerPrivate::pushCrumblePath &&
            !compareCrumbleBarInfo(d->formEditorView->crumblePath()->dataForLastIndex().value<CrumbleBarInfo>(),
                                   createCrumbleBarInfo().value<CrumbleBarInfo>()))
        d->formEditorView->crumblePath()->pushElement(simplfiedDisplayName(), createCrumbleBarInfo());

    d->documentLoaded = true;
    d->subComponentManager->update(d->searchPath, d->model->imports());
    Q_ASSERT(d->masterModel);
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
    QFileInfo oldFileInfo(d->fileName);
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
    if (d->textEdit)
        return d->textEdit->document()->isModified();
    else
        return false;
}

bool DesignDocumentController::isUndoAvailable() const
{

    if (d->textEdit)
        return d->textEdit->document()->isUndoAvailable();
    return false;
}

bool DesignDocumentController::isRedoAvailable() const
{
    if (d->textEdit)
        return d->textEdit->document()->isRedoAvailable();
    return false;
}

void DesignDocumentController::close()
{
    d->documentLoaded = false;
    emit designDocumentClosed();
}

void DesignDocumentController::deleteSelected()
{
    if (!d->model)
        return;

    try {
        if (d->formEditorView) {
            RewriterTransaction transaction(d->formEditorView.data());
            QList<ModelNode> toDelete = d->formEditorView->selectedModelNodes();
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
    QScopedPointer<Model> model(Model::create("QtQuick.Rectangle", 1, 0, this->model()));
    model->setFileUrl(d->model->fileUrl());
    model->changeImports(d->model->imports(), QList<Import>());

    Q_ASSERT(model);

    DesignDocumentControllerView view;

    d->model->attachView(&view);

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

        d->model->detachView(&view);

        model->attachView(&view);
        view.replaceModel(selectedNode);

        Q_ASSERT(view.rootModelNode().isValid());
        Q_ASSERT(view.rootModelNode().type() != "empty");

        view.toClipboard();
    } else { //multi items selected
        d->model->detachView(&view);
        model->attachView(&view);

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
    QScopedPointer<Model> model(Model::create("empty", 1, 0, this->model()));
    model->setFileUrl(d->model->fileUrl());
    model->changeImports(d->model->imports(), QList<Import>());

    Q_ASSERT(model);

    if (!d->model)
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
        d->model->attachView(&view);

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
            RewriterTransaction transaction(d->formEditorView.data());

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
            RewriterTransaction transaction(d->formEditorView.data());

            model->detachView(&view);
            d->model->attachView(&view);
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
    if (!d->model)
        return;

    DesignDocumentControllerView view;
    d->model->attachView(&view);


    QList<ModelNode> allNodesExceptRootNode(view.allModelNodes());
    allNodesExceptRootNode.removeOne(view.rootModelNode());
    view.setSelectedModelNodes(allNodesExceptRootNode);
}

RewriterView *DesignDocumentController::rewriterView() const
{
    return d->rewriterView.data();
}

void DesignDocumentController::undo()
{
    if (d->rewriterView && !d->rewriterView->modificationGroupActive())
        d->textEdit->undo();
    d->propertyEditorView->resetView();
}

void DesignDocumentController::redo()
{
    if (d->rewriterView && !d->rewriterView->modificationGroupActive())
        d->textEdit->redo();
    d->propertyEditorView->resetView();
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

    controller->connect(target, SIGNAL(profileChanged()), controller, SLOT(activeQtVersionChanged()));
    return QtSupport::QtProfileInformation::qtVersion(target->profile());
}

void DesignDocumentController::activeQtVersionChanged()
{
    QtSupport::BaseQtVersion *newQtVersion = getActiveQtVersion(this);

    if (!newQtVersion ) {
        d->qt_versionId = -1;
        return;
    }

    if (d->qt_versionId == newQtVersion->uniqueId())
        return;

    d->qt_versionId = newQtVersion->uniqueId();

    if (d->nodeInstanceView)
        d->nodeInstanceView->setPathToQt(pathToQt());
}

#ifdef ENABLE_TEXT_VIEW
void DesignDocumentController::showText()
{
    d->stackedWidget->setCurrentWidget(d->textEdit.data());
}
#endif // ENABLE_TEXT_VIEW

#ifdef ENABLE_TEXT_VIEW
void DesignDocumentController::showForm()
{
    d->stackedWidget->setCurrentWidget(d->formEditorView->widget());
}
#endif // ENABLE_TEXT_VIEW

bool DesignDocumentController::save(QWidget *parent)
{
    //    qDebug() << "Saving document to file \"" << d->fileName << "\"...";
    //
    if (d->fileName.isEmpty()) {
        saveAs(parent);
        return true;
    }
    Utils::FileSaver saver(d->fileName, QIODevice::Text);
    if (d->model)
        saver.write(d->textEdit->toPlainText().toLatin1());
    if (!saver.finalize(parent ? parent : d->stackedWidget.data()))
        return false;
    if (d->model)
        d->textEdit->setPlainText(d->textEdit->toPlainText()); // clear undo/redo history

    return true;
}


QString DesignDocumentController::contextHelpId() const
{
    DesignDocumentControllerView view;
    d->model->attachView(&view);

    QList<ModelNode> nodes = view.selectedModelNodes();
    QString helpId;
    if (!nodes.isEmpty()) {
        helpId = nodes.first().type();
        helpId.replace("QtQuick", "QML");
    }

    return helpId;
}

} // namespace QmlDesigner
