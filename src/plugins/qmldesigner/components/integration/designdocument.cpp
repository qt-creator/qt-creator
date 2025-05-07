// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designdocument.h"
#include "designdocumentview.h"
#include "documentmanager.h"
#include "qmldesignerconstants.h"
#include "qmlvisualnode.h"

#include <auxiliarydataproperties.h>
#ifndef QDS_USE_PROJECTSTORAGE
#  include <metainfo.h>
#endif
#include <model/modelresourcemanagement.h>
#include <nodeinstanceview.h>
#include <nodelistproperty.h>
#include <rewritingexception.h>
#include <utils3d.h>
#include <variantproperty.h>
#include <viewmanager.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/kit.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>
#include <timelineactions.h>
#include <svgpasteaction.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QUrl>
#include <QDebug>

#include <QApplication>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QClipboard>

enum {
    debug = false
};

namespace QmlDesigner {

/**
  \class QmlDesigner::DesignDocument

  DesignDocument acts as a facade to a model representing a qml document,
  and the different views/widgets accessing it.
  */
DesignDocument::DesignDocument([[maybe_unused]] const QUrl &filePath,
                               ProjectStorageDependencies projectStorageDependencies,
                               ExternalDependenciesInterface &externalDependencies)
#ifdef QDS_USE_PROJECTSTORAGE
    : m_documentModel(Model::create(projectStorageDependencies,
                                    "Item",
                                    {Import::createLibraryImport("QtQuick")},
                                    filePath,
                                    std::make_unique<ModelResourceManagement>()))
#else
    : m_documentModel(
        Model::create("QtQuick.Item", 1, 0, nullptr, std::make_unique<ModelResourceManagement>()))
    , m_subComponentManager(new SubComponentManager(m_documentModel.get(), externalDependencies))
#endif
    , m_rewriterView(new RewriterView(externalDependencies, RewriterView::Amend))
    , m_documentLoaded(false)
    , m_currentTarget(nullptr)
    , m_projectStorageDependencies(projectStorageDependencies)
    , m_externalDependencies{externalDependencies}
{
#ifndef QDS_USE_PROJECTSTORAGE
    m_rewriterView->setIsDocumentRewriterView(true);
#endif
}

DesignDocument::~DesignDocument() = default;

Model *DesignDocument::currentModel() const
{
    if (m_inFileComponentModel) {
        return m_inFileComponentModel.get();
    }

    return m_documentModel.get();
}

Model *DesignDocument::documentModel() const
{
    return m_documentModel.get();
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
    QString componentText = rewriterView()->extractText({componentNode}).value(componentNode);

    if (componentText.isEmpty())
        return false;

    if (!componentNode.isRootNode()) {
        //change to subcomponent model
        changeToInFileComponentModel(createComponentTextModifier(m_documentTextModifier.get(),
                                                                 rewriterView(),
                                                                 componentText,
                                                                 componentNode));
    }

    return true;
}

const AbstractView *DesignDocument::view() const
{
    return viewManager().view();
}

ModelPointer DesignDocument::createInFileComponentModel()
{
#ifdef QDS_USE_PROJECTSTORAGE
    auto model = m_documentModel->createModel("Item", std::make_unique<ModelResourceManagement>());
#else
    auto model = Model::create("QtQuick.Item",
                               1,
                               0,
                               nullptr,
                               std::make_unique<ModelResourceManagement>());
    model->setFileUrl(m_documentModel->fileUrl());
    model->setMetaInfo(m_documentModel->metaInfo());
#endif

    return model;
}

bool DesignDocument::pasteSVG()
{
    SVGPasteAction svgPasteAction;

    if (!svgPasteAction.containsSVG(QApplication::clipboard()->text()))
        return false;

    rewriterView()->executeInTransaction("DesignDocument::paste1", [&]() {
        ModelNode targetNode;

        // If nodes are currently selected make the first node in selection the target
        if (!view()->selectedModelNodes().isEmpty())
            targetNode = view()->firstSelectedModelNode();

        // If target is still invalid make the root node the target
        if (!targetNode.isValid())
            targetNode = view()->rootModelNode();

        // Check if document has studio components import, if not create it
        QmlDesigner::Import import = QmlDesigner::Import::createLibraryImport("QtQuick.Studio.Components", "1.0");
        if (!currentModel()->hasImport(import, true, true)) {
            QmlDesigner::Import studioComponentsImport = QmlDesigner::Import::createLibraryImport("QtQuick.Studio.Components", "1.0");
            try {
                currentModel()->changeImports({studioComponentsImport}, {});
            } catch (const QmlDesigner::Exception &) {
                QTC_ASSERT(false, return);
            }
        }

        svgPasteAction.createQmlObjectNode(targetNode);
    });

    return true;
}

void DesignDocument::moveNodesToPosition(const QList<ModelNode> &nodes, const std::optional<QVector3D> &position)
{
    if (!nodes.size())
        return;

    QList<ModelNode> movingNodes = nodes;
    DesignDocumentView view{m_externalDependencies};
    currentModel()->attachView(&view);

    ModelNode targetNode; // the node that new nodes should be placed in
    if (!view.selectedModelNodes().isEmpty())
        targetNode = view.firstSelectedModelNode();

    // in case we copy and paste a selection we paste in the parent item
    if ((view.selectedModelNodes().size() == movingNodes.size()) && targetNode.hasParentProperty()) {
        targetNode = targetNode.parentProperty().parentModelNode();
    } else if (view.selectedModelNodes().isEmpty()) {
        // if selection is empty and copied nodes are all 3D nodes, paste them under the active scene
        bool all3DNodes = Utils::allOf(movingNodes, [](const ModelNode &node) {
            return node.metaInfo().isQtQuick3DNode();
        });

        if (all3DNodes) {
            int activeSceneId = Utils3D::active3DSceneId(m_documentModel.get());

            if (activeSceneId != -1) {
                NodeListProperty sceneNodeProperty = QmlVisualNode::findSceneNodeProperty(
                    rootModelNode().view(), activeSceneId);
                targetNode = sceneNodeProperty.parentModelNode();
            }
        }
    }

    if (!targetNode.isValid())
        targetNode = view.rootModelNode();

    for (const ModelNode &node : std::as_const(movingNodes)) {
        for (const ModelNode &node2 : std::as_const(movingNodes)) {
            if (node.isAncestorOf(node2))
                movingNodes.removeAll(node2);
        }
    }

    bool isSingleNode = movingNodes.size() == 1;
    if (isSingleNode) {
        ModelNode singleNode = movingNodes.first();
        if (targetNode.hasParentProperty()
            && singleNode.simplifiedTypeName() == targetNode.simplifiedTypeName()
            && singleNode.variantProperty("width").value() == targetNode.variantProperty("width").value()
            && singleNode.variantProperty("height").value() == targetNode.variantProperty("height").value()) {
            targetNode = targetNode.parentProperty().parentModelNode();
        }
    }

    const PropertyName defaultPropertyName = targetNode.metaInfo().defaultPropertyName();
    if (!targetNode.metaInfo().property(defaultPropertyName).isListProperty())
        qWarning() << Q_FUNC_INFO << "Cannot reparent to" << targetNode;
    NodeListProperty parentProperty = targetNode.nodeListProperty(defaultPropertyName);
    QList<ModelNode> pastedNodeList;

    std::optional<QmlVisualNode> firstVisualNode;
    QVector3D translationVect;
    for (const ModelNode &node : std::as_const(movingNodes)) {
        ModelNode pastedNode(view.insertModel(node));
        pastedNodeList.append(pastedNode);
        parentProperty.reparentHere(pastedNode);

        QmlVisualNode visualNode(pastedNode);
        if (!firstVisualNode && visualNode) {
            firstVisualNode = visualNode;
            translationVect = (position && firstVisualNode) ? *position - firstVisualNode->position()
                                                            : QVector3D();
        }
        visualNode.translate(translationVect);
    }

    view.setSelectedModelNodes(pastedNodeList);
    view.model()->clearMetaInfoCache();
}

bool DesignDocument::inFileComponentModelActive() const
{
    return bool(m_inFileComponentModel);
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
    return fileName().toUrlishString();
}

QString DesignDocument::simplfiedDisplayName() const
{
    if (rootModelNode().id().isEmpty())
        return rootModelNode().id();
    else
        return rootModelNode().simplifiedTypeName();
}

void DesignDocument::updateFileName(const Utils::FilePath & /*oldFileName*/, const Utils::FilePath &newFileName)
{
    if (m_documentModel)
        m_documentModel->setFileUrl(QUrl::fromLocalFile(newFileName.toUrlishString()));

    if (m_inFileComponentModel)
        m_inFileComponentModel->setFileUrl(QUrl::fromLocalFile(newFileName.toUrlishString()));

    emit displayNameChanged(displayName());
}

Utils::FilePath DesignDocument::fileName() const
{
    return editor() ? editor()->document()->filePath() : Utils::FilePath();
}

ProjectExplorer::Target *DesignDocument::currentTarget() const
{
    return m_currentTarget;
}

bool DesignDocument::isDocumentLoaded() const
{
    return m_documentLoaded;
}

void DesignDocument::resetToDocumentModel()
{
    const Utils::PlainTextEdit *edit = plainTextEdit();
    if (edit)
        edit->document()->clearUndoRedoStacks();

    m_inFileComponentModel.reset();
}

void DesignDocument::loadDocument(Utils::PlainTextEdit *edit)
{
    Q_CHECK_PTR(edit);

    connect(edit, &Utils::PlainTextEdit::undoAvailable, this, &DesignDocument::undoAvailable);
    connect(edit, &Utils::PlainTextEdit::redoAvailable, this, &DesignDocument::redoAvailable);
    connect(edit, &Utils::PlainTextEdit::modificationChanged, this, &DesignDocument::dirtyStateChanged);

    m_documentTextModifier.reset(new BaseTextEditModifier(qobject_cast<TextEditor::TextEditorWidget *>(plainTextEdit())));

    connect(m_documentTextModifier.get(),
            &TextModifier::textChanged,
            this,
            &DesignDocument::updateQrcFiles);

    m_rewriterView->setTextModifier(m_documentTextModifier.get());

    m_inFileComponentTextModifier.reset();

    updateFileName(Utils::FilePath(), fileName());

    updateQrcFiles();

    m_documentLoaded = true;
}

void DesignDocument::changeToDocumentModel()
{
    viewManager().detachRewriterView();
    viewManager().detachViewsExceptRewriterAndComponetView();

    const Utils::PlainTextEdit *edit = plainTextEdit();
    if (edit)
        edit->document()->clearUndoRedoStacks();

    m_rewriterView->setTextModifier(m_documentTextModifier.get());

    m_inFileComponentModel.reset();
    m_inFileComponentTextModifier.reset();

    viewManager().attachRewriterView();
    viewManager().attachViewsExceptRewriterAndComponetView();
}

bool DesignDocument::isQtForMCUsProject() const
{
    if (m_currentTarget && m_currentTarget->buildSystem())
        return m_currentTarget->buildSystem()->additionalData("CustomQtForMCUs").toBool();

    return false;
}

Utils::FilePath DesignDocument::projectFolder() const
{
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::projectForFile(fileName());

    if (currentProject)
        return currentProject->projectDirectory();
    return {};
}

bool DesignDocument::hasProject() const
{
    return !DocumentManager::currentProjectDirPath().isEmpty();
}

void DesignDocument::setModified()
{
    if (m_documentTextModifier)
        m_documentTextModifier->textDocument()->setModified(true);
}

void DesignDocument::changeToInFileComponentModel(ComponentTextModifier *textModifer)
{
    m_inFileComponentTextModifier.reset(textModifer);
    viewManager().detachRewriterView();
    viewManager().detachViewsExceptRewriterAndComponetView();

    const Utils::PlainTextEdit *edit = plainTextEdit();
    if (edit)
        edit->document()->clearUndoRedoStacks();

    m_inFileComponentModel = createInFileComponentModel();

    m_rewriterView->setTextModifier(m_inFileComponentTextModifier.get());

    viewManager().attachRewriterView();
    viewManager().attachViewsExceptRewriterAndComponetView();
}

void DesignDocument::updateQrcFiles()
{
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::projectForFile(fileName());

    if (currentProject) {
        const auto srcFiles = currentProject->files(ProjectExplorer::Project::SourceFiles);
        for (const Utils::FilePath &fileName : srcFiles) {
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

void DesignDocument::clearUndoRedoStacks() const
{
    const Utils::PlainTextEdit *edit = plainTextEdit();
    if (edit)
        edit->document()->clearUndoRedoStacks();
}

void DesignDocument::close()
{
    m_documentLoaded = false;
    emit designDocumentClosed();
}

#ifndef QDS_USE_PROJECTSTORAGE
void DesignDocument::updateSubcomponentManager()
{
    Q_ASSERT(m_subComponentManager);
    m_subComponentManager->update(QUrl::fromLocalFile(fileName().toUrlishString()),
                                  currentModel()->imports() + currentModel()->possibleImports());
}

void DesignDocument::addSubcomponentManagerImport(const Import &import)
{
    m_subComponentManager->addAndParseImport(import);
}
#endif

void DesignDocument::deleteSelected()
{
    if (!currentModel())
        return;

    QStringList lockedNodes;
    for (const ModelNode &modelNode : view()->selectedModelNodes()) {
        for (const ModelNode &node : modelNode.allSubModelNodesAndThisNode()) {
            if (node.isValid() && !node.isRootNode() && node.locked() && !lockedNodes.contains(node.id()))
                lockedNodes.push_back(node.id());
        }
    }

    if (!lockedNodes.empty()) {
        Utils::sort(lockedNodes);
        QString detailedText = QString("<b>" + tr("Locked items:") + "</b><br>");

        for (const auto &id : std::as_const(lockedNodes))
            detailedText.append("- " + id + "<br>");

        detailedText.chop(QString("<br>").size());

        QMessageBox msgBox;
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle(tr("Delete/Cut Item"));
        msgBox.setText(QString(tr("Deleting or cutting this item will modify locked items.") + "<br><br>%1")
                               .arg(detailedText));
        msgBox.setInformativeText(tr("Do you want to continue by removing the item (Delete) or removing it and copying it to the clipboard (Cut)?"));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);

        if (msgBox.exec() == QMessageBox::Cancel)
            return;
    }

    rewriterView()->executeInTransaction("DesignDocument::deleteSelected", [this] {
        const QList<ModelNode> toDelete = view()->selectedModelNodes();
        for (ModelNode node : toDelete) {
            if (node.isValid() && !node.isRootNode() && QmlObjectNode::isValidQmlObjectNode(node))
                QmlObjectNode(node).destroy();
        }
    });
}

void DesignDocument::copySelected()
{
    DesignDocumentView view{m_externalDependencies};

    currentModel()->attachView(&view);

    DesignDocumentView::copyModelNodes(view.selectedModelNodes(), m_externalDependencies);
}

void DesignDocument::cutSelected()
{
    copySelected();
    deleteSelected();
}

void DesignDocument::duplicateSelected()
{
    DesignDocumentView view{m_externalDependencies};
    currentModel()->attachView(&view);
    const QList<ModelNode> selectedNodes = view.selectedModelNodes();
    currentModel()->detachView(&view);

    rewriterView()->executeInTransaction("DesignDocument::duplicateSelected", [this, selectedNodes]() {
        moveNodesToPosition(selectedNodes, {});
    });
}

void DesignDocument::paste()
{
    pasteToPosition({});
}

void DesignDocument::pasteToPosition(const std::optional<QVector3D> &position)
{
    if (pasteSVG())
        return;

    if (TimelineActions::clipboardContainsKeyframes()) // pasting keyframes is handled in TimelineView
        return;

    auto pasteModel = DesignDocumentView::pasteToModel(m_externalDependencies);

    if (!pasteModel)
        return;

    DesignDocumentView view{m_externalDependencies};
    pasteModel->attachView(&view);
    ModelNode rootNode(view.rootModelNode());

    if (rootNode.type() == "empty")
        return;

    QList<ModelNode> selectedNodes;
    if (rootNode.id() == "__multi__selection__")
        selectedNodes << rootNode.directSubModelNodes();
    else
        selectedNodes << rootNode;

    pasteModel->detachView(&view);

    rewriterView()->executeInTransaction("DesignDocument::pasteToPosition", [this, selectedNodes, position]() {
        moveNodesToPosition(selectedNodes, position);
    });
}

void DesignDocument::selectAll()
{
    if (!currentModel())
        return;

    DesignDocumentView view{m_externalDependencies};
    currentModel()->attachView(&view);

    QList<ModelNode> allNodesExceptRootNode(view.allModelNodes());
    allNodesExceptRootNode.removeOne(view.rootModelNode());
    view.setSelectedModelNodes(allNodesExceptRootNode);
}

RewriterView *DesignDocument::rewriterView() const
{
    return m_rewriterView.get();
}

#ifndef QDS_USE_PROJECTSTORAGE
static void removeUnusedImports(RewriterView *rewriter)
{
    // Remove any import statements for asset based nodes (composed effect or imported3d)
    // if there is no nodes using them in the scene.
    QTC_ASSERT(rewriter && rewriter->model(), return);

    GeneratedComponentUtils compUtils{rewriter->externalDependencies()};

    const QString effectPrefix = compUtils.composedEffectsTypePrefix();
    const QString imported3dPrefix = compUtils.import3dTypePrefix();
    const QList<Utils::FilePath> qmlFiles = compUtils.imported3dComponents();
    QHash<QString, QString> m_imported3dTypeMap;
    for (const Utils::FilePath &qmlFile : qmlFiles) {
        QString importName = compUtils.getImported3dImportName(qmlFile);
        QString type = qmlFile.baseName();
        m_imported3dTypeMap.insert(importName, type);
    }

    const Imports &imports = rewriter->model()->imports();
    QHash<QString, Import> assetImports;
    for (const Import &import : imports) {
        if (import.url().startsWith(effectPrefix)) {
            QString type = import.url().split('.').last();
            assetImports.insert(type, import);
        } else if (import.url().startsWith(imported3dPrefix)) {
            assetImports.insert(m_imported3dTypeMap[import.url()], import);
        }
    }

    const QList<ModelNode> allNodes = rewriter->allModelNodes();
    for (const ModelNode &node : allNodes) {
        if (QmlItemNode(node).isEffectItem()
            || (node.isComponent() && node.metaInfo().isQtQuick3DNode())) {
            assetImports.remove(node.simplifiedTypeName());
        }
    }

    if (!assetImports.isEmpty()) {
        Imports removeImports;
        for (const Import &import : assetImports)
            removeImports.append(import);
        rewriter->model()->changeImports({}, removeImports);
    }

    rewriter->forceAmend();
}
#endif

void DesignDocument::setEditor(Core::IEditor *editor)
{
    m_textEditor = editor;
    // if the user closed the file explicit we do not want to do anything with it anymore

    connect(Core::EditorManager::instance(), &Core::EditorManager::aboutToSave,
            this, [this](Core::IDocument *document) {
        if (m_textEditor && m_textEditor->document() == document) {
            if (m_documentModel && m_documentModel->rewriterView()) {

#ifdef QDS_USE_PROJECTSTORAGE
                // TODO: ProjectStorage should handle this via Model somehow (QDS-14519)
#else
                removeUnusedImports(rewriterView());
#endif
                m_documentModel->rewriterView()->writeAuxiliaryData();
            }
        }
    });

    connect(Core::EditorManager::instance(), &Core::EditorManager::editorAboutToClose,
            this, [this](Core::IEditor *editor) {
        if (m_textEditor.data() == editor)
            m_textEditor.clear();
    });

    connect(editor->document(), &Core::IDocument::filePathChanged, this, &DesignDocument::updateFileName);

    updateActiveTarget();
    updateActiveTarget();
}

Core::IEditor *DesignDocument::editor() const
{
    return m_textEditor.data();
}

TextEditor::BaseTextEditor *DesignDocument::textEditor() const
{
    return qobject_cast<TextEditor::BaseTextEditor*>(editor());
}

Utils::PlainTextEdit *DesignDocument::plainTextEdit() const
{
    if (TextEditor::BaseTextEditor *editor = textEditor())
        return editor->editorWidget();
    return nullptr;
}

ModelNode DesignDocument::rootModelNode() const
{
    return rewriterView()->rootModelNode();
}

void DesignDocument::undo()
{
    if (rewriterView() && !rewriterView()->modificationGroupActive()) {
        plainTextEdit()->undo();
        rewriterView()->forceAmend();
    }

    viewManager().resetPropertyEditorView();
}

void DesignDocument::redo()
{
    if (rewriterView() && !rewriterView()->modificationGroupActive()) {
        plainTextEdit()->redo();
        rewriterView()->forceAmend();
    }

    viewManager().resetPropertyEditorView();
}

static ProjectExplorer::Target *getActiveTarget(DesignDocument *designDocument)
{
    auto currentProject = ProjectExplorer::ProjectManager::projectForFile(designDocument->fileName());

    if (!currentProject)
        currentProject = ProjectExplorer::ProjectTree::currentProject();

    if (!currentProject)
        return nullptr;

    QObject::connect(ProjectExplorer::ProjectTree::instance(),
                     &ProjectExplorer::ProjectTree::currentProjectChanged,
                     designDocument,
                     &DesignDocument::updateActiveTarget,
                     Qt::UniqueConnection);

    QObject::connect(currentProject,
                     &ProjectExplorer::Project::activeTargetChanged,
                     designDocument,
                     &DesignDocument::updateActiveTarget,
                     Qt::UniqueConnection);

    auto target = currentProject->activeTarget();

    if (!target || !target->kit()->isValid())
        return nullptr;

    QObject::connect(target,
                     &ProjectExplorer::Target::kitChanged,
                     designDocument,
                     &DesignDocument::updateActiveTarget,
                     Qt::UniqueConnection);

    return target;
}

void DesignDocument::updateActiveTarget()
{
    m_currentTarget = getActiveTarget(this);
    viewManager().setNodeInstanceViewTarget(m_currentTarget);
}

void DesignDocument::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (auto v = view())
        QmlDesignerPlugin::contextHelp(callback, v->contextHelpId());
    else
        callback({});
}

} // namespace QmlDesigner
