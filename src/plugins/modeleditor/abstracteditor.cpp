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

#include "abstracteditor.h"

#include "actionhandler.h"
#include "diagramsviewmanager.h"
#include "documentinterface.h"
#include "dragtool.h"
#include "editordiagramview.h"
#include "elementtasks.h"
#include "extdocumentcontroller.h"
#include "modeleditor_constants.h"
#include "modeleditor_plugin.h"
#include "modelsmanager.h"
#include "openelementvisitor.h"
#include "uicontroller.h"

#include "qmt/controller/undocontroller.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dselection.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_ui/diagram_mime_types.h"
#include "qmt/diagram_ui/diagramsmanager.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model_controller/mselection.h"
#include "qmt/model_ui/treemodel.h"
#include "qmt/model_ui/treemodelmanager.h"
#include "qmt/model_widgets_ui/modeltreeview.h"
#include "qmt/model_widgets_ui/propertiesview.h"
#include "qmt/stereotype/shapepaintvisitor.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/stereotype/toolbar.h"
#include "qmt/style/style.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QMap>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QToolBox>
#include <QUndoStack>
#include <QVBoxLayout>
#include <QWidget>
#include <QStyleFactory>

#include <QDebug>

namespace ModelEditor {
namespace Internal {

class AbstractEditor::AbstractEditorPrivate
{
public:
    UiController *uiController = 0;
    ActionHandler *actionHandler = 0;
    DocumentInterface *document = 0;
    qmt::PropertiesView *propertiesView = 0;
    Core::MiniSplitter *rightSplitter = 0;
    QWidget *leftGroup = 0;
    QHBoxLayout *leftGroupLayout = 0;
    QToolBox *leftToolBox = 0;
    EditorDiagramView *diagramView = 0;
    DiagramsViewManager *diagramsViewManager = 0;
    Core::MiniSplitter *rightHorizSplitter = 0;
    qmt::ModelTreeView *modelTreeView = 0;
    qmt::TreeModelManager *modelTreeViewServant = 0;
    QScrollArea *propertiesScrollArea = 0;
    QWidget *propertiesGroupWidget = 0;
    QWidget *toolbar = 0;
    SelectedArea selectedArea = NOTHING_SELECTED;
};

AbstractEditor::AbstractEditor(UiController *uiController, ActionHandler *actionHandler,
                               QWidget *parent)
    : IEditor(parent),
      d(new AbstractEditorPrivate)
{
    d->uiController = uiController;
    d->actionHandler = actionHandler;
}

AbstractEditor::~AbstractEditor()
{
    if (d->document) {
        ExtDocumentController *documentController = d->document->documentController();
        qmt::DiagramsManager *diagramsManager = documentController->getDiagramsManager();
        qmt::MDiagram *diagram = documentController->getModelController()->findObject<qmt::MDiagram>(d->document->diagramUid());
        QTC_CHECK(diagram);
        diagramsManager->unbindDiagramSceneModel(diagram);
    }
    delete d->toolbar;
    delete d;
}

void AbstractEditor::init(QWidget *parent)
{
    // create and configure properties view
    d->propertiesView = new qmt::PropertiesView(this);

    // create and configure editor ui
    d->rightSplitter = new Core::MiniSplitter(parent);
    connect(d->rightSplitter, &QSplitter::splitterMoved,
            this, &AbstractEditor::onRightSplitterMoved);
    connect(d->uiController, &UiController::rightSplitterChanged,
            this, &AbstractEditor::onRightSplitterChanged);

    d->leftGroup = new QWidget(d->rightSplitter);
    d->leftGroupLayout = new QHBoxLayout(d->leftGroup);
    d->leftGroupLayout->setContentsMargins(0, 0, 0, 0);
    d->leftGroupLayout->setSpacing(0);

    d->leftToolBox = new QToolBox(d->leftGroup);
    // Windows style does not truncate the tab label to a very small width (GTK+ does)
    static QStyle *windowsStyle = QStyleFactory().create(QStringLiteral("Windows"));
    if (windowsStyle)
        d->leftToolBox->setStyle(windowsStyle);
    // TODO improve this (and the diagram colors) for use with dark theme
    d->leftToolBox->setStyleSheet(
                QLatin1String("QToolBox::tab {"
                              "    margin-left: 2px;"
                              "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                              "                           stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,"
                              "                           stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);"
                              "    color: #606060;"
                              "}"
                              ""
                              "QToolBox::tab:selected {"
                              "    font: italic;"
                              "    color: black;"
                              "}"));
    QFont font;
    font.setPointSizeF(font.pointSizeF() * 0.8);
    d->leftToolBox->setFont(font);

    d->diagramView = new EditorDiagramView(d->leftGroup);
    d->diagramView->setVisible(false);

    d->leftGroupLayout->addWidget(d->leftToolBox, 0);
    auto frame = new QFrame(d->leftGroup);
    frame->setFrameShape(QFrame::VLine);
    d->leftGroupLayout->addWidget(frame, 0);
    d->leftGroupLayout->addWidget(d->diagramView, 1);

    d->rightHorizSplitter = new Core::MiniSplitter(d->rightSplitter);
    d->rightHorizSplitter->setOrientation(Qt::Vertical);
    connect(d->rightHorizSplitter, &QSplitter::splitterMoved,
            this, &AbstractEditor::onRightHorizSplitterMoved);
    connect(d->uiController, &UiController::rightHorizSplitterChanged,
            this, &AbstractEditor::onRightHorizSplitterChanged);

    d->modelTreeView = new qmt::ModelTreeView(d->rightHorizSplitter);
    d->modelTreeView->setFrameShape(QFrame::NoFrame);

    d->modelTreeViewServant = new qmt::TreeModelManager(this);
    d->modelTreeViewServant->setModelTreeView(d->modelTreeView);

    d->propertiesScrollArea = new QScrollArea(d->rightHorizSplitter);
    d->propertiesScrollArea->setFrameShape(QFrame::NoFrame);
    d->propertiesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->propertiesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->propertiesScrollArea->setWidgetResizable(true);

    d->rightHorizSplitter->insertWidget(0, d->modelTreeView);
    d->rightHorizSplitter->insertWidget(1, d->propertiesScrollArea);
    d->rightHorizSplitter->setStretchFactor(0, 2); // magic stretch factors for equal sizing
    d->rightHorizSplitter->setStretchFactor(1, 3);

    d->rightSplitter->insertWidget(0, d->leftGroup);
    d->rightSplitter->insertWidget(1, d->rightHorizSplitter);
    d->rightSplitter->setStretchFactor(0, 1);
    d->rightSplitter->setStretchFactor(1, 0);

    setWidget(d->rightSplitter);

    // restore splitter sizes afer any stretch factor has been set as fallback
    if (d->uiController->hasRightSplitterState())
        d->rightSplitter->restoreState(d->uiController->rightSplitterState());
    if (d->uiController->hasRightHorizSplitterState())
        d->rightHorizSplitter->restoreState(d->uiController->rightHorizSplitterState());

    // create and configure toolbar
    d->toolbar = new QWidget();
    auto toolbarLayout = new QHBoxLayout(d->toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(0);
    toolbarLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    toolbarLayout->addWidget(createToolbarCommandButton(
                                 Constants::ACTION_ADD_PACKAGE, [this]() { onAddPackage(); },
                                 QIcon(QStringLiteral(":/modelinglib/48x48/package.png")),
                                 tr("Add Package"), d->toolbar));
    toolbarLayout->addWidget(createToolbarCommandButton(
                                 Constants::ACTION_ADD_COMPONENT, [this]() { onAddComponent(); },
                                 QIcon(QStringLiteral(":/modelinglib/48x48/component.png")),
                                 tr("Add Component"), d->toolbar));
    toolbarLayout->addWidget(createToolbarCommandButton(
                                 Constants::ACTION_ADD_CLASS, [this]() { onAddClass(); },
                                 QIcon(QStringLiteral(":/modelinglib/48x48/class.png")),
                                 tr("Add Class"), d->toolbar));
    toolbarLayout->addWidget(createToolbarCommandButton(
                                 Constants::ACTION_ADD_CANVAS_DIAGRAM, [this]() { onAddCanvasDiagram(); },
                                 QIcon(QStringLiteral(":/modelinglib/48x48/canvas-diagram.png")),
                                 tr("Add Canvas Diagram"), d->toolbar));
    toolbarLayout->addItem(new QSpacerItem(20, 0));
}

void AbstractEditor::setDocument(DocumentInterface *document)
{
    QTC_CHECK(!d->document);
    d->document = document;

    initToolbars();

    ExtDocumentController *documentController = d->document->documentController();

    d->diagramView->setPxNodeController(documentController->pxNodeController());

    QTC_CHECK(!d->diagramsViewManager);
    d->diagramsViewManager = ModelEditorPlugin::modelsManager()->findDiagramsViewManager(documentController);
    documentController->getDiagramsManager()->setDiagramsView(d->diagramsViewManager);

    d->propertiesView->setDiagramController(documentController->getDiagramController());
    d->propertiesView->setModelController(documentController->getModelController());
    d->propertiesView->setStereotypeController(documentController->getStereotypeController());
    d->propertiesView->setStyleController(documentController->getStyleController());

    d->modelTreeView->setTreeModel(documentController->getSortedTreeModel());
    d->modelTreeView->setElementTasks(documentController->elementTasks());

    d->modelTreeViewServant->setTreeModel(documentController->getTreeModel());

    connect(documentController, &qmt::DocumentController::diagramClipboardChanged,
            this, &AbstractEditor::onDiagramClipboardChanged, Qt::QueuedConnection);
    connect(documentController->getUndoController()->getUndoStack(), &QUndoStack::canUndoChanged,
            this, &AbstractEditor::onCanUndoChanged, Qt::QueuedConnection);
    connect(documentController->getUndoController()->getUndoStack(), &QUndoStack::canRedoChanged,
            this, &AbstractEditor::onCanRedoChanged, Qt::QueuedConnection);
    connect(documentController->getTreeModel(), &qmt::TreeModel::modelReset,
            this, &AbstractEditor::onTreeModelReset, Qt::QueuedConnection);
    connect(documentController->getDiagramController(), &qmt::DiagramController::modified,
            this, &AbstractEditor::onDiagramModified, Qt::QueuedConnection);
    connect(documentController->getDiagramsManager(), &qmt::DiagramsManager::diagramSelectionChanged,
            this, &AbstractEditor::onDiagramSelectionChanged, Qt::QueuedConnection);
    connect(documentController->getDiagramsManager(), &qmt::DiagramsManager::diagramActivated,
            this, &AbstractEditor::onDiagramActivated, Qt::QueuedConnection);
    connect(documentController->getDiagramSceneController(), &qmt::DiagramSceneController::newElementCreated,
            this, &AbstractEditor::onNewElementCreated, Qt::QueuedConnection);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &AbstractEditor::onCurrentEditorChanged, Qt::QueuedConnection);

    connect(d->modelTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AbstractEditor::onTreeViewSelectionChanged, Qt::QueuedConnection);
    connect(d->modelTreeView, &qmt::ModelTreeView::treeViewActivated,
            this, &AbstractEditor::onTreeViewActivated, Qt::QueuedConnection);
    connect(d->modelTreeView, &QAbstractItemView::doubleClicked,
            this, &AbstractEditor::onTreeViewDoubleClicked, Qt::QueuedConnection);

    updateSelectedArea(NOTHING_SELECTED);

    // select diagram in model tree view
    qmt::MDiagram *diagram = documentController->getModelController()->findObject<qmt::MDiagram>(d->document->diagramUid());
    QTC_CHECK(diagram);
    QModelIndex modelIndex = documentController->getTreeModel()->getIndex(diagram);
    if (modelIndex.isValid())
        d->modelTreeView->selectFromSourceModelIndex(modelIndex);
}

QWidget *AbstractEditor::toolBar()
{
    return d->toolbar;
}

qmt::MDiagram *AbstractEditor::editorDiagram() const
{
    if (!d->diagramView->getDiagramSceneModel())
        return 0;
    return d->diagramView->getDiagramSceneModel()->getDiagram();
}

void AbstractEditor::undo()
{
    d->document->documentController()->getUndoController()->getUndoStack()->undo();
}

void AbstractEditor::redo()
{
    d->document->documentController()->getUndoController()->getUndoStack()->redo();
}

void AbstractEditor::cut()
{
    ExtDocumentController *documentController = d->document->documentController();

    switch (d->selectedArea) {
    case NOTHING_SELECTED:
        break;
    case DIAGRAM_SELECTED:
        documentController->cutFromCurrentDiagram();
        break;
    case TREE_VIEW_SELECTED:
        documentController->cutFromModel(d->modelTreeViewServant->getSelectedObjects());
        break;
    }
}

void AbstractEditor::copy()
{
    ExtDocumentController *documentController = d->document->documentController();

    switch (d->selectedArea) {
    case NOTHING_SELECTED:
        break;
    case DIAGRAM_SELECTED:
        if (documentController->hasCurrentDiagramSelection())
            documentController->copyFromCurrentDiagram();
        else
            documentController->copyCurrentDiagram();
        break;
    case TREE_VIEW_SELECTED:
        documentController->copyFromModel(d->modelTreeViewServant->getSelectedObjects());
        break;
    }
}

void AbstractEditor::paste()
{
    ExtDocumentController *documentController = d->document->documentController();

    switch (d->selectedArea) {
    case NOTHING_SELECTED:
        break;
    case DIAGRAM_SELECTED:
        documentController->pasteIntoCurrentDiagram();
        break;
    case TREE_VIEW_SELECTED:
        documentController->pasteIntoModel(d->modelTreeViewServant->getSelectedObject());
        break;
    }
}

void AbstractEditor::removeSelectedElements()
{
    switch (d->selectedArea) {
    case NOTHING_SELECTED:
        break;
    case DIAGRAM_SELECTED:
        d->document->documentController()->removeFromCurrentDiagram();
        break;
    case TREE_VIEW_SELECTED:
        break;
    }
}

void AbstractEditor::deleteSelectedElements()
{
    ExtDocumentController *documentController = d->document->documentController();

    switch (d->selectedArea) {
    case NOTHING_SELECTED:
        break;
    case DIAGRAM_SELECTED:
        documentController->deleteFromCurrentDiagram();
        break;
    case TREE_VIEW_SELECTED:
        documentController->deleteFromModel(d->modelTreeViewServant->getSelectedObjects());
        break;
    }
}

void AbstractEditor::selectAll()
{
    d->document->documentController()->selectAllOnCurrentDiagram();
}

void AbstractEditor::editProperties()
{
    d->propertiesView->editSelectedElement();
}

qmt::MPackage *AbstractEditor::guessSelectedPackage() const
{
    qmt::MPackage *package = 0;
    switch (d->selectedArea) {
    case NOTHING_SELECTED:
        package = d->modelTreeViewServant->getSelectedPackage();
        break;
    case DIAGRAM_SELECTED:
    {
        qmt::DocumentController *documentController = d->document->documentController();
        qmt::DiagramsManager *diagramsManager = documentController->getDiagramsManager();
        qmt::MDiagram *diagram = editorDiagram();
        qmt::DSelection selection = diagramsManager->getDiagramSceneModel(diagram)->getSelectedElements();
        if (selection.getIndices().size() == 1) {
            qmt::DPackage *diagramElement = documentController->getDiagramController()->findElement<qmt::DPackage>(selection.getIndices().at(0).getElementKey(), diagram);
            if (diagramElement)
                package = documentController->getModelController()->findObject<qmt::MPackage>(diagramElement->getModelUid());
        }
        break;
    }
    case TREE_VIEW_SELECTED:
        package = d->modelTreeViewServant->getSelectedPackage();
        break;
    }
    return package;
}

void AbstractEditor::updateSelectedArea(SelectedArea selectedArea)
{
    d->selectedArea = selectedArea;

    qmt::DocumentController *documentController = d->document->documentController();
    bool canCutCopyDelete = false;
    bool canRemove = false;
    bool canPaste = false;
    bool canSelectAll = false;
    bool canCopyDiagram = false;
    QList<qmt::MElement *> propertiesModelElements;
    QList<qmt::DElement *> propertiesDiagramElements;
    qmt::MDiagram *propertiesDiagram = 0;

    qmt::MDiagram *activeDiagram = documentController->getDiagramsManager()->getCurrentDiagram();
    if (activeDiagram != editorDiagram())
        return;
    switch (d->selectedArea) {
    case NOTHING_SELECTED:
        canSelectAll = activeDiagram && !activeDiagram->getDiagramElements().isEmpty();
        break;
    case DIAGRAM_SELECTED:
    {
        QTC_ASSERT(activeDiagram, break);
        bool hasSelection = documentController->getDiagramsManager()->getDiagramSceneModel(activeDiagram)->hasSelection();
        canCutCopyDelete = hasSelection;
        canRemove = hasSelection;
        canPaste = !documentController->isDiagramClipboardEmpty();
        canSelectAll = !activeDiagram->getDiagramElements().isEmpty();
        canCopyDiagram = !hasSelection;
        if (hasSelection) {
            qmt::DSelection selection = documentController->getDiagramsManager()->getDiagramSceneModel(activeDiagram)->getSelectedElements();
            if (!selection.isEmpty()) {
                foreach (qmt::DSelection::Index index, selection.getIndices()) {
                    qmt::DElement *diagramElement = documentController->getDiagramController()->findElement(index.getElementKey(), activeDiagram);
                    if (diagramElement)
                        propertiesDiagramElements.append(diagramElement);
                }
                if (!propertiesDiagramElements.isEmpty())
                    propertiesDiagram = activeDiagram;
            }
        }
        break;
    }
    case TREE_VIEW_SELECTED:
    {
        bool hasSelection = !d->modelTreeViewServant->getSelectedObjects().isEmpty();
        bool hasSingleSelection = d->modelTreeViewServant->getSelectedObjects().getIndices().size() == 1;
        canCutCopyDelete = hasSelection && !d->modelTreeViewServant->isRootPackageSelected();
        canPaste =  hasSingleSelection && !documentController->isModelClipboardEmpty();
        canSelectAll = activeDiagram && !activeDiagram->getDiagramElements().isEmpty();
        QModelIndexList indexes = d->modelTreeView->getSelectedSourceModelIndexes();
        if (!indexes.isEmpty()) {
            foreach (const QModelIndex &propertiesIndex, indexes) {
                if (propertiesIndex.isValid()) {
                   qmt::MElement *modelElement = documentController->getTreeModel()->getElement(propertiesIndex);
                   if (modelElement)
                       propertiesModelElements.append(modelElement);
                }
            }
        }
        break;
    }
    }

    d->actionHandler->cutAction()->setEnabled(canCutCopyDelete);
    d->actionHandler->copyAction()->setEnabled(canCutCopyDelete || canCopyDiagram);
    d->actionHandler->pasteAction()->setEnabled(canPaste);
    d->actionHandler->removeAction()->setEnabled(canRemove);
    d->actionHandler->deleteAction()->setEnabled(canCutCopyDelete);
    d->actionHandler->selectAllAction()->setEnabled(canSelectAll);

    if (!propertiesModelElements.isEmpty())
        showProperties(propertiesModelElements);
    else if (!propertiesDiagramElements.isEmpty())
        showProperties(propertiesDiagram, propertiesDiagramElements);
    else
        clearProperties();
}

void AbstractEditor::showProperties(const QList<qmt::MElement *> &modelElements)
{
    if (modelElements != d->propertiesView->getSelectedModelElements()) {
        clearProperties();
        if (modelElements.size() > 0) {
            d->propertiesView->setSelectedModelElements(modelElements);
            d->propertiesGroupWidget = d->propertiesView->getWidget();
            d->propertiesScrollArea->setWidget(d->propertiesGroupWidget);
        }
    }
}

void AbstractEditor::showProperties(qmt::MDiagram *diagram,
                                    const QList<qmt::DElement *> &diagramElements)
{
    if (diagram != d->propertiesView->getSelectedDiagram()
            || diagramElements != d->propertiesView->getSelectedDiagramElements())
    {
        clearProperties();
        if (diagram && diagramElements.size() > 0) {
            d->propertiesView->setSelectedDiagramElements(diagramElements, diagram);
            d->propertiesGroupWidget = d->propertiesView->getWidget();
            d->propertiesScrollArea->setWidget(d->propertiesGroupWidget);
        }
    }
}

void AbstractEditor::clearProperties()
{
    d->propertiesView->clearSelection();
    if (d->propertiesGroupWidget) {
        QWidget *scrollWidget = d->propertiesScrollArea->takeWidget();
        Q_UNUSED(scrollWidget); // avoid warning in release mode
        QTC_CHECK(scrollWidget == d->propertiesGroupWidget);
        d->propertiesGroupWidget->deleteLater();
        d->propertiesGroupWidget = 0;
    }
}

void AbstractEditor::openDiagram(qmt::MDiagram *diagram)
{
    QTC_ASSERT(diagram, return);
    d->document->documentController()->getDiagramsManager()->openDiagram(diagram);
}

void AbstractEditor::showDiagram(qmt::MDiagram *diagram)
{
    if (diagram) {
        qmt::DiagramSceneModel *diagramSceneModel = d->document->documentController()->getDiagramsManager()->bindDiagramSceneModel(diagram);
        QTC_ASSERT(diagramSceneModel, return);
        QTC_ASSERT(d->diagramView, return);
        d->diagramView->setDiagramSceneModel(diagramSceneModel);
        d->diagramView->setVisible(true);
    }
}

void AbstractEditor::expandModelTreeToDepth(int depth)
{
    d->modelTreeView->expandToDepth(depth);
}

QWidget *AbstractEditor::createToolbarCommandButton(const Core::Id &id, const std::function<void()> &slot,
                                                    const QIcon &icon, const QString &toolTipBase,
                                                    QWidget *parent)
{
    auto button = new Core::CommandButton(id, parent);
    button->setIcon(icon);
    button->setToolTipBase(toolTipBase);
    connect(button, &Core::CommandButton::clicked, this, slot);
    return button;
}

/*!
    Tries to change the \a button icon to the icon specified by \a name
    from the current theme. Returns \c true if icon is updated, \c false
    otherwise.
*/

bool AbstractEditor::updateButtonIconByTheme(QAbstractButton *button, const QString &name)
{
    QTC_ASSERT(button, return false);
    QTC_ASSERT(!name.isEmpty(), return false);

    if (QIcon::hasThemeIcon(name)) {
        button->setIcon(QIcon::fromTheme(name));
        return true;
    }

    return false;
}

void AbstractEditor::onAddPackage()
{
    ExtDocumentController *documentController = d->document->documentController();

    qmt::MPackage *package = documentController->createNewPackage(d->modelTreeViewServant->getSelectedPackage());
    d->modelTreeView->selectFromSourceModelIndex(documentController->getTreeModel()->getIndex(package));
    metaObject()->invokeMethod(this, "onEditSelectedElement", Qt::QueuedConnection);
}

void AbstractEditor::onAddComponent()
{
    ExtDocumentController *documentController = d->document->documentController();

    qmt::MComponent *component = documentController->createNewComponent(d->modelTreeViewServant->getSelectedPackage());
    d->modelTreeView->selectFromSourceModelIndex(documentController->getTreeModel()->getIndex(component));
    metaObject()->invokeMethod(this, "onEditSelectedElement", Qt::QueuedConnection);
}

void AbstractEditor::onAddClass()
{
    ExtDocumentController *documentController = d->document->documentController();

    qmt::MClass *klass = documentController->createNewClass(d->modelTreeViewServant->getSelectedPackage());
    d->modelTreeView->selectFromSourceModelIndex(documentController->getTreeModel()->getIndex(klass));
    metaObject()->invokeMethod(this, "onEditSelectedElement", Qt::QueuedConnection);
}

void AbstractEditor::onAddCanvasDiagram()
{
    ExtDocumentController *documentController = d->document->documentController();

    qmt::MDiagram *diagram = documentController->createNewCanvasDiagram(d->modelTreeViewServant->getSelectedPackage());
    d->modelTreeView->selectFromSourceModelIndex(documentController->getTreeModel()->getIndex(diagram));
    metaObject()->invokeMethod(this, "onEditSelectedElement", Qt::QueuedConnection);
}

void AbstractEditor::onCurrentEditorChanged(Core::IEditor *editor)
{
    if (this == editor) {
        QUndoStack *undo_stack = d->document->documentController()->getUndoController()->getUndoStack();
        d->actionHandler->undoAction()->setDisabled(!undo_stack->canUndo());
        d->actionHandler->redoAction()->setDisabled(!undo_stack->canRedo());

        updateSelectedArea(NOTHING_SELECTED);
    }
}

void AbstractEditor::onCanUndoChanged(bool canUndo)
{
    if (this == Core::EditorManager::currentEditor())
        d->actionHandler->undoAction()->setEnabled(canUndo);
}

void AbstractEditor::onCanRedoChanged(bool canRedo)
{
    if (this == Core::EditorManager::currentEditor())
        d->actionHandler->redoAction()->setEnabled(canRedo);
}

void AbstractEditor::onTreeModelReset()
{
    updateSelectedArea(NOTHING_SELECTED);
}

void AbstractEditor::onTreeViewSelectionChanged(const QItemSelection &selected,
                                                const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    updateSelectedArea(TREE_VIEW_SELECTED);
}

void AbstractEditor::onTreeViewActivated()
{
    updateSelectedArea(TREE_VIEW_SELECTED);
}

void AbstractEditor::onTreeViewDoubleClicked(const QModelIndex &index)
{
    ExtDocumentController *documentController = d->document->documentController();

    QModelIndex treeModelIndex = d->modelTreeView->mapToSourceModelIndex(index);
    qmt::MElement *melement = documentController->getTreeModel()->getElement(treeModelIndex);
    // double click on package is already used for toggeling tree
    if (melement && !dynamic_cast<qmt::MPackage *>(melement))
        documentController->elementTasks()->openElement(melement);
}

void AbstractEditor::onCurrentDiagramChanged(const qmt::MDiagram *diagram)
{
    if (diagram) {
        QTC_CHECK(diagram == d->document->documentController()->getDiagramsManager()->getCurrentDiagram());
        updateSelectedArea(DIAGRAM_SELECTED);
    } else {
        updateSelectedArea(NOTHING_SELECTED);
    }
}

void AbstractEditor::onDiagramActivated(const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    updateSelectedArea(DIAGRAM_SELECTED);
}

void AbstractEditor::onDiagramClipboardChanged(bool isEmpty)
{
    Q_UNUSED(isEmpty);

    if (this == Core::EditorManager::currentEditor())
        updateSelectedArea(d->selectedArea);
}

void AbstractEditor::onNewElementCreated(qmt::DElement *element, qmt::MDiagram *diagram)
{
    if (diagram == editorDiagram()) {
        ExtDocumentController *documentController = d->document->documentController();

        documentController->getDiagramsManager()->getDiagramSceneModel(diagram)->selectElement(element);
        qmt::MElement *melement = documentController->getModelController()->findElement(element->getModelUid());
        if (!(melement && melement->getFlags().testFlag(qmt::MElement::REVERSE_ENGINEERED)))
            metaObject()->invokeMethod(this, "onEditSelectedElement", Qt::QueuedConnection);
    }
}

void AbstractEditor::onDiagramSelectionChanged(const qmt::MDiagram *diagram)
{
    if (diagram == editorDiagram())
        updateSelectedArea(DIAGRAM_SELECTED);
}

void AbstractEditor::onDiagramModified(const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    updateSelectedArea(d->selectedArea);
}

void AbstractEditor::onEditSelectedElement()
{
    // TODO introduce similar method for selected elements in model tree
    // currently this method is called on adding new elements in model tree
    // but the method is a no-op in that case.
    qmt::MDiagram *diagram = d->propertiesView->getSelectedDiagram();
    QList<qmt::DElement *> elements = d->propertiesView->getSelectedDiagramElements();
    if (diagram && !elements.isEmpty()) {
        qmt::DElement *element = elements.at(0);
        if (element) {
            qmt::DiagramSceneModel *diagramSceneModel = d->document->documentController()->getDiagramsManager()->getDiagramSceneModel(diagram);
            if (diagramSceneModel->isElementEditable(element)) {
                diagramSceneModel->editElement(element);
                return;
            }
        }
        d->propertiesView->editSelectedElement();
    }
}

void AbstractEditor::onRightSplitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);

    d->uiController->onRightSplitterChanged(d->rightSplitter->saveState());
}

void AbstractEditor::onRightSplitterChanged(const QByteArray &state)
{
    d->rightSplitter->restoreState(state);
}

void AbstractEditor::onRightHorizSplitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);

    d->uiController->onRightHorizSplitterChanged(d->rightHorizSplitter->saveState());
}

void AbstractEditor::onRightHorizSplitterChanged(const QByteArray &state)
{
    d->rightHorizSplitter->restoreState(state);
}

void AbstractEditor::initToolbars()
{
    QHash<QString, QWidget *> toolBars;
    // TODO add toolbars sorted by prio
    qmt::DocumentController *documentController = d->document->documentController();
    qmt::StereotypeController *stereotypeController = documentController->getStereotypeController();
    foreach (const qmt::Toolbar &toolbar, stereotypeController->getToolbars()) {
        QWidget *toolBar = toolBars.value(toolbar.getId());
        QLayout *toolBarLayout = 0;
        if (!toolBar) {
            toolBar = new QWidget(d->leftToolBox);
            toolBarLayout = new QVBoxLayout(toolBar);
            toolBarLayout->setContentsMargins(2, 2, 2, 2);
            toolBarLayout->setSpacing(6);
            d->leftToolBox->addItem(toolBar, toolbar.getId());
            toolBars.insert(toolbar.getId(), toolBar);
        } else {
            toolBarLayout = toolBar->layout();
            QTC_ASSERT(toolBarLayout, return);
        }
        foreach (const qmt::Toolbar::Tool &tool, toolbar.getTools()) {
            switch (tool._tool_type) {
            case qmt::Toolbar::TOOLTYPE_TOOL:
            {
                QString iconPath;
                qmt::StereotypeIcon::Element stereotypeIconElement = qmt::StereotypeIcon::ELEMENT_ANY;
                qmt::StyleEngine::ElementType styleEngineElementType = qmt::StyleEngine::TYPE_OTHER;
                if (tool._element_type == QLatin1String(qmt::ELEMENT_TYPE_PACKAGE)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/package.png");
                    stereotypeIconElement = qmt::StereotypeIcon::ELEMENT_PACKAGE;
                    styleEngineElementType = qmt::StyleEngine::TYPE_PACKAGE;
                } else if (tool._element_type == QLatin1String(qmt::ELEMENT_TYPE_COMPONENT)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/component.png");
                    stereotypeIconElement = qmt::StereotypeIcon::ELEMENT_COMPONENT;
                    styleEngineElementType = qmt::StyleEngine::TYPE_COMPONENT;
                } else if (tool._element_type == QLatin1String(qmt::ELEMENT_TYPE_CLASS)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/class.png");
                    stereotypeIconElement = qmt::StereotypeIcon::ELEMENT_CLASS;
                    styleEngineElementType = qmt::StyleEngine::TYPE_CLASS;
                } else if (tool._element_type == QLatin1String(qmt::ELEMENT_TYPE_ITEM)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/item.png");
                    stereotypeIconElement = qmt::StereotypeIcon::ELEMENT_ITEM;
                    styleEngineElementType = qmt::StyleEngine::TYPE_ITEM;
                } else if (tool._element_type == QLatin1String(qmt::ELEMENT_TYPE_ANNOTATION)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/annotation.png");
                    styleEngineElementType = qmt::StyleEngine::TYPE_ANNOTATION;
                } else if (tool._element_type == QLatin1String(qmt::ELEMENT_TYPE_BOUNDARY)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/boundary.png");
                    styleEngineElementType = qmt::StyleEngine::TYPE_BOUNDARY;
                }
                QIcon icon;
                if (!tool._stereotype.isEmpty() && stereotypeIconElement != qmt::StereotypeIcon::ELEMENT_ANY) {
                    const qmt::Style *style = documentController->getStyleController()->adaptStyle(styleEngineElementType);
                    icon = stereotypeController->createIcon(
                                stereotypeIconElement, QStringList() << tool._stereotype,
                                QString(), style, QSize(48, 48), QMarginsF(3.0, 2.0, 3.0, 4.0));
                }
                if (icon.isNull())
                    icon = QIcon(iconPath);
                if (!icon.isNull()) {
                    toolBarLayout->addWidget(new DragTool(icon, tool._name, tool._element_type,
                                                          tool._stereotype, toolBar));
                }
                break;
            }
            case qmt::Toolbar::TOOLTYPE_SEPARATOR:
            {
                auto horizLine1 = new QFrame(d->leftToolBox);
                horizLine1->setFrameShape(QFrame::HLine);
                toolBarLayout->addWidget(horizLine1);
                break;
            }
            }
        }
    }

    // fallback if no toolbar was defined
    if (!toolBars.isEmpty()) {
        QString generalId = QStringLiteral("General");
        auto toolBar = new QWidget(d->leftToolBox);
        auto toolBarLayout = new QVBoxLayout(toolBar);
        toolBarLayout->setContentsMargins(2, 2, 2, 2);
        toolBarLayout->setSpacing(6);
        d->leftToolBox->insertItem(0, toolBar, generalId);
        toolBars.insert(generalId, toolBar);
        toolBarLayout->addWidget(
                    new DragTool(QIcon(QStringLiteral(":/modelinglib/48x48/package.png")),
                                 tr("Package"), QLatin1String(qmt::ELEMENT_TYPE_PACKAGE),
                                 QString(), toolBar));
        toolBarLayout->addWidget(
                    new DragTool(QIcon(QStringLiteral(":/modelinglib/48x48/component.png")),
                                 tr("Component"), QLatin1String(qmt::ELEMENT_TYPE_COMPONENT),
                                 QString(), toolBar));
        toolBarLayout->addWidget(
                    new DragTool(QIcon(QStringLiteral(":/modelinglib/48x48/class.png")),
                                 tr("Class"), QLatin1String(qmt::ELEMENT_TYPE_CLASS),
                                 QString(), toolBar));
        toolBarLayout->addWidget(
                    new DragTool(QIcon(QStringLiteral(":/modelinglib/48x48/item.png")),
                                 tr("Item"), QLatin1String(qmt::ELEMENT_TYPE_ITEM),
                                 QString(), toolBar));
        auto horizLine1 = new QFrame(d->leftToolBox);
        horizLine1->setFrameShape(QFrame::HLine);
        toolBarLayout->addWidget(horizLine1);
        toolBarLayout->addWidget(
                    new DragTool(QIcon(QStringLiteral(":/modelinglib/48x48/annotation.png")),
                                 tr("Annotation"), QLatin1String(qmt::ELEMENT_TYPE_ANNOTATION),
                                 QString(), toolBar));
        toolBarLayout->addWidget(
                    new DragTool(QIcon(QStringLiteral(":/modelinglib/48x48/boundary.png")),
                                 tr("Boundary"), QLatin1String(qmt::ELEMENT_TYPE_BOUNDARY),
                                 QString(), toolBar));
    }

    // add stretch to all layouts and calculate width of tool bar
    int maxWidth = 48;
    foreach (QWidget *toolBar, toolBars) {
        QTC_ASSERT(toolBar, continue);
        auto layout = dynamic_cast<QBoxLayout *>(toolBar->layout());
        QTC_ASSERT(layout, continue);
        layout->addStretch(1);
        toolBar->adjustSize();
        if (maxWidth < toolBar->width())
            maxWidth = toolBar->width();
    }
    d->leftToolBox->setFixedWidth(maxWidth);

    d->leftToolBox->setCurrentIndex(0);
}

} // namespace Internal
} // namespace ModelEditor
