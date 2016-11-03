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

#include "modeleditor.h"

#include "actionhandler.h"
#include "diagramsviewmanager.h"
#include "dragtool.h"
#include "editordiagramview.h"
#include "elementtasks.h"
#include "extdocumentcontroller.h"
#include "extpropertiesmview.h"
#include "modeldocument.h"
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
#include "qmt/tasks/finddiagramvisitor.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <utils/fadingindicator.h>
#include <utils/styledbar.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QImageWriter>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTimer>
#include <QToolBox>
#include <QUndoStack>
#include <QVBoxLayout>
#include <QMenu>

#include <algorithm>

namespace ModelEditor {
namespace Internal {

static const char PROPERTYNAME_TOOLBARID[] = "ToolbarId";
static const double ZOOM_FACTOR = 1.05;

class ModelEditor::ModelEditorPrivate
{
public:
    UiController *uiController = 0;
    ActionHandler *actionHandler = 0;
    ModelDocument *document = 0;
    qmt::PropertiesView *propertiesView = 0;
    Core::MiniSplitter *rightSplitter = 0;
    QWidget *leftGroup = 0;
    QHBoxLayout *leftGroupLayout = 0;
    QToolBox *leftToolBox = 0;
    QStackedWidget *diagramStack = 0;
    EditorDiagramView *diagramView = 0;
    QLabel *noDiagramLabel = 0;
    DiagramsViewManager *diagramsViewManager = 0;
    Core::MiniSplitter *rightHorizSplitter = 0;
    qmt::ModelTreeView *modelTreeView = 0;
    qmt::TreeModelManager *modelTreeViewServant = 0;
    QScrollArea *propertiesScrollArea = 0;
    QWidget *propertiesGroupWidget = 0;
    QWidget *toolbar = 0;
    QComboBox *diagramSelector = 0;
    SelectedArea selectedArea = SelectedArea::Nothing;
    QString lastExportDirPath;
    QAction *syncBrowserWithDiagramAction = 0;
    QAction *syncDiagramWithBrowserAction = 0;
    QAction *syncEachOtherAction = 0;
};

ModelEditor::ModelEditor(UiController *uiController, ActionHandler *actionHandler, QWidget *parent)
    : IEditor(parent),
      d(new ModelEditorPrivate)
{
    setContext(Core::Context(Constants::MODEL_EDITOR_ID));
    d->uiController = uiController;
    d->actionHandler = actionHandler;
    d->document = new ModelDocument(this);
    connect(d->document, &ModelDocument::contentSet, this, &ModelEditor::onContentSet);
    init(parent);
}

ModelEditor::~ModelEditor()
{
    closeCurrentDiagram(false);
    delete d->toolbar;
    delete d;
}

Core::IDocument *ModelEditor::document()
{
    return d->document;
}

QWidget *ModelEditor::toolBar()
{
    return d->toolbar;
}

QByteArray ModelEditor::saveState() const
{
    return saveState(currentDiagram());
}

bool ModelEditor::restoreState(const QByteArray &state)
{
    QDataStream stream(state);
    int version = 0;
    stream >> version;
    if (version >= 1) {
        qmt::Uid uid;
        stream >> uid;
        if (version >= 2) {
            bool sync = false;
            bool syncBrowserWithDiagram = false;
            bool syncDiagramWithBrowser = false;
            bool syncEachOther = false;
            stream >> sync >> syncBrowserWithDiagram >> syncDiagramWithBrowser >> syncEachOther;
            d->actionHandler->synchronizeBrowserAction()->setChecked(sync);
            d->syncBrowserWithDiagramAction->setChecked(
                        syncBrowserWithDiagram || (!syncDiagramWithBrowser && !syncEachOther));
            d->syncDiagramWithBrowserAction->setChecked(syncDiagramWithBrowser);
            d->syncEachOtherAction->setChecked(syncEachOther);
        }
        if (uid.isValid()) {
            qmt::MDiagram *diagram = d->document->documentController()->modelController()->findObject<qmt::MDiagram>(uid);
            if (diagram) {
                openDiagram(diagram, false);
                return true;
            }
        }
    }
    return false;
}

void ModelEditor::init(QWidget *parent)
{
    // create and configure properties view
    d->propertiesView = new qmt::PropertiesView(this);

    // create and configure editor ui
    d->rightSplitter = new Core::MiniSplitter(parent);
    connect(d->rightSplitter, &QSplitter::splitterMoved,
            this, &ModelEditor::onRightSplitterMoved);
    connect(d->uiController, &UiController::rightSplitterChanged,
            this, &ModelEditor::onRightSplitterChanged);

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
    QFont font = d->leftToolBox->font();
    font.setPointSizeF(font.pointSizeF() * 0.8);
    d->leftToolBox->setFont(font);
    connect(d->leftToolBox, &QToolBox::currentChanged, this, &ModelEditor::onToolbarSelectionChanged);

    d->diagramStack = new QStackedWidget(d->leftGroup);

    d->diagramView = new EditorDiagramView(d->diagramStack);
    d->diagramStack->addWidget(d->diagramView);

    d->noDiagramLabel = new QLabel(d->diagramStack);
    const QString placeholderText =
            tr("<html><body style=\"color:#909090; font-size:14px\">"
               "<div align='center'>"
               "<div style=\"font-size:20px\">Open a diagram</div>"
               "<table><tr><td>"
               "<hr/>"
               "<div style=\"margin-top: 5px\">&bull; Double-click on diagram in model tree</div>"
               "<div style=\"margin-top: 5px\">&bull; Select \"Open Diagram\" from package's context menu in model tree</div>"
               "</td></tr></table>"
               "</div>"
               "</body></html>");
    d->noDiagramLabel->setText(placeholderText);
    d->diagramStack->addWidget(d->noDiagramLabel);
    d->diagramStack->setCurrentWidget(d->noDiagramLabel);

    d->leftGroupLayout->addWidget(d->leftToolBox, 0);
    auto frame = new QFrame(d->leftGroup);
    frame->setFrameShape(QFrame::VLine);
    d->leftGroupLayout->addWidget(frame, 0);
    d->leftGroupLayout->addWidget(d->diagramStack, 1);

    d->rightHorizSplitter = new Core::MiniSplitter(d->rightSplitter);
    d->rightHorizSplitter->setOrientation(Qt::Vertical);
    connect(d->rightHorizSplitter, &QSplitter::splitterMoved,
            this, &ModelEditor::onRightHorizSplitterMoved);
    connect(d->uiController, &UiController::rightHorizSplitterChanged,
            this, &ModelEditor::onRightHorizSplitterChanged);

    d->modelTreeView = new qmt::ModelTreeView(d->rightHorizSplitter);
    d->modelTreeView->setFrameShape(QFrame::NoFrame);

    d->modelTreeViewServant = new qmt::TreeModelManager(this);
    d->modelTreeViewServant->setModelTreeView(d->modelTreeView);

    d->propertiesScrollArea = new QScrollArea(d->rightHorizSplitter);
    d->propertiesScrollArea->setFrameShape(QFrame::NoFrame);
    d->propertiesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->propertiesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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

    // restore splitter sizes after any stretch factor has been set as fallback
    if (d->uiController->hasRightSplitterState())
        d->rightSplitter->restoreState(d->uiController->rightSplitterState());
    if (d->uiController->hasRightHorizSplitterState())
        d->rightHorizSplitter->restoreState(d->uiController->rightHorizSplitterState());

    // create and configure toolbar
    d->toolbar = new QWidget();
    auto toolbarLayout = new QHBoxLayout(d->toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(0);

    auto openParentButton = new Core::CommandButton(Constants::OPEN_PARENT_DIAGRAM, d->toolbar);
    openParentButton->setDefaultAction(d->actionHandler->openParentDiagramAction());
    toolbarLayout->addWidget(openParentButton);

    d->diagramSelector = new QComboBox(d->toolbar);
    connect(d->diagramSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &ModelEditor::onDiagramSelectorSelected);
    toolbarLayout->addWidget(d->diagramSelector, 1);
    toolbarLayout->addStretch(1);

    toolbarLayout->addWidget(
                createToolbarCommandButton(Constants::ACTION_ADD_PACKAGE, [this]() { onAddPackage(); },
    QIcon(QStringLiteral(":/modelinglib/48x48/package.png")),
    tr("Add Package"), d->toolbar));
    toolbarLayout->addWidget(
                createToolbarCommandButton(Constants::ACTION_ADD_COMPONENT, [this]() { onAddComponent(); },
    QIcon(QStringLiteral(":/modelinglib/48x48/component.png")),
    tr("Add Component"), d->toolbar));
    toolbarLayout->addWidget(
                createToolbarCommandButton(Constants::ACTION_ADD_CLASS, [this]() { onAddClass(); },
    QIcon(QStringLiteral(":/modelinglib/48x48/class.png")),
    tr("Add Class"), d->toolbar));
    toolbarLayout->addWidget(
                createToolbarCommandButton(Constants::ACTION_ADD_CANVAS_DIAGRAM, [this]() { onAddCanvasDiagram(); },
    QIcon(QStringLiteral(":/modelinglib/48x48/canvas-diagram.png")),
    tr("Add Canvas Diagram"), d->toolbar));
    toolbarLayout->addSpacing(20);

    auto syncToggleButton = new Core::CommandButton(Constants::ACTION_SYNC_BROWSER, d->toolbar);
    syncToggleButton->setDefaultAction(d->actionHandler->synchronizeBrowserAction());
    QMenu *syncMenu = new QMenu(syncToggleButton);
    QActionGroup *syncGroup = new QActionGroup(syncMenu);
    d->syncBrowserWithDiagramAction = syncMenu->addAction(QStringLiteral("Synchronize Structure with Diagram"));
    d->syncBrowserWithDiagramAction->setCheckable(true);
    d->syncBrowserWithDiagramAction->setActionGroup(syncGroup);
    d->syncDiagramWithBrowserAction = syncMenu->addAction(QStringLiteral("Synchronize Diagram with Structure"));
    d->syncDiagramWithBrowserAction->setCheckable(true);
    d->syncDiagramWithBrowserAction->setActionGroup(syncGroup);
    d->syncEachOtherAction = syncMenu->addAction(QStringLiteral("Keep Synchronized"));
    d->syncEachOtherAction->setCheckable(true);
    d->syncEachOtherAction->setActionGroup(syncGroup);
    syncToggleButton->setMenu(syncMenu);
    d->syncBrowserWithDiagramAction->setChecked(true);
    toolbarLayout->addWidget(syncToggleButton);
}

void ModelEditor::initDocument()
{
    initToolbars();

    ExtDocumentController *documentController = d->document->documentController();

    d->diagramView->setPxNodeController(documentController->pxNodeController());

    QTC_CHECK(!d->diagramsViewManager);
    d->diagramsViewManager = new DiagramsViewManager(this);
    //connect(diagramsViewManager, &DiagramsViewManager::someDiagramOpened,
    //        documentController->diagramsManager(), &qmt::DiagramsManager::someDiagramOpened);
    connect(d->diagramsViewManager, &DiagramsViewManager::openNewDiagram,
            this, &ModelEditor::showDiagram);
    connect(d->diagramsViewManager, &DiagramsViewManager::closeOpenDiagram,
            this, &ModelEditor::closeDiagram);
    connect(d->diagramsViewManager, &DiagramsViewManager::closeAllOpenDiagrams,
            this, &ModelEditor::closeAllDiagrams);
    documentController->diagramsManager()->setDiagramsView(d->diagramsViewManager);

    d->propertiesView->setDiagramController(documentController->diagramController());
    d->propertiesView->setModelController(documentController->modelController());
    d->propertiesView->setStereotypeController(documentController->stereotypeController());
    d->propertiesView->setStyleController(documentController->styleController());
    d->propertiesView->setMViewFactory([=](qmt::PropertiesView *view) {
        auto extView = new ExtPropertiesMView(view);
        extView->setProjectController(documentController->projectController());
        return extView;
    });

    d->modelTreeView->setTreeModel(documentController->sortedTreeModel());
    d->modelTreeView->setElementTasks(documentController->elementTasks());

    d->modelTreeViewServant->setTreeModel(documentController->treeModel());

    connect(documentController, &qmt::DocumentController::diagramClipboardChanged,
            this, &ModelEditor::onDiagramClipboardChanged, Qt::QueuedConnection);
    connect(documentController->undoController()->undoStack(), &QUndoStack::canUndoChanged,
            this, &ModelEditor::onCanUndoChanged, Qt::QueuedConnection);
    connect(documentController->undoController()->undoStack(), &QUndoStack::canRedoChanged,
            this, &ModelEditor::onCanRedoChanged, Qt::QueuedConnection);
    connect(documentController->treeModel(), &qmt::TreeModel::modelReset,
            this, &ModelEditor::onTreeModelReset, Qt::QueuedConnection);
    connect(documentController->diagramController(), &qmt::DiagramController::modified,
            this, &ModelEditor::onDiagramModified, Qt::QueuedConnection);
    connect(documentController->diagramsManager(), &qmt::DiagramsManager::diagramSelectionChanged,
            this, &ModelEditor::onDiagramSelectionChanged, Qt::QueuedConnection);
    connect(documentController->diagramsManager(), &qmt::DiagramsManager::diagramActivated,
            this, &ModelEditor::onDiagramActivated, Qt::QueuedConnection);
    connect(documentController->diagramSceneController(), &qmt::DiagramSceneController::newElementCreated,
            this, &ModelEditor::onNewElementCreated, Qt::QueuedConnection);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &ModelEditor::onCurrentEditorChanged, Qt::QueuedConnection);

    connect(d->diagramView, &EditorDiagramView::zoomIn,
            this, &ModelEditor::zoomIn);
    connect(d->diagramView, &EditorDiagramView::zoomOut,
            this, &ModelEditor::zoomOut);

    connect(d->modelTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ModelEditor::onTreeViewSelectionChanged, Qt::QueuedConnection);
    connect(d->modelTreeView, &qmt::ModelTreeView::treeViewActivated,
            this, &ModelEditor::onTreeViewActivated, Qt::QueuedConnection);
    connect(d->modelTreeView, &QAbstractItemView::doubleClicked,
            this, &ModelEditor::onTreeViewDoubleClicked, Qt::QueuedConnection);

    connect(documentController->modelController(), &qmt::ModelController::endMoveObject,
            this, &ModelEditor::updateDiagramSelector);
    connect(documentController->modelController(), &qmt::ModelController::endRemoveObject,
            this, &ModelEditor::updateDiagramSelector);
    connect(documentController->modelController(), &qmt::ModelController::endResetModel,
            this, &ModelEditor::updateDiagramSelector);
    connect(documentController->modelController(), &qmt::ModelController::endUpdateObject,
            this, &ModelEditor::updateDiagramSelector);

    updateSelectedArea(SelectedArea::Nothing);
}

qmt::MDiagram *ModelEditor::currentDiagram() const
{
    if (!d->diagramView->diagramSceneModel())
        return 0;
    return d->diagramView->diagramSceneModel()->diagram();
}

void ModelEditor::showDiagram(qmt::MDiagram *diagram)
{
    openDiagram(diagram, true);
}

void ModelEditor::undo()
{
    d->document->documentController()->undoController()->undoStack()->undo();
}

void ModelEditor::redo()
{
    d->document->documentController()->undoController()->undoStack()->redo();
}

void ModelEditor::cut()
{
    ExtDocumentController *documentController = d->document->documentController();

    switch (d->selectedArea) {
    case SelectedArea::Nothing:
        break;
    case SelectedArea::Diagram:
        documentController->cutFromDiagram(currentDiagram());
        break;
    case SelectedArea::TreeView:
        documentController->cutFromModel(d->modelTreeViewServant->selectedObjects());
        break;
    }
}

void ModelEditor::copy()
{
    ExtDocumentController *documentController = d->document->documentController();

    switch (d->selectedArea) {
    case SelectedArea::Nothing:
        break;
    case SelectedArea::Diagram:
        if (documentController->hasDiagramSelection(currentDiagram()))
            documentController->copyFromDiagram(currentDiagram());
        else
            documentController->copyDiagram(currentDiagram());
        break;
    case SelectedArea::TreeView:
        documentController->copyFromModel(d->modelTreeViewServant->selectedObjects());
        break;
    }
}

void ModelEditor::paste()
{
    ExtDocumentController *documentController = d->document->documentController();

    switch (d->selectedArea) {
    case SelectedArea::Nothing:
        break;
    case SelectedArea::Diagram:
        documentController->pasteIntoDiagram(currentDiagram());
        break;
    case SelectedArea::TreeView:
        documentController->pasteIntoModel(d->modelTreeViewServant->selectedObject());
        break;
    }
}

void ModelEditor::removeSelectedElements()
{
    switch (d->selectedArea) {
    case SelectedArea::Nothing:
        break;
    case SelectedArea::Diagram:
        d->document->documentController()->removeFromDiagram(currentDiagram());
        break;
    case SelectedArea::TreeView:
        break;
    }
}

void ModelEditor::deleteSelectedElements()
{
    ExtDocumentController *documentController = d->document->documentController();

    switch (d->selectedArea) {
    case SelectedArea::Nothing:
        break;
    case SelectedArea::Diagram:
        documentController->deleteFromDiagram(currentDiagram());
        break;
    case SelectedArea::TreeView:
        documentController->deleteFromModel(d->modelTreeViewServant->selectedObjects());
        break;
    }
}

void ModelEditor::selectAll()
{
    d->document->documentController()->selectAllOnDiagram(currentDiagram());
}

void ModelEditor::openParentDiagram()
{
    qmt::MDiagram *diagram = currentDiagram();
    if (diagram)
        d->document->documentController()->elementTasks()->openParentDiagram(diagram);
}

void ModelEditor::editProperties()
{
    d->propertiesView->editSelectedElement();
}

void ModelEditor::editSelectedItem()
{
    onEditSelectedElement();
}

void ModelEditor::exportDiagram()
{
    qmt::MDiagram *diagram = currentDiagram();
    if (diagram) {
        if (d->lastExportDirPath.isEmpty())
            d->lastExportDirPath = d->document->filePath().toFileInfo().canonicalPath();
        QString filter = tr("Images (*.png *.jpeg *.jpg *.tif *.tiff);;PDF (*.pdf)");
#ifndef QT_NO_SVG
        filter += tr(";;SVG (*.svg)");
#endif // QT_NO_SVG
        QString fileName = QFileDialog::getSaveFileName(
                    Core::ICore::dialogParent(),
                    tr("Export Diagram"), d->lastExportDirPath, filter);
        if (!fileName.isEmpty()) {
            qmt::DocumentController *documentController = d->document->documentController();
            qmt::DiagramSceneModel *sceneModel = documentController->diagramsManager()->diagramSceneModel(diagram);
            bool success = false;
            QString suffix = QFileInfo(fileName).suffix().toLower();
            // TODO use QFileDialog::selectedNameFilter() as fallback if no suffix is given
            if (suffix.isEmpty()) {
                suffix = QStringLiteral("png");
                fileName += QStringLiteral(".png");
            }
            if (suffix == QStringLiteral("pdf"))
                success = sceneModel->exportPdf(fileName);
#ifndef QT_NO_SVG
            else if (suffix == QStringLiteral("svg"))
                success = sceneModel->exportSvg(fileName);
#endif // QT_NO_SVG
            else
                success = sceneModel->exportImage(fileName);
            if (success)
                d->lastExportDirPath = QFileInfo(fileName).canonicalPath();
            else
                QMessageBox::critical(Core::ICore::dialogParent(), tr("Exporting Diagram Failed"),
                                      tr("Exporting the diagram into file<br>\"%1\"<br>failed.").arg(fileName));
        }
    }
}

void ModelEditor::zoomIn()
{
    QTransform transform = d->diagramView->transform();
    transform.scale(ZOOM_FACTOR, ZOOM_FACTOR);
    d->diagramView->setTransform(transform);
    showZoomIndicator();
}

void ModelEditor::zoomOut()
{
    QTransform transform = d->diagramView->transform();
    transform.scale(1.0 / ZOOM_FACTOR, 1.0 / ZOOM_FACTOR);
    d->diagramView->setTransform(transform);
    showZoomIndicator();
}

void ModelEditor::resetZoom()
{
    d->diagramView->setTransform(QTransform());
    showZoomIndicator();
}

qmt::MPackage *ModelEditor::guessSelectedPackage() const
{
    qmt::MPackage *package = 0;
    switch (d->selectedArea) {
    case SelectedArea::Nothing:
        package = d->modelTreeViewServant->selectedPackage();
        break;
    case SelectedArea::Diagram:
    {
        qmt::DocumentController *documentController = d->document->documentController();
        qmt::DiagramsManager *diagramsManager = documentController->diagramsManager();
        qmt::MDiagram *diagram = currentDiagram();
        qmt::DSelection selection = diagramsManager->diagramSceneModel(diagram)->selectedElements();
        if (selection.indices().size() == 1) {
            qmt::DPackage *diagramElement = documentController->diagramController()->findElement<qmt::DPackage>(selection.indices().at(0).elementKey(), diagram);
            if (diagramElement)
                package = documentController->modelController()->findObject<qmt::MPackage>(diagramElement->modelUid());
        }
        break;
    }
    case SelectedArea::TreeView:
        package = d->modelTreeViewServant->selectedPackage();
        break;
    }
    return package;
}

void ModelEditor::updateSelectedArea(SelectedArea selectedArea)
{
    d->selectedArea = selectedArea;

    qmt::DocumentController *documentController = d->document->documentController();
    bool canCutCopyDelete = false;
    bool canRemove = false;
    bool canPaste = false;
    bool canSelectAll = false;
    bool canCopyDiagram = false;
    bool canOpenParentDiagram = false;
    bool canExportDiagram = false;
    QList<qmt::MElement *> propertiesModelElements;
    QList<qmt::DElement *> propertiesDiagramElements;
    qmt::MDiagram *propertiesDiagram = 0;

    qmt::MDiagram *activeDiagram = currentDiagram();
    switch (d->selectedArea) {
    case SelectedArea::Nothing:
        canSelectAll = activeDiagram && !activeDiagram->diagramElements().isEmpty();
        canExportDiagram = activeDiagram != 0;
        break;
    case SelectedArea::Diagram:
    {
        if (activeDiagram) {
            canExportDiagram = true;
            bool hasSelection = documentController->diagramsManager()->diagramSceneModel(activeDiagram)->hasSelection();
            canCutCopyDelete = hasSelection;
            canRemove = hasSelection;
            canPaste = !documentController->isDiagramClipboardEmpty();
            canSelectAll = !activeDiagram->diagramElements().isEmpty();
            canCopyDiagram = !hasSelection;
            if (hasSelection) {
                qmt::DSelection selection = documentController->diagramsManager()->diagramSceneModel(activeDiagram)->selectedElements();
                if (!selection.isEmpty()) {
                    foreach (qmt::DSelection::Index index, selection.indices()) {
                        qmt::DElement *diagramElement = documentController->diagramController()->findElement(index.elementKey(), activeDiagram);
                        if (diagramElement)
                            propertiesDiagramElements.append(diagramElement);
                    }
                    if (!propertiesDiagramElements.isEmpty())
                        propertiesDiagram = activeDiagram;
                }
            }
        }
        break;
    }
    case SelectedArea::TreeView:
    {
        canExportDiagram = activeDiagram != 0;
        bool hasSelection = !d->modelTreeViewServant->selectedObjects().isEmpty();
        bool hasSingleSelection = d->modelTreeViewServant->selectedObjects().indices().size() == 1;
        canCutCopyDelete = hasSelection && !d->modelTreeViewServant->isRootPackageSelected();
        canPaste =  hasSingleSelection && !documentController->isModelClipboardEmpty();
        canSelectAll = activeDiagram && !activeDiagram->diagramElements().isEmpty();
        QModelIndexList indexes = d->modelTreeView->selectedSourceModelIndexes();
        if (!indexes.isEmpty()) {
            foreach (const QModelIndex &propertiesIndex, indexes) {
                if (propertiesIndex.isValid()) {
                    qmt::MElement *modelElement = documentController->treeModel()->element(propertiesIndex);
                    if (modelElement)
                        propertiesModelElements.append(modelElement);
                }
            }
        }
        break;
    }
    }
    canOpenParentDiagram = d->document->documentController()->elementTasks()->hasParentDiagram(currentDiagram());

    d->actionHandler->cutAction()->setEnabled(canCutCopyDelete);
    d->actionHandler->copyAction()->setEnabled(canCutCopyDelete || canCopyDiagram);
    d->actionHandler->pasteAction()->setEnabled(canPaste);
    d->actionHandler->removeAction()->setEnabled(canRemove);
    d->actionHandler->deleteAction()->setEnabled(canCutCopyDelete);
    d->actionHandler->selectAllAction()->setEnabled(canSelectAll);
    d->actionHandler->openParentDiagramAction()->setEnabled(canOpenParentDiagram);
    d->actionHandler->exportDiagramAction()->setEnabled(canExportDiagram);

    if (!propertiesModelElements.isEmpty())
        showProperties(propertiesModelElements);
    else if (!propertiesDiagramElements.isEmpty())
        showProperties(propertiesDiagram, propertiesDiagramElements);
    else
        clearProperties();
}

void ModelEditor::showProperties(const QList<qmt::MElement *> &modelElements)
{
    if (modelElements != d->propertiesView->selectedModelElements()) {
        clearProperties();
        if (modelElements.size() > 0) {
            d->propertiesView->setSelectedModelElements(modelElements);
            d->propertiesGroupWidget = d->propertiesView->widget();
            d->propertiesScrollArea->setWidget(d->propertiesGroupWidget);
        }
    }
}

void ModelEditor::showProperties(qmt::MDiagram *diagram,
                                 const QList<qmt::DElement *> &diagramElements)
{
    if (diagram != d->propertiesView->selectedDiagram()
            || diagramElements != d->propertiesView->selectedDiagramElements())
    {
        clearProperties();
        if (diagram && diagramElements.size() > 0) {
            d->propertiesView->setSelectedDiagramElements(diagramElements, diagram);
            d->propertiesGroupWidget = d->propertiesView->widget();
            d->propertiesScrollArea->setWidget(d->propertiesGroupWidget);
        }
    }
}

void ModelEditor::clearProperties()
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

void ModelEditor::expandModelTreeToDepth(int depth)
{
    d->modelTreeView->expandToDepth(depth);
}

QToolButton *ModelEditor::createToolbarCommandButton(const Core::Id &id, const std::function<void()> &slot,
                                                     const QIcon &icon, const QString &toolTipBase,
                                                     QWidget *parent)
{
    auto button = new Core::CommandButton(id, parent);
    auto action = new QAction(button);
    action->setIcon(icon);
    action->setToolTip(toolTipBase);
    button->setDefaultAction(action);
    //button->setIcon(icon);
    //button->setToolTipBase(toolTipBase);
    connect(button, &Core::CommandButton::clicked, this, slot);
    return button;
}

/*!
    Tries to change the \a button icon to the icon specified by \a name
    from the current theme. Returns \c true if icon is updated, \c false
    otherwise.
*/

bool ModelEditor::updateButtonIconByTheme(QAbstractButton *button, const QString &name)
{
    QTC_ASSERT(button, return false);
    QTC_ASSERT(!name.isEmpty(), return false);

    if (QIcon::hasThemeIcon(name)) {
        button->setIcon(QIcon::fromTheme(name));
        return true;
    }

    return false;
}

void ModelEditor::showZoomIndicator()
{
    int scale = int(d->diagramView->transform().map(QPointF(100, 100)).x() + 0.5);
    Utils::FadingIndicator::showText(d->diagramStack,
                                     QCoreApplication::translate("ModelEditor",
                                                                 "Zoom: %1%").arg(scale),
                                     Utils::FadingIndicator::SmallText);
}

void ModelEditor::onAddPackage()
{
    ExtDocumentController *documentController = d->document->documentController();

    qmt::MPackage *package = documentController->createNewPackage(d->modelTreeViewServant->selectedPackage());
    d->modelTreeView->selectFromSourceModelIndex(documentController->treeModel()->indexOf(package));
    QTimer::singleShot(0, this, [this]() { onEditSelectedElement(); });
}

void ModelEditor::onAddComponent()
{
    ExtDocumentController *documentController = d->document->documentController();

    qmt::MComponent *component = documentController->createNewComponent(d->modelTreeViewServant->selectedPackage());
    d->modelTreeView->selectFromSourceModelIndex(documentController->treeModel()->indexOf(component));
    QTimer::singleShot(0, this, [this]() { onEditSelectedElement(); });
}

void ModelEditor::onAddClass()
{
    ExtDocumentController *documentController = d->document->documentController();

    qmt::MClass *klass = documentController->createNewClass(d->modelTreeViewServant->selectedPackage());
    d->modelTreeView->selectFromSourceModelIndex(documentController->treeModel()->indexOf(klass));
    QTimer::singleShot(0, this, [this]() { onEditSelectedElement(); });
}

void ModelEditor::onAddCanvasDiagram()
{
    ExtDocumentController *documentController = d->document->documentController();

    qmt::MDiagram *diagram = documentController->createNewCanvasDiagram(d->modelTreeViewServant->selectedPackage());
    d->modelTreeView->selectFromSourceModelIndex(documentController->treeModel()->indexOf(diagram));
    QTimer::singleShot(0, this, [this]() { onEditSelectedElement(); });
}

void ModelEditor::onCurrentEditorChanged(Core::IEditor *editor)
{
    if (this == editor) {
        QUndoStack *undoStack = d->document->documentController()->undoController()->undoStack();
        d->actionHandler->undoAction()->setDisabled(!undoStack->canUndo());
        d->actionHandler->redoAction()->setDisabled(!undoStack->canRedo());

        updateSelectedArea(SelectedArea::Nothing);
    }
}

void ModelEditor::onCanUndoChanged(bool canUndo)
{
    if (this == Core::EditorManager::currentEditor())
        d->actionHandler->undoAction()->setEnabled(canUndo);
}

void ModelEditor::onCanRedoChanged(bool canRedo)
{
    if (this == Core::EditorManager::currentEditor())
        d->actionHandler->redoAction()->setEnabled(canRedo);
}

void ModelEditor::onTreeModelReset()
{
    updateSelectedArea(SelectedArea::Nothing);
}

void ModelEditor::onTreeViewSelectionChanged(const QItemSelection &selected,
                                             const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    synchronizeDiagramWithBrowser();
    updateSelectedArea(SelectedArea::TreeView);
}

void ModelEditor::onTreeViewActivated()
{
    updateSelectedArea(SelectedArea::TreeView);
}

void ModelEditor::onTreeViewDoubleClicked(const QModelIndex &index)
{
    ExtDocumentController *documentController = d->document->documentController();

    QModelIndex treeModelIndex = d->modelTreeView->mapToSourceModelIndex(index);
    qmt::MElement *melement = documentController->treeModel()->element(treeModelIndex);
    // double click on package is already used for toggeling tree
    if (melement && !dynamic_cast<qmt::MPackage *>(melement))
        documentController->elementTasks()->openElement(melement);
}

void ModelEditor::onCurrentDiagramChanged(const qmt::MDiagram *diagram)
{
    if (diagram == currentDiagram()) {
        updateSelectedArea(SelectedArea::Diagram);
    } else {
        updateSelectedArea(SelectedArea::Nothing);
    }
}

void ModelEditor::onDiagramActivated(const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    updateSelectedArea(SelectedArea::Diagram);
}

void ModelEditor::onDiagramClipboardChanged(bool isEmpty)
{
    Q_UNUSED(isEmpty);

    if (this == Core::EditorManager::currentEditor())
        updateSelectedArea(d->selectedArea);
}

void ModelEditor::onNewElementCreated(qmt::DElement *element, qmt::MDiagram *diagram)
{
    if (diagram == currentDiagram()) {
        ExtDocumentController *documentController = d->document->documentController();

        documentController->diagramsManager()->diagramSceneModel(diagram)->selectElement(element);
        qmt::MElement *melement = documentController->modelController()->findElement(element->modelUid());
        if (!(melement && melement->flags().testFlag(qmt::MElement::ReverseEngineered)))
            QTimer::singleShot(0, this, [this]() { onEditSelectedElement(); });
    }
}

void ModelEditor::onDiagramSelectionChanged(const qmt::MDiagram *diagram)
{
    if (diagram == currentDiagram()) {
        synchronizeBrowserWithDiagram(diagram);
        updateSelectedArea(SelectedArea::Diagram);
    }
}

void ModelEditor::onDiagramModified(const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    updateSelectedArea(d->selectedArea);
}

void ModelEditor::onRightSplitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);

    d->uiController->onRightSplitterChanged(d->rightSplitter->saveState());
}

void ModelEditor::onRightSplitterChanged(const QByteArray &state)
{
    d->rightSplitter->restoreState(state);
}

void ModelEditor::onRightHorizSplitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);

    d->uiController->onRightHorizSplitterChanged(d->rightHorizSplitter->saveState());
}

void ModelEditor::onRightHorizSplitterChanged(const QByteArray &state)
{
    d->rightHorizSplitter->restoreState(state);
}

void ModelEditor::onToolbarSelectionChanged()
{
    storeToolbarIdInDiagram(currentDiagram());
}

void ModelEditor::initToolbars()
{
    QHash<QString, QWidget *> toolBars;
    qmt::DocumentController *documentController = d->document->documentController();
    qmt::StereotypeController *stereotypeController = documentController->stereotypeController();
    QList<qmt::Toolbar> toolbars = stereotypeController->toolbars();
    std::stable_sort(toolbars.begin(), toolbars.end(),
                     [=](const qmt::Toolbar &lhs, const qmt::Toolbar &rhs) { return lhs.priority() > rhs.priority(); });
    foreach (const qmt::Toolbar &toolbar, toolbars) {
        QWidget *toolBar = toolBars.value(toolbar.id());
        QLayout *toolBarLayout = 0;
        if (!toolBar) {
            toolBar = new QWidget(d->leftToolBox);
            toolBar->setProperty(PROPERTYNAME_TOOLBARID, toolbar.id());
            toolBarLayout = new QVBoxLayout(toolBar);
            toolBarLayout->setContentsMargins(2, 2, 2, 2);
            toolBarLayout->setSpacing(6);
            // TODO add a user defined string attribute to toolbar as toolbar title
            d->leftToolBox->addItem(toolBar, toolbar.id());
            toolBars.insert(toolbar.id(), toolBar);
        } else {
            toolBarLayout = toolBar->layout();
            QTC_ASSERT(toolBarLayout, continue);
        }
        foreach (const qmt::Toolbar::Tool &tool, toolbar.tools()) {
            switch (tool.m_toolType) {
            case qmt::Toolbar::TooltypeTool:
            {
                QString iconPath;
                qmt::StereotypeIcon::Element stereotypeIconElement = qmt::StereotypeIcon::ElementAny;
                qmt::StyleEngine::ElementType styleEngineElementType = qmt::StyleEngine::TypeOther;
                if (tool.m_elementType == QLatin1String(qmt::ELEMENT_TYPE_PACKAGE)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/package.png");
                    stereotypeIconElement = qmt::StereotypeIcon::ElementPackage;
                    styleEngineElementType = qmt::StyleEngine::TypePackage;
                } else if (tool.m_elementType == QLatin1String(qmt::ELEMENT_TYPE_COMPONENT)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/component.png");
                    stereotypeIconElement = qmt::StereotypeIcon::ElementComponent;
                    styleEngineElementType = qmt::StyleEngine::TypeComponent;
                } else if (tool.m_elementType == QLatin1String(qmt::ELEMENT_TYPE_CLASS)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/class.png");
                    stereotypeIconElement = qmt::StereotypeIcon::ElementClass;
                    styleEngineElementType = qmt::StyleEngine::TypeClass;
                } else if (tool.m_elementType == QLatin1String(qmt::ELEMENT_TYPE_ITEM)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/item.png");
                    stereotypeIconElement = qmt::StereotypeIcon::ElementItem;
                    styleEngineElementType = qmt::StyleEngine::TypeItem;
                } else if (tool.m_elementType == QLatin1String(qmt::ELEMENT_TYPE_ANNOTATION)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/annotation.png");
                    styleEngineElementType = qmt::StyleEngine::TypeAnnotation;
                } else if (tool.m_elementType == QLatin1String(qmt::ELEMENT_TYPE_BOUNDARY)) {
                    iconPath = QStringLiteral(":/modelinglib/48x48/boundary.png");
                    styleEngineElementType = qmt::StyleEngine::TypeBoundary;
                }
                QIcon icon;
                if (!tool.m_stereotype.isEmpty() && stereotypeIconElement != qmt::StereotypeIcon::ElementAny) {
                    const qmt::Style *style = documentController->styleController()->adaptStyle(styleEngineElementType);
                    icon = stereotypeController->createIcon(
                                stereotypeIconElement, QStringList() << tool.m_stereotype,
                                QString(), style, QSize(48, 48), QMarginsF(3.0, 2.0, 3.0, 4.0));
                }
                if (icon.isNull())
                    icon = QIcon(iconPath);
                if (!icon.isNull()) {
                    toolBarLayout->addWidget(new DragTool(icon, tool.m_name, tool.m_elementType,
                                                          tool.m_stereotype, toolBar));
                }
                break;
            }
            case qmt::Toolbar::TooltypeSeparator:
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
        toolBar->setProperty(PROPERTYNAME_TOOLBARID, generalId);
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
        auto layout = qobject_cast<QBoxLayout *>(toolBar->layout());
        QTC_ASSERT(layout, continue);
        layout->addStretch(1);
        toolBar->adjustSize();
        if (maxWidth < toolBar->width())
            maxWidth = toolBar->width();
    }
    d->leftToolBox->setFixedWidth(maxWidth);

    d->leftToolBox->setCurrentIndex(0);
}

void ModelEditor::openDiagram(qmt::MDiagram *diagram, bool addToHistory)
{
    closeCurrentDiagram(addToHistory);
    if (diagram) {
        qmt::DiagramSceneModel *diagramSceneModel = d->document->documentController()->diagramsManager()->bindDiagramSceneModel(diagram);
        d->diagramView->setDiagramSceneModel(diagramSceneModel);
        d->diagramStack->setCurrentWidget(d->diagramView);
        updateSelectedArea(SelectedArea::Nothing);
        addDiagramToSelector(diagram);
        if (!diagram->toolbarId().isEmpty()) {
            for (int i = 0; i < d->leftToolBox->count(); ++i) {
                QWidget *widget = d->leftToolBox->widget(i);
                if (widget && widget->property(PROPERTYNAME_TOOLBARID).toString() == diagram->toolbarId()) {
                    d->leftToolBox->setCurrentIndex(i);
                    break;
                }
            }
        } else {
            storeToolbarIdInDiagram(diagram);
        }
    }
}

void ModelEditor::closeCurrentDiagram(bool addToHistory)
{
    ExtDocumentController *documentController = d->document->documentController();
    qmt::DiagramsManager *diagramsManager = documentController->diagramsManager();
    qmt::DiagramSceneModel *sceneModel = d->diagramView->diagramSceneModel();
    if (sceneModel) {
        qmt::MDiagram *diagram = sceneModel->diagram();
        if (diagram) {
            if (addToHistory)
                addToNavigationHistory(diagram);
            d->diagramStack->setCurrentWidget(d->noDiagramLabel);
            d->diagramView->setDiagramSceneModel(0);
            diagramsManager->unbindDiagramSceneModel(diagram);
        }
    }
}

void ModelEditor::closeDiagram(const qmt::MDiagram *diagram)
{
    ExtDocumentController *documentController = d->document->documentController();
    qmt::DiagramsManager *diagramsManager = documentController->diagramsManager();
    qmt::DiagramSceneModel *sceneModel = d->diagramView->diagramSceneModel();
    if (sceneModel && diagram == sceneModel->diagram()) {
        addToNavigationHistory(diagram);
        d->diagramStack->setCurrentWidget(d->noDiagramLabel);
        d->diagramView->setDiagramSceneModel(0);
        diagramsManager->unbindDiagramSceneModel(diagram);
    }
}

void ModelEditor::closeAllDiagrams()
{
    closeCurrentDiagram(true);
}

void ModelEditor::onContentSet()
{
    initDocument();

    // open diagram
    ExtDocumentController *documentController = d->document->documentController();
    qmt::MDiagram *rootDiagram = documentController->findOrCreateRootDiagram();
    showDiagram(rootDiagram);
    // select diagram in model tree view
    QModelIndex modelIndex = documentController->treeModel()->indexOf(rootDiagram);
    if (modelIndex.isValid())
        d->modelTreeView->selectFromSourceModelIndex(modelIndex);

    expandModelTreeToDepth(0);
}

void ModelEditor::addDiagramToSelector(const qmt::MDiagram *diagram)
{
    QString diagramLabel = buildDiagramLabel(diagram);
    QVariant diagramUid = QVariant::fromValue(diagram->uid());
    int i = d->diagramSelector->findData(diagramUid);
    if (i >= 0)
        d->diagramSelector->removeItem(i);
    d->diagramSelector->insertItem(0, QIcon(QStringLiteral(":/modelinglib/48x48/canvas-diagram.png")), diagramLabel, diagramUid);
    d->diagramSelector->setCurrentIndex(0);
    while (d->diagramSelector->count() > 20)
        d->diagramSelector->removeItem(d->diagramSelector->count() - 1);
}

void ModelEditor::updateDiagramSelector()
{
    int i = 0;
    while (i < d->diagramSelector->count()) {
        qmt::Uid diagramUid = d->diagramSelector->itemData(i).value<qmt::Uid>();
        if (diagramUid.isValid()) {
            qmt::MDiagram *diagram = d->document->documentController()->modelController()->findObject<qmt::MDiagram>(diagramUid);
            if (diagram) {
                QString diagramLabel = buildDiagramLabel(diagram);
                if (diagramLabel != d->diagramSelector->itemText(i))
                    d->diagramSelector->setItemText(i, diagramLabel);
                ++i;
                continue;
            }
        }
        d->diagramSelector->removeItem(i);
    }
}

void ModelEditor::onDiagramSelectorSelected(int index)
{
    qmt::Uid diagramUid = d->diagramSelector->itemData(index).value<qmt::Uid>();
    if (diagramUid.isValid()) {
        qmt::MDiagram *diagram = d->document->documentController()->modelController()->findObject<qmt::MDiagram>(diagramUid);
        if (diagram) {
            showDiagram(diagram);
            return;
        }
    }
    d->diagramSelector->setCurrentIndex(0);
}

QString ModelEditor::buildDiagramLabel(const qmt::MDiagram *diagram)
{
    QString label = diagram->name();
    qmt::MObject *owner = diagram->owner();
    QStringList path;
    while (owner) {
        path.append(owner->name());
        owner = owner->owner();
    }
    if (!path.isEmpty()) {
        label += QStringLiteral(" [");
        label += path.last();
        for (int i = path.count() - 2; i >= 0; --i) {
            label += QLatin1Char('.');
            label += path.at(i);
        }
        label += QLatin1Char(']');
    }
    return label;
}

void ModelEditor::storeToolbarIdInDiagram(qmt::MDiagram *diagram)
{
    int leftToolboxIndex = d->leftToolBox->currentIndex();
    if (diagram && leftToolboxIndex >= 0 && leftToolboxIndex < d->leftToolBox->count()) {
        QWidget *widget = d->leftToolBox->widget(leftToolboxIndex);
        if (widget) {
            QString toolbarId = widget->property(PROPERTYNAME_TOOLBARID).toString();
            if (toolbarId != diagram->toolbarId()) {
                // NOTE: This update is done without calling ModelController::startUpdateObject() by intention.
                // This update does not need to be seen by any other controller nor shall it
                // No undo shall be possible
                // The document should not be set to modified state
                diagram->setToolbarId(toolbarId);
            }
        }
    }
}

void ModelEditor::addToNavigationHistory(const qmt::MDiagram *diagram)
{
    if (Core::EditorManager::currentEditor() == this) {
        Core::EditorManager::cutForwardNavigationHistory();
        Core::EditorManager::addCurrentPositionToNavigationHistory(saveState(diagram));
    }
}

QByteArray ModelEditor::saveState(const qmt::MDiagram *diagram) const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    stream << 2; // version number
    if (diagram)
        stream << diagram->uid();
    else
        stream << qmt::Uid::invalidUid();
    stream << d->actionHandler->synchronizeBrowserAction()->isChecked()
           << d->syncBrowserWithDiagramAction->isChecked()
           << d->syncDiagramWithBrowserAction->isChecked()
           << d->syncEachOtherAction->isChecked();
    return state;
}

void ModelEditor::onEditSelectedElement()
{
    // TODO introduce similar method for selected elements in model tree
    // currently this method is called on adding new elements in model tree
    // but the method is a no-op in that case.
    qmt::MDiagram *diagram = d->propertiesView->selectedDiagram();
    QList<qmt::DElement *> elements = d->propertiesView->selectedDiagramElements();
    if (diagram && !elements.isEmpty()) {
        qmt::DElement *element = elements.at(0);
        if (element) {
            qmt::DiagramSceneModel *diagramSceneModel = d->document->documentController()->diagramsManager()->diagramSceneModel(diagram);
            if (diagramSceneModel->isElementEditable(element)) {
                d->diagramStack->currentWidget()->setFocus();
                diagramSceneModel->editElement(element);
                return;
            }
        }
        d->propertiesView->editSelectedElement();
    }
}

bool ModelEditor::isSyncBrowserWithDiagram() const
{
    return d->actionHandler->synchronizeBrowserAction()->isChecked()
            && (d->syncBrowserWithDiagramAction->isChecked() || d->syncEachOtherAction->isChecked());
}

bool ModelEditor::isSyncDiagramWithBrowser() const
{
    return d->actionHandler->synchronizeBrowserAction()->isChecked()
            && (d->syncDiagramWithBrowserAction->isChecked() || d->syncEachOtherAction->isChecked());
}

void ModelEditor::synchronizeDiagramWithBrowser()
{
    if (isSyncDiagramWithBrowser()) {
        if (currentDiagram()) {
            bool done = false;
            qmt::DocumentController *documentController = d->document->documentController();
            QModelIndexList indexes = d->modelTreeView->selectedSourceModelIndexes();
            if (!indexes.isEmpty()) {
                foreach (const QModelIndex &index, indexes) {
                    if (index.isValid()) {
                        qmt::MElement *modelElement = documentController->treeModel()->element(index);
                        if (modelElement) {
                            foreach (qmt::DElement *diagramElement, currentDiagram()->diagramElements()) {
                                if (diagramElement->modelUid() == modelElement->uid()) {
                                    // disconnect temporarily avoiding double update of properties Ui
                                    disconnect(documentController->diagramsManager(), &qmt::DiagramsManager::diagramSelectionChanged,
                                               this, &ModelEditor::onDiagramSelectionChanged);
                                    d->diagramView->diagramSceneModel()->selectElement(diagramElement);
                                    connect(documentController->diagramsManager(), &qmt::DiagramsManager::diagramSelectionChanged,
                                            this, &ModelEditor::onDiagramSelectionChanged, Qt::QueuedConnection);
                                    done = true;
                                    break;
                                }
                            }
                            if (done)
                                break;
                        }
                    }
                }
            }
        }
    }
}

void ModelEditor::synchronizeBrowserWithDiagram(const qmt::MDiagram *diagram)
{
    if (isSyncBrowserWithDiagram()) {
        qmt::DocumentController *documentController = d->document->documentController();
        qmt::DSelection selection = documentController->diagramsManager()->diagramSceneModel(diagram)->selectedElements();
        if (!selection.isEmpty()) {
            foreach (qmt::DSelection::Index index, selection.indices()) {
                qmt::DElement *diagramElement = documentController->diagramController()->findElement(index.elementKey(), diagram);
                if (diagramElement) {
                    qmt::MElement *modelElement = documentController->modelController()->findElement(diagramElement->modelUid());
                    if (modelElement) {
                        QModelIndex index = d->modelTreeViewServant->treeModel()->indexOf(modelElement);
                        if (index.isValid()) {
                            // disconnect temporarily avoiding double update of properties Ui
                            disconnect(d->modelTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
                                       this, &ModelEditor::onTreeViewSelectionChanged);
                            d->modelTreeView->selectFromSourceModelIndex(index);
                            connect(d->modelTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
                                    this, &ModelEditor::onTreeViewSelectionChanged, Qt::QueuedConnection);
                            break;
                        }
                    }
                }
            }
        }
    }
}

} // namespace Internal
} // namespace ModelEditor
