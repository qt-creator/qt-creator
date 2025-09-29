// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dwidget.h"

#include "edit3dactions.h"
#include "edit3dcanvas.h"
#include "edit3dmaterialsaction.h"
#include "edit3dtoolbarmenu.h"
#include "edit3dview.h"

#include <auxiliarydataproperties.h>
#include <designeractionmanager.h>
#include <designdocument.h>
#include <designericons.h>
#include <designmodewidget.h>
#include <externaldependenciesinterface.h>
#include <generatedcomponentutils.h>
#include <nodeabstractproperty.h>
#include <nodehints.h>
#include <nodeinstanceview.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmleditormenu.h>
#include <qmlvisualnode.h>
#include <seekerslider.h>
#include <toolbox.h>
#include <viewmanager.h>
#include <utils3d.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>

#include <modelutils.h>

#include <qmldesignerutils/asset.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QVBoxLayout>

using namespace Core;

namespace QmlDesigner {

inline static QIcon contextIcon(const DesignerIcons::IconId &iconId)
{
    return DesignerActionManager::instance().contextIcon(iconId);
};

static QIcon getEntryIcon(const ItemLibraryEntry &entry)
{
    static const QMap<QString, DesignerIcons::IconId> itemLibraryDesignerIconId = {
        {"OrthographicCamera__Camera Orthographic", DesignerIcons::CameraOrthographicIcon},
        {"PerspectiveCamera__Camera Perspective", DesignerIcons::CameraPerspectiveIcon},
        {"DirectionalLight__Light Directional", DesignerIcons::LightDirectionalIcon},
        {"PointLight__Light Point", DesignerIcons::LightPointIcon},
        {"SpotLight__Light Spot", DesignerIcons::LightSpotIcon},
        {"Model__Cone", DesignerIcons::ModelConeIcon},
        {"Model__Cube", DesignerIcons::ModelCubeIcon},
        {"Model__Cylinder", DesignerIcons::ModelCylinderIcon},
        {"Model__Plane", DesignerIcons::ModelPlaneIcon},
        {"Model__Sphere", DesignerIcons::ModelSphereIcon},
    };

    QString entryKey = entry.typeName() + "__" + entry.name();
    if (itemLibraryDesignerIconId.contains(entryKey)) {
        return contextIcon(itemLibraryDesignerIconId.value(entryKey));
    }
    return QIcon(entry.libraryEntryIconPath());
}

Edit3DWidget::Edit3DWidget(Edit3DView *view)
    : m_view(view)
    , m_bundleHelper(std::make_unique<BundleHelper>(view, this))
{
    setAcceptDrops(true);

    QString sheet = Utils::FileUtils::fetchQrc(":/qmldesigner/stylesheet.css");
    setStyleSheet(Theme::replaceCssColors(sheet));

    Core::Context context(Constants::qml3DEditorContextId);
    m_context = new Core::IContext(this);
    m_context->setContext(context);
    m_context->setWidget(this);

    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);

    auto fillLayout = new QVBoxLayout(this);
    fillLayout->setContentsMargins(0, 0, 0, 0);
    fillLayout->setSpacing(0);
    setLayout(fillLayout);

    // Initialize toolbar
    m_toolBox = new ToolBox(this);
    fillLayout->addWidget(m_toolBox.data());

    // Iterate through view actions. A null action indicates a separator and a second null action
    // after separator indicates an exclusive group.
    auto handleActions = [this, &context](const QVector<Edit3DAction *> &actions, QMenu *menu, bool left) {
        bool previousWasSeparator = true;
        QActionGroup *group = nullptr;
        QActionGroup *proxyGroup = nullptr;
        for (auto action : actions) {
            if (action) {
                QAction *a = action->action();
                if (group)
                    group->addAction(a);
                if (menu) {
                    menu->addAction(a);
                } else {
                    addAction(a);
                    if (left)
                        m_toolBox->addLeftSideAction(a);
                    else
                        m_toolBox->addRightSideAction(a);
                }
                previousWasSeparator = false;

                // Register action as creator command to make it configurable
                Core::Command *command = Core::ActionManager::registerAction(
                            a, Utils::Id::fromName(action->menuId()), context);
                m_actionToCommandHash.insert(a, command);
                command->setDefaultKeySequence(a->shortcut());
                if (proxyGroup)
                    proxyGroup->addAction(command->action());
                // Menu actions will have custom tooltips
                if (menu)
                    a->setToolTip(command->stringWithAppendedShortcut(a->toolTip()));
                else
                    command->augmentActionWithShortcutToolTip(a);

                // Clear action shortcut so it doesn't conflict with command's override action
                a->setShortcut({});
            } else {
                if (previousWasSeparator) {
                    group = new QActionGroup(this);
                    proxyGroup = new QActionGroup(this);
                    previousWasSeparator = false;
                } else {
                    group = nullptr;
                    proxyGroup = nullptr;
                    auto separator = new QAction(this);
                    separator->setSeparator(true);
                    if (menu) {
                        menu->addAction(separator);
                    } else {
                        addAction(separator);
                        if (left)
                            m_toolBox->addLeftSideAction(separator);
                        else
                            m_toolBox->addRightSideAction(separator);
                    }
                    previousWasSeparator = true;
                }
            }
        }
    };

    handleActions(view->leftActions(), nullptr, true);
    handleActions(view->rightActions(), nullptr, false);

    m_visibilityTogglesMenu = new Edit3DToolbarMenu(this);
    handleActions(view->visibilityToggleActions(), m_visibilityTogglesMenu, false);

    m_backgroundColorMenu = new QMenu(this);
    m_backgroundColorMenu->setToolTipsVisible(true);

    handleActions(view->backgroundColorActions(), m_backgroundColorMenu, false);

    m_viewportPresetsMenu = new QMenu(this);
    m_viewportPresetsMenu->setToolTipsVisible(true);
    handleActions(view->viewportPresetActions(), m_viewportPresetsMenu, false);

    createContextMenu();

    // Onboarding label contains instructions for new users how to get 3D content into the project
    m_onboardingLabel = new QLabel(this);
    m_onboardingLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_onboardingLabel->setWordWrap(true);
    connect(m_onboardingLabel, &QLabel::linkActivated, this, &Edit3DWidget::linkActivated);
    fillLayout->addWidget(m_onboardingLabel.data());

    // Canvas is used to render the actual edit 3d view
    m_canvas = new Edit3DCanvas(this);
    fillLayout->addWidget(m_canvas.data());
    showCanvas(false);

    IContext::attach(this,
                     Context(Constants::qml3DEditorContextId, Constants::qtQuickToolsMenuContextId),
                     [this](const IContext::HelpCallback &callback) { contextHelp(callback); });
}

void Edit3DWidget::createContextMenu()
{
    m_contextMenu = new QmlEditorMenu(this);

    m_editComponentAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::EditComponentIcon),
                tr("Edit Component"), [&] {
        DocumentManager::goIntoComponent(m_view->singleSelectedModelNode());
    });

    m_materialsAction = new Edit3DMaterialsAction(contextIcon(DesignerIcons::MaterialIcon), this);
    m_contextMenu->addAction(m_materialsAction);

    m_contextMenu->addSeparator();

    m_copyAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::CopyIcon),
                tr("Copy"), [&] {
        QmlDesignerPlugin::instance()->currentDesignDocument()->copySelected();
    });

    m_pasteAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::PasteIcon),
                tr("Paste"), [&] {
        QmlDesignerPlugin::instance()->currentDesignDocument()->pasteToPosition(m_contextMenuPos3d);
    });

    m_deleteAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::DeleteIcon),
                tr("Delete"), [&] {
        view()->executeInTransaction("Edit3DWidget::createContextMenu", [&] {
            for (ModelNode &node : m_view->selectedModelNodes())
                node.destroy();
        });
    });

    m_duplicateAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::DuplicateIcon),
                tr("Duplicate"), [&] {
        QmlDesignerPlugin::instance()->currentDesignDocument()->duplicateSelected();
    });

    m_contextMenu->addSeparator();

    m_fitSelectedAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::FitSelectedIcon),
                tr("Fit Selected Items to View"), [&] {
        view()->emitView3DAction(View3DActionType::FitToView, true);
    });

    m_alignCameraAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::AlignCameraToViewIcon),
                tr("Align Camera to View"), [&] {
        view()->emitView3DAction(View3DActionType::AlignCamerasToView, true);
    });

    m_alignViewAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::AlignViewToCameraIcon),
                tr("Align View to Camera"), [&] {
        view()->emitView3DAction(View3DActionType::AlignViewToCamera, true);
    });

    m_bakeLightsAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::LightIcon),  // TODO: placeholder icon
                tr("Bake Lights"), [&] {
        view()->bakeLightsAction()->action()->trigger();
    });

    m_contextMenu->addSeparator();

    m_selectParentAction = m_contextMenu->addAction(
        contextIcon(DesignerIcons::ParentIcon), tr("Select Parent"), [&] {
            ModelNode parentNode = ModelUtils::lowestCommonAncestor(view()->selectedModelNodes());
            if (!parentNode.isValid())
                return;

            if (!parentNode.isRootNode() && view()->isSelectedModelNode(parentNode))
                parentNode = parentNode.parentProperty().parentModelNode();

            view()->setSelectedModelNode(parentNode);
        });

    QAction *defaultToggleGroupAction = view()->edit3DAction(View3DActionType::SelectionModeToggle)->action();
    m_toggleGroupAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::ToggleGroupIcon),
                tr("Group Selection Mode"), [&]() {
        view()->edit3DAction(View3DActionType::SelectionModeToggle)->action()->trigger();
    });
    connect(defaultToggleGroupAction, &QAction::toggled, m_toggleGroupAction, &QAction::setChecked);
    m_toggleGroupAction->setCheckable(true);
    m_toggleGroupAction->setChecked(defaultToggleGroupAction->isChecked());

    m_contextMenu->addSeparator();

    auto overridesSubMenu = new QmlEditorMenu(tr("Viewport Shading"), m_contextMenu);
    overridesSubMenu->setToolTipsVisible(true);
    m_contextMenu->addMenu(overridesSubMenu);

    m_wireFrameAction = overridesSubMenu->addAction(
        tr("Wireframe"), this, &Edit3DWidget::onWireframeAction);
    m_wireFrameAction->setCheckable(true);
    m_wireFrameAction->setToolTip(tr("Show models as wireframe."));

    overridesSubMenu->addSeparator();

    // The type enum order must match QQuick3DDebugSettings::QQuick3DMaterialOverrides enums
    enum class MaterialOverrideType {
        None,
        BaseColor,
        Roughness,
        Metalness,
        Diffuse,
        Specular,
        ShadowOcclusion,
        Emission,
        AmbientOcclusion,
        Normals,
        Tangents,
        Binormals,
        F0
    };

    auto addOverrideMenuAction = [&](const QString &label, const QString &toolTip,
                                     MaterialOverrideType type) {
        QAction *action = overridesSubMenu->addAction(label);
        connect(action, &QAction::triggered, this, [this, action] { onMatOverrideAction(action); });
        action->setData(int(type));
        action->setCheckable(true);
        action->setToolTip(toolTip);
        m_matOverrideActions.insert(int(type), action);
    };

    addOverrideMenuAction(tr("Default"),
                          tr("Rendering occurs as normal."),
                          MaterialOverrideType::None);
    addOverrideMenuAction(tr("Base Color"),
                          tr("The base or diffuse color of a material is passed through without any lighting."),
                          MaterialOverrideType::BaseColor);
    addOverrideMenuAction(tr("Roughness"),
                          tr("The roughness of a material is passed through as an unlit greyscale value."),
                          MaterialOverrideType::Roughness);
    addOverrideMenuAction(tr("Metalness"),
                          tr("The metalness of a material is passed through as an unlit greyscale value."),
                          MaterialOverrideType::Metalness);
    addOverrideMenuAction(tr("Normals"),
                          tr("The interpolated world space normal value of the material mapped to an RGB color."),
                          MaterialOverrideType::Normals);
    addOverrideMenuAction(tr("Ambient Occlusion"),
                          tr("Only the ambient occlusion of the material."),
                          MaterialOverrideType::AmbientOcclusion);
    addOverrideMenuAction(tr("Emission"),
                          tr("Only the emissive contribution of the material."),
                          MaterialOverrideType::Emission);
    addOverrideMenuAction(tr("Shadow Occlusion"),
                          tr("The occlusion caused by shadows as a greyscale value."),
                          MaterialOverrideType::ShadowOcclusion);
    addOverrideMenuAction(tr("Diffuse"),
                          tr("Only the diffuse contribution of the material after all lighting."),
                          MaterialOverrideType::Diffuse);
    addOverrideMenuAction(tr("Specular"),
                          tr("Only the specular contribution of the material after all lighting."),
                          MaterialOverrideType::Specular);

    overridesSubMenu->addSeparator();

    QAction *resetAction = overridesSubMenu->addAction(
        tr("Reset All Viewports"), this, &Edit3DWidget::onResetAllOverridesAction);
    resetAction->setToolTip(tr("Reset all shading options for all viewports."));

    m_contextMenu->addSeparator();

    m_addToContentLibAction = m_contextMenu->addAction(
        contextIcon(DesignerIcons::CreateIcon),  // TODO: placeholder icon
        tr("Add to Content Library"), [&] {
            QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("ContentLibrary");
            view()->emitCustomNotification("add_node_to_content_lib", {}); // To ContentLibrary
        });

    m_importBundleAction = m_contextMenu->addAction(
        contextIcon(DesignerIcons::CreateIcon),  // TODO: placeholder icon
        tr("Import Bundle"), [&] {
            m_bundleHelper->importBundleToProject();
        });

    m_exportBundleAction = m_contextMenu->addAction(
        contextIcon(DesignerIcons::CreateIcon),  // TODO: placeholder icon
        tr("Export Bundle"), [&] {
            m_bundleHelper->exportBundle(m_view->selectedModelNodes());
        });

    m_contextMenu->addSeparator();
}

bool Edit3DWidget::isPasteAvailable() const
{
    return QApplication::clipboard()->text().startsWith(Constants::HEADER_3DPASTE_CONTENT);
}

bool Edit3DWidget::isSceneLocked() const
{
    if (m_view && m_view->hasModelNodeForInternalId(m_canvas->activeScene())) {
        ModelNode node = m_view->modelNodeForInternalId(m_canvas->activeScene());
        if (ModelUtils::isThisOrAncestorLocked(node))
            return true;
    }
    return false;
}

void Edit3DWidget::showOnboardingLabel()
{
    QString text;
    if (m_view->externalDependencies().isQtForMcusProject())
        text = tr("3D view is not supported in MCU projects.");

    if (text.isEmpty()) {
        if (m_view->externalDependencies().isQt6Project()) {
            QString labelText =
                tr("To use the <b>3D</b> view, add the <b>QtQuick3D</b> module and the <b>View3D</b>"
                   " component in the <b>Components</b> view or click"
                   " <a href=\"#add_import\"><span style=\"text-decoration:none;color:%1\">here</span></a>"
                   ".<br><br>"
                   "To import 3D assets, select <b>+</b> in the <b>Assets</b> view.");
            text = labelText.arg(Utils::creatorColor(Utils::Theme::TextColorLink).name());
        } else {
            text = tr("3D view is not supported in Qt5 projects.");
        }
    }

    m_onboardingLabel->setText(text);
    m_onboardingLabel->setVisible(true);
}

// Called by the view to update the "create" sub-menu when the Quick3D entries are ready.
void Edit3DWidget::updateCreateSubMenu(const QList<ItemLibraryDetails> &entriesList)
{
    if (!m_contextMenu)
        return;

    if (m_createSubMenu) {
        m_contextMenu->removeAction(m_createSubMenu->menuAction());
        m_createSubMenu->deleteLater();
    }

    m_nameToImport.clear();
    m_nameToEntry.clear();

    m_createSubMenu = new QmlEditorMenu(tr("Create"), m_contextMenu);
    m_createSubMenu->setIcon(contextIcon(DesignerIcons::CreateIcon));
    m_contextMenu->addMenu(m_createSubMenu);

    const QString docPath = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName().toUrlishString();

    auto isEntryValid = [&](const ItemLibraryEntry &entry) -> bool {
        // Don't allow entries that match current document
        const QString path = entry.customComponentSource();
        return path.isEmpty() || docPath != path;
    };

    for (const auto &details : entriesList) {
        QList<ItemLibraryEntry> entries = details.entryList;
        if (entries.isEmpty())
            continue;

        QMenu *catMenu = nullptr;

        std::sort(entries.begin(), entries.end(), [](const ItemLibraryEntry &a, const ItemLibraryEntry &b) {
            return a.name() < b.name();
        });

        for (const ItemLibraryEntry &entry : std::as_const(entries)) {
            if (!isEntryValid(entry))
                continue;

            if (!catMenu) {
                catMenu = new QmlEditorMenu(details.name, m_createSubMenu);
                catMenu->setIcon(details.icon);
                m_createSubMenu->addMenu(catMenu);
            }

            QAction *action = catMenu->addAction(getEntryIcon(entry), entry.name());
            connect(action, &QAction::triggered, this, [this, action] { onCreateAction(action); });
            action->setData(entry.name());
            Import import = Import::createLibraryImport(entry.requiredImport(),
                                                 QString::number(entry.majorVersion())
                                                     + QLatin1Char('.')
                                                     + QString::number(entry.minorVersion()));
            m_nameToImport.insert(entry.name(), import);
            m_nameToEntry.insert(entry.name(), entry);
        }
    }

    // Create menu for imported 3d models, which don't have ItemLibraryEntries
    const GeneratedComponentUtils &compUtils
        = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    QList<Utils::FilePath> qmlFiles = compUtils.imported3dComponents();
    QMenu *catMenu = nullptr;

    if (!qmlFiles.isEmpty()) {
        catMenu = new QmlEditorMenu(tr("Imported Models"), m_createSubMenu);
        catMenu->setIcon(contextIcon(DesignerIcons::ImportedModelsIcon));
        m_createSubMenu->addMenu(catMenu);

        std::ranges::sort(qmlFiles, {}, &Utils::FilePath::baseName);

        const QIcon icon = QIcon(":/edit3d/images/item-3D_model-icon.png");

        for (const Utils::FilePath &qmlFile : std::as_const(qmlFiles)) {
            QString qmlName = qmlFile.baseName();
            QAction *action = catMenu->addAction(icon, qmlName);
            connect(action, &QAction::triggered, this, [this, action] { onCreateAction(action); });
            action->setData(qmlName);

            QString importName = compUtils.getImported3dImportName(qmlFile);
            if (!importName.isEmpty()) {
                Import import = Import::createLibraryImport(importName);
                m_nameToImport.insert(qmlName, import);
            }
        }
    }
}

// Action triggered from the "create" sub-menu
void Edit3DWidget::onCreateAction(QAction *action)
{
    if (!m_view || !m_view->model() || isSceneLocked())
        return;

    m_view->executeInTransaction(__FUNCTION__, [&] {
        const QString actionName = action->data().toString();
        Import import = m_nameToImport.value(actionName);

        if (!m_view->model()->hasImport(import, true, true))
            m_view->model()->changeImports({import}, {});

        ModelNode modelNode;
        if (m_nameToEntry.contains(actionName)) {
            int activeScene = Utils3D::active3DSceneId(m_view->model());
            ItemLibraryEntry entry = m_nameToEntry.value(actionName);

            modelNode = QmlVisualNode::createQml3DNode(m_view, entry, activeScene,
                                                       m_contextMenuPos3d).modelNode();
        } else {
            ModelNode sceneNode = Utils3D::active3DSceneNode(m_view);
            if (!sceneNode.isValid())
                sceneNode = m_view->rootModelNode();

            modelNode = QmlVisualNode::QmlVisualNode::createQml3DNode(
                            m_view, actionName.toUtf8(), sceneNode, import.url(),
                            m_contextMenuPos3d).modelNode();
        }

        QTC_ASSERT(modelNode.isValid(), return);
        m_view->setSelectedModelNode(modelNode);

        // if added node is a Model, assign it a material
        if (modelNode.metaInfo().isQtQuick3DModel())
            Utils3D::assignMaterialTo3dModel(m_view, modelNode);
    });
}

void Edit3DWidget::onMatOverrideAction(QAction *action)
{
    if (!m_view || !m_view->model())
        return;

    QVariantList list;
    for (int i = 0; i < m_view->viewportToolStates().size(); ++i) {
        Edit3DView::ViewportToolState state = m_view->viewportToolStates()[i];
        if (i == m_view->activeViewport()) {
            state.matOverride = action->data().toInt();
            m_view->setViewportToolState(i, state);
            list.append(action->data());
        } else {
            list.append(state.matOverride);
        }
    }

    view()->emitView3DAction(View3DActionType::MaterialOverride, list);
}

void Edit3DWidget::onWireframeAction()
{
    if (!m_view || !m_view->model())
        return;

    QVariantList list;
    for (int i = 0; i < m_view->viewportToolStates().size(); ++i) {
        Edit3DView::ViewportToolState state = m_view->viewportToolStates()[i];
        if (i == m_view->activeViewport()) {
            state.showWireframe = m_wireFrameAction->isChecked();
            m_view->setViewportToolState(i, state);
            list.append(m_wireFrameAction->isChecked());
        } else {
            list.append(state.showWireframe);
        }
    }

    view()->emitView3DAction(View3DActionType::ShowWireframe, list);
}

void Edit3DWidget::onResetAllOverridesAction()
{
    if (!m_view || !m_view->model())
        return;

    QVariantList wList;
    QVariantList mList;

    for (int i = 0; i < m_view->viewportToolStates().size(); ++i) {
        Edit3DView::ViewportToolState state;
        state.showWireframe = false;
        state.matOverride = 0;
        m_view->setViewportToolState(i, state);
        wList.append(state.showWireframe);
        mList.append(state.matOverride);
    }

    view()->emitView3DAction(View3DActionType::ShowWireframe, wList);
    view()->emitView3DAction(View3DActionType::MaterialOverride, mList);
}

void Edit3DWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_view)
        QmlDesignerPlugin::contextHelp(callback, m_view->contextHelpId());
    else
        callback({});
}

void Edit3DWidget::showCanvas(bool show)
{
    if (!show) {
        QImage emptyImage;
        m_canvas->updateRenderImage(emptyImage);
    }
    m_canvas->setVisible(show);

    if (show)
        m_onboardingLabel->setVisible(false);
    else
        showOnboardingLabel();
    m_toolBox->setVisible(show);
}

QMenu *Edit3DWidget::visibilityTogglesMenu() const
{
    return m_visibilityTogglesMenu.data();
}

void Edit3DWidget::showVisibilityTogglesMenu(bool show, const QPoint &pos)
{
    if (m_visibilityTogglesMenu.isNull())
        return;
    if (show)
        m_visibilityTogglesMenu->popup(pos);
    else
        m_visibilityTogglesMenu->close();
}

QMenu *Edit3DWidget::backgroundColorMenu() const
{
    return m_backgroundColorMenu.data();
}

void Edit3DWidget::showBackgroundColorMenu(bool show, const QPoint &pos)
{
    if (m_backgroundColorMenu.isNull())
        return;
    if (show)
        m_backgroundColorMenu->popup(pos);
    else
        m_backgroundColorMenu->close();
}

QMenu *Edit3DWidget::viewportPresetsMenu() const
{
    return m_viewportPresetsMenu.data();
}

void Edit3DWidget::showViewportPresetsMenu(bool show, const QPoint &pos)
{
    if (m_viewportPresetsMenu.isNull())
        return;
    if (show)
        m_viewportPresetsMenu->popup(pos);
    else
        m_viewportPresetsMenu->close();
}

void Edit3DWidget::showContextMenu(const QPoint &pos, const ModelNode &modelNode, const QVector3D &pos3d)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    m_contextMenuTarget = modelNode;
    m_contextMenuPos3d = pos3d;

    const bool isModel = modelNode.metaInfo().isQtQuick3DModel();
    const bool isNode = modelNode.metaInfo().isQtQuick3DNode();
    const bool allowAlign = view()->edit3DAction(View3DActionType::AlignCamerasToView)->action()->isEnabled();
    const bool isSingleComponent = view()->hasSingleSelectedModelNode() && modelNode.isComponent();
    const bool anyNodeSelected = view()->hasSelectedModelNodes();
    const bool selectionExcludingRoot = anyNodeSelected && !view()->rootModelNode().isSelected();

    auto bundleModuleIds = view()->model()->moduleIdsStartsWith(
        compUtils.componentBundlesTypePrefix().toUtf8(), Storage::ModuleKind::QmlLibrary);
    const bool isInBundle = bundleModuleIds.contains(modelNode.exportedTypeName().moduleId);

    if (m_createSubMenu)
        m_createSubMenu->setEnabled(!isSceneLocked());

    m_editComponentAction->setEnabled(isSingleComponent);
    m_materialsAction->setEnabled(isModel);
    m_duplicateAction->setEnabled(selectionExcludingRoot);
    m_copyAction->setEnabled(selectionExcludingRoot);
    m_pasteAction->setEnabled(isPasteAvailable());
    m_deleteAction->setEnabled(selectionExcludingRoot);
    m_fitSelectedAction->setEnabled(anyNodeSelected);
    m_alignCameraAction->setEnabled(allowAlign);
    m_alignViewAction->setEnabled(allowAlign);
    m_selectParentAction->setEnabled(selectionExcludingRoot);
    m_toggleGroupAction->setEnabled(true);
    m_bakeLightsAction->setVisible(view()->bakeLightsAction()->action()->isVisible());
    m_bakeLightsAction->setEnabled(view()->bakeLightsAction()->action()->isEnabled());
    m_addToContentLibAction->setEnabled(isNode && !isInBundle);
    m_exportBundleAction->setEnabled(isNode);
    m_materialsAction->updateMenu(view()->selectedModelNodes());

    if (m_view) {
        int idx = m_view->activeViewport();
        m_wireFrameAction->setChecked(m_view->viewportToolStates()[idx].showWireframe);
        for (QAction *a : std::as_const(m_matOverrideActions))
            a->setChecked(false);
        int type = m_view->viewportToolStates()[idx].matOverride;
        m_matOverrideActions[type]->setChecked(true);
    }

    m_contextMenu->popup(mapToGlobal(pos));
}

void Edit3DWidget::linkActivated([[maybe_unused]] const QString &link)
{
    Utils3D::addQuick3DImportAndView3D(m_view);
}

Edit3DCanvas *Edit3DWidget::canvas() const
{
    return m_canvas.data();
}

Edit3DView *Edit3DWidget::view() const
{
    return m_view.data();
}

void Edit3DWidget::dragEnterEvent(QDragEnterEvent *dragEnterEvent)
{
    // Block all drags if scene root node is locked
    if (m_view->hasModelNodeForInternalId(m_canvas->activeScene())) {
        ModelNode node = m_view->modelNodeForInternalId(m_canvas->activeScene());
        if (ModelUtils::isThisOrAncestorLocked(node))
            return;
    }

    m_draggedEntry = {};

    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    if (dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ASSETS)
        || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_TEXTURE)) {
        const auto urls = dragEnterEvent->mimeData()->urls();
        for (const QUrl &url : urls) {
            Asset asset(url.toLocalFile());
            if (asset.isImported3D() || asset.isTexture3D()) {
                dragEnterEvent->acceptProposedAction();
                break;
            }
        }
    } else if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData())
               || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_MATERIAL)
               || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_MATERIAL)
               || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_ITEM_3D)
               || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_TEXTURE)) {
        if (Utils3D::active3DSceneNode(m_view).isValid())
            dragEnterEvent->acceptProposedAction();
    } else if (dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO)) {
        QByteArray data = dragEnterEvent->mimeData()->data(Constants::MIME_TYPE_ITEM_LIBRARY_INFO);
        if (!data.isEmpty()) {
            QDataStream stream(data);
            stream >> m_draggedEntry;
            if (NodeHints::fromItemLibraryEntry(m_draggedEntry, view()->model()).canBeDroppedInView3D())
                dragEnterEvent->acceptProposedAction();
        }
    }
}

void Edit3DWidget::dropEvent(QDropEvent *dropEvent)
{
    dropEvent->accept();
    setFocus();
    const QPointF pos = m_canvas->mapFrom(this, dropEvent->position());

    // handle dropping materials and textures
    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_MATERIAL)
        || dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_TEXTURE)) {
        bool isMaterial = dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_MATERIAL);
        QByteArray data = dropEvent->mimeData()->data(isMaterial
                                          ? QString::fromLatin1(Constants::MIME_TYPE_MATERIAL)
                                          : QString::fromLatin1(Constants::MIME_TYPE_TEXTURE));
        if (ModelNode dropNode = m_view->modelNodeForInternalId(data.toInt())) {
            if (isMaterial)
                m_view->dropMaterial(dropNode, pos);
            else
                m_view->dropTexture(dropNode, pos);
        }
        m_view->model()->endDrag();
        return;
    }

    // handle dropping bundle materials
    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_MATERIAL)) {
        m_view->dropBundleMaterial(pos);
        m_view->model()->endDrag();
        return;
    }

    // handle dropping bundle items
    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_ITEM_3D)) {
        m_view->dropBundleItem(pos);
        m_view->model()->endDrag();
        return;
    }

    // handle dropping from component view
    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO)) {
        if (!m_draggedEntry.name().isEmpty())
            m_view->dropComponent(m_draggedEntry, pos);
        m_view->model()->endDrag();
        return;
    }

    // handle dropping image assets
    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ASSETS)
        || dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_TEXTURE)) {
        m_view->dropAssets(dropEvent->mimeData()->urls(), pos);
        m_view->model()->endDrag();
        return;
    }

    // handle dropping external assets
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    QHash<QString, QStringList> addedAssets = actionManager.handleExternalAssetsDrop(dropEvent->mimeData());

    view()->executeInTransaction("Edit3DWidget::dropEvent", [&] {
    // add 3D assets to 3d editor (QtQuick3D import will be added if missing)
        const QStringList added3DAssets = addedAssets.value(ComponentCoreConstants::add3DAssetsDisplayString);
        for (const QString &assetPath : added3DAssets) {
            QString fileName = QFileInfo(assetPath).baseName();
            fileName = fileName.at(0).toUpper() + fileName.mid(1); // capitalize first letter
            auto model = m_view->model();
            Utils::PathString import3dTypePrefix = QmlDesignerPlugin::instance()
                                                       ->documentManager()
                                                       .generatedComponentUtils()
                                                       .import3dTypePrefix();
            auto moduleId = model->module(import3dTypePrefix, Storage::ModuleKind::QmlLibrary);
            auto metaInfo = model->metaInfo(moduleId, fileName.toUtf8());
            if (auto entries = metaInfo.itemLibrariesEntries(); entries.size()) {
                auto entry = ItemLibraryEntry::create(m_view->model()->pathCache(), entries.front());
                QmlVisualNode::createQml3DNode(view(), entry, m_canvas->activeScene(), {}, false);
            }
        }
    });

    m_view->model()->endDrag();
}

} // namespace QmlDesigner
