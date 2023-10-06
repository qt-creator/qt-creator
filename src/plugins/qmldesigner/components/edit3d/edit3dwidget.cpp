// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dwidget.h"
#include "designdocument.h"
#include "designericons.h"
#include "edit3dactions.h"
#include "edit3dcanvas.h"
#include "edit3dtoolbarmenu.h"
#include "edit3dview.h"
#include "edit3dviewconfig.h"
#include "externaldependenciesinterface.h"
#include "materialutils.h"
#include "metainfo.h"
#include "modelnodeoperations.h"
#include "nodeabstractproperty.h"
#include "nodehints.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "qmleditormenu.h"
#include "qmlvisualnode.h"
#include "viewmanager.h"

#include <auxiliarydataproperties.h>
#include <designeractionmanager.h>
#include <designermcumanager.h>
#include <import.h>
#include <model/modelutils.h>
#include <nodeinstanceview.h>
#include <seekerslider.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <toolbox.h>
#include <utils/asset.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QVBoxLayout>

namespace QmlDesigner {

inline static QIcon contextIcon(const DesignerIcons::IconId &iconId)
{
    return DesignerActionManager::instance().contextIcon(iconId);
};

static QIcon getEntryIcon(const ItemLibraryEntry &entry)
{
    static const QMap<QString, DesignerIcons::IconId> itemLibraryDesignerIconId = {
        {"QtQuick3D.OrthographicCamera__Camera Orthographic", DesignerIcons::CameraOrthographicIcon},
        {"QtQuick3D.PerspectiveCamera__Camera Perspective", DesignerIcons::CameraPerspectiveIcon},
        {"QtQuick3D.DirectionalLight__Light Directional", DesignerIcons::LightDirectionalIcon},
        {"QtQuick3D.PointLight__Light Point", DesignerIcons::LightPointIcon},
        {"QtQuick3D.SpotLight__Light Spot", DesignerIcons::LightSpotIcon},
        {"QtQuick3D.Model__Cone", DesignerIcons::ModelConeIcon},
        {"QtQuick3D.Model__Cube", DesignerIcons::ModelCubeIcon},
        {"QtQuick3D.Model__Cylinder", DesignerIcons::ModelCylinderIcon},
        {"QtQuick3D.Model__Plane", DesignerIcons::ModelPlaneIcon},
        {"QtQuick3D.Model__Sphere", DesignerIcons::ModelSphereIcon},
    };

    QString entryKey = entry.typeName() + "__" + entry.name();
    if (itemLibraryDesignerIconId.contains(entryKey)) {
        return contextIcon(itemLibraryDesignerIconId.value(entryKey));
    }
    return QIcon(entry.libraryEntryIconPath());
}

Edit3DWidget::Edit3DWidget(Edit3DView *view)
    : m_view(view)
{
    setAcceptDrops(true);

    QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(sheet)));

    Core::Context context(Constants::C_QMLEDITOR3D);
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
                            a, action->menuId().constData(), context);
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

    createContextMenu();

    // Onboarding label contains instructions for new users how to get 3D content into the project
    m_onboardingLabel = new QLabel(this);
    m_onboardingLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    connect(m_onboardingLabel, &QLabel::linkActivated, this, &Edit3DWidget::linkActivated);
    fillLayout->addWidget(m_onboardingLabel.data());

    // Canvas is used to render the actual edit 3d view
    m_canvas = new Edit3DCanvas(this);
    fillLayout->addWidget(m_canvas.data());
    showCanvas(false);
}

void Edit3DWidget::createContextMenu()
{
    m_contextMenu = new QmlEditorMenu(this);

    m_editComponentAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::EditComponentIcon),
                tr("Edit Component"), [&] {
        DocumentManager::goIntoComponent(m_view->singleSelectedModelNode());
    });

    m_editMaterialAction = m_contextMenu->addAction(
                contextIcon(DesignerIcons::MaterialIcon),
                tr("Edit Material"), [&] {
        SelectionContext selCtx(m_view);
        selCtx.setTargetNode(m_contextMenuTarget);
        ModelNodeOperations::editMaterial(selCtx);
    });

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
    const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();
    if (mcuManager.isMCUProject()) {
        const QStringList mcuAllowedList = mcuManager.allowedImports();
        if (!mcuAllowedList.contains("QtQuick3d"))
            text = tr("3D view is not supported in MCU projects.");
    }

    if (text.isEmpty()) {
        if (m_view->externalDependencies().isQt6Project()) {
            QString labelText =
                tr("Your file does not import Qt Quick 3D.<br><br>"
                   "To create a 3D view, add the"
                   " <b>QtQuick3D</b>"
                   " module in the"
                   " <b>Components</b>"
                   " view or click"
                   " <a href=\"#add_import\"><span style=\"text-decoration:none;color:%1\">here</span></a>"
                   ".<br><br>"
                   "To import 3D assets, select"
                   " <b>+</b>"
                   " in the"
                   " <b>Assets</b>"
                   " view.");
            text = labelText.arg(Utils::creatorTheme()->color(Utils::Theme::TextColorLink).name());
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

    m_nameToEntry.clear();

    m_createSubMenu = new QmlEditorMenu(tr("Create"), m_contextMenu);
    m_createSubMenu->setIcon(contextIcon(DesignerIcons::CreateIcon));
    m_contextMenu->addMenu(m_createSubMenu);

    const QString docPath = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName().toString();

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

            QAction *action = catMenu->addAction(
                        getEntryIcon(entry),
                        entry.name(),
                        this,
                        &Edit3DWidget::onCreateAction);
            action->setData(entry.name());
            m_nameToEntry.insert(entry.name(), entry);
        }
    }
}

// Action triggered from the "create" sub-menu
void Edit3DWidget::onCreateAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action || !m_view || !m_view->model() || isSceneLocked())
        return;

    m_view->executeInTransaction(__FUNCTION__, [&] {
        ItemLibraryEntry entry = m_nameToEntry.value(action->data().toString());
        Import import = Import::createLibraryImport(entry.requiredImport(),
                                                    QString::number(entry.majorVersion())
                                                    + QLatin1Char('.')
                                                    + QString::number(entry.minorVersion()));
        if (!m_view->model()->hasImport(import, true, true))
            m_view->model()->changeImports({import}, {});

        int activeScene = -1;
        auto data = m_view->rootModelNode().auxiliaryData(active3dSceneProperty);
        if (data)
            activeScene = data->toInt();
        auto modelNode = QmlVisualNode::createQml3DNode(m_view, entry,
                                                        activeScene, m_contextMenuPos3d).modelNode();
        QTC_ASSERT(modelNode.isValid(), return);
        m_view->setSelectedModelNode(modelNode);

        // if added node is a Model, assign it a material
        if (modelNode.metaInfo().isQtQuick3DModel())
            MaterialUtils::assignMaterialTo3dModel(m_view, modelNode);
    });
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

void Edit3DWidget::showContextMenu(const QPoint &pos, const ModelNode &modelNode, const QVector3D &pos3d)
{
    m_contextMenuTarget = modelNode;
    m_contextMenuPos3d = pos3d;

    const bool isModel = modelNode.metaInfo().isQtQuick3DModel();
    const bool allowAlign = view()->edit3DAction(View3DActionType::AlignCamerasToView)->action()->isEnabled();
    const bool isSingleComponent = view()->hasSingleSelectedModelNode() && modelNode.isComponent();
    const bool anyNodeSelected = view()->hasSelectedModelNodes();
    const bool selectionExcludingRoot = anyNodeSelected && !view()->rootModelNode().isSelected();

    if (m_createSubMenu)
        m_createSubMenu->setEnabled(!isSceneLocked());

    m_editComponentAction->setEnabled(isSingleComponent);
    m_editMaterialAction->setEnabled(isModel);
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

    m_contextMenu->popup(mapToGlobal(pos));
}

void Edit3DWidget::linkActivated([[maybe_unused]] const QString &link)
{
    if (m_view)
        m_view->addQuick3DImport();
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
        if (!urls.isEmpty()) {
            if (Asset(urls.first().toLocalFile()).isValidTextureSource())
                dragEnterEvent->acceptProposedAction();
        }
    } else if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData())
               || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_MATERIAL)
               || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_MATERIAL)
               || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_EFFECT)
               || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_TEXTURE)) {
        if (m_view->active3DSceneNode().isValid())
            dragEnterEvent->acceptProposedAction();
    } else if (dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO)) {
        QByteArray data = dragEnterEvent->mimeData()->data(Constants::MIME_TYPE_ITEM_LIBRARY_INFO);
        if (!data.isEmpty()) {
            QDataStream stream(data);
            stream >> m_draggedEntry;
            if (NodeHints::fromItemLibraryEntry(m_draggedEntry).canBeDroppedInView3D())
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

    // handle dropping bundle effects
    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_EFFECT)) {
        m_view->dropBundleEffect(pos);
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
        m_view->dropAsset(dropEvent->mimeData()->urls().first().toLocalFile(), pos);
        m_view->model()->endDrag();
        return;
    }

    // handle dropping external assets
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    QHash<QString, QStringList> addedAssets = actionManager.handleExternalAssetsDrop(dropEvent->mimeData());

    view()->executeInTransaction("Edit3DWidget::dropEvent", [&] {
        // add 3D assets to 3d editor (QtQuick3D import will be added if missing)
        ItemLibraryInfo *itemLibInfo = m_view->model()->metaInfo().itemLibraryInfo();

        const QStringList added3DAssets = addedAssets.value(ComponentCoreConstants::add3DAssetsDisplayString);
        for (const QString &assetPath : added3DAssets) {
            QString fileName = QFileInfo(assetPath).baseName();
            fileName = fileName.at(0).toUpper() + fileName.mid(1); // capitalize first letter
            QString type = QString("Quick3DAssets.%1.%1").arg(fileName);
            QList<ItemLibraryEntry> entriesForType = itemLibInfo->entriesForType(type.toLatin1());
            if (!entriesForType.isEmpty()) { // should always be true, but just in case
                QmlVisualNode::createQml3DNode(view(), entriesForType.at(0),
                                               m_canvas->activeScene(), {}, false).modelNode();
            }
        }
    });

    m_view->model()->endDrag();
}

} // namespace QmlDesigner
