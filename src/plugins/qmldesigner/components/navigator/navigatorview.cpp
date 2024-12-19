// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatorview.h"

#include "designeractionmanager.h"
#include "designericons.h"
#include "iconcheckboxitemdelegate.h"
#include "nameitemdelegate.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"

#include <assetslibrarywidget.h>
#include <bindingproperty.h>
#include <commontypecache.h>
#include <designersettings.h>
#include <designmodecontext.h>
#include <itemlibraryentry.h>
#include <modelutils.h>
#include <nodeinstanceview.h>
#include <nodelistproperty.h>
#include <nodeproperty.h>
#include <rewritingexception.h>
#include <theme.h>
#include <utils3d.h>
#include <variantproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignericons.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/utilsicons.h>
#include <utils/stylehelper.h>

#include <QHeaderView>
#include <QMimeData>
#include <QPixmap>
#include <QTimer>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <QMenu>

using namespace Core;

inline static void setScenePos(const QmlDesigner::ModelNode &modelNode, const QPointF &pos)
{
    if (modelNode.hasParentProperty() && QmlDesigner::QmlItemNode::isValidQmlItemNode(modelNode.parentProperty().parentModelNode())) {
        QmlDesigner::QmlItemNode parentNode = modelNode.parentProperty().parentModelNode();

        if (!parentNode.modelNode().metaInfo().isLayoutable()) {
            QPointF localPos = parentNode.instanceSceneTransform().inverted().map(pos);
            modelNode.variantProperty("x").setValue(localPos.toPoint().x());
            modelNode.variantProperty("y").setValue(localPos.toPoint().y());
        } else { //Items in Layouts do not have a position
            modelNode.removeProperty("x");
            modelNode.removeProperty("y");
        }
    }
}

inline static void moveNodesUp(const QList<QmlDesigner::ModelNode> &nodes)
{
    for (const auto &node : nodes) {
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty()) {
            int oldIndex = node.parentProperty().indexOf(node);
            int index = oldIndex;
            index--;
            if (index < 0)
                index = node.parentProperty().count() - 1; //wrap around
            if (oldIndex != index)
                node.parentProperty().toNodeListProperty().slide(oldIndex, index);
        }
    }
}

inline static void moveNodesDown(const QList<QmlDesigner::ModelNode> &nodes)
{
    for (const auto &node : nodes) {
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty()) {
            int oldIndex = node.parentProperty().indexOf(node);
            int index = oldIndex;
            index++;
            if (index >= node.parentProperty().count())
                index = 0; //wrap around
            if (oldIndex != index)
                node.parentProperty().toNodeListProperty().slide(oldIndex, index);
        }
    }
}

namespace QmlDesigner {

NavigatorView::NavigatorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_blockSelectionChangedSignal(false)
{

}

NavigatorView::~NavigatorView()
{
    if (m_widget && !m_widget->parent())
        delete m_widget.data();
}

bool NavigatorView::hasWidget() const
{
    return true;
}

WidgetInfo NavigatorView::widgetInfo()
{
    if (!m_widget)
        setupWidget();

    return createWidgetInfo(m_widget.data(),
                            QStringLiteral("Navigator"),
                            WidgetInfo::LeftPane,
                            tr("Navigator"),
                            tr("Navigator view"));
}

void NavigatorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    QTreeView *treeView = treeWidget();

    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Name,
                                             QHeaderView::Stretch);
    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Alias,
                                             QHeaderView::Fixed);
    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Visibility,
                                             QHeaderView::Fixed);
    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Lock, QHeaderView::Fixed);

    treeView->header()->setStretchLastSection(false);
    treeView->header()->setMinimumSectionSize(24);
    treeView->header()->setDefaultSectionSize(24);

    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Alias, 24);
    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Visibility, 24);
    // Make last column a bit wider to compensate the shift to the left due to vertical scrollbar
    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Lock, 32);
    treeView->setIndentation(20);

    m_currentModelInterface->setFilter(false);
    m_currentModelInterface->setNameFilter("");
    m_widget->clearSearch();

    QTimer::singleShot(0, this, [this, treeView]() {
        m_currentModelInterface->setFilter(
                    QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS).toBool());

        m_currentModelInterface->setOrder(
                    QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER).toBool());

        // Expand everything to begin with to ensure model node to index cache is populated
        treeView->expandAll();

        if (AbstractView::model() && m_expandMap.contains(AbstractView::model()->fileUrl())) {
            const QHash<QString, bool> localExpandMap = m_expandMap[AbstractView::model()->fileUrl()];
            auto it = localExpandMap.constBegin();
            while (it != localExpandMap.constEnd()) {
                const ModelNode node = modelNodeForId(it.key());
                // When editing subcomponent, the current root node may be marked collapsed in the
                // full file view, but we never want to actually collapse it, so skip it.
                if (!node.isRootNode()) {
                    const QModelIndex index = indexForModelNode(node);
                    if (index.isValid())
                        treeWidget()->setExpanded(index, it.value());
                }
                ++it;
            }
        }
    });

    clearExplorerWarnings();

    ActionContainer *addmenu = ActionManager::createMenu("QtCreator.Menu.Add");
    addmenu->menu()->setTitle(tr("&Add"));
    addmenu->appendGroup("QtCreator.Group.Add.Create2D");
    addmenu->appendGroup("QtCreator.Group.Add.Create3D");
    addmenu->setOnAllDisabledBehavior(ActionContainer::Show);

    ActionContainer *create2d = ActionManager::createMenu("QtCreator.Menu.Add.Create2D");
    addmenu->addMenu(create2d, "QtCreator.Group.Add.Create2D");
    create2d->menu()->setTitle(tr("&Create 2D"));
    create2d->appendGroup("QtCreator.Group.Add.Create2D");
    create2d->appendGroup("QtCreator.Group.Add.Create2D.Animation");
    create2d->appendGroup("QtCreator.Group.Add.Create2D.Buttons");
    create2d->appendGroup("QtCreator.Group.Add.Create2D.Layouts");
    create2d->appendGroup("QtCreator.Group.Add.Create2D.Container");
    create2d->appendGroup("QtCreator.Group.Add.Create2D.Display");
    create2d->appendGroup("QtCreator.Group.Add.Create2D.Input");
    create2d->appendGroup("QtCreator.Group.Add.Create2D.Instances");
    create2d->appendGroup("QtCreator.Group.Add.Create2D.Views");
    create2d->setOnAllDisabledBehavior(ActionContainer::Disable);
    create2d->menu()->setDisabled(false);

    ActionContainer *animation = ActionManager::createMenu("QtCreator.Menu.Add.Create2D.Animation");
    create2d->addMenu(animation, "QtCreator.Group.Add.Create2D.Animation");
    animation->menu()->setTitle(tr("&Animation"));
    animation->appendGroup("QtCreator.Group.Add.Create2D.Animation");
    animation->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *buttons = ActionManager::createMenu("QtCreator.Menu.Add.Create2D.Buttons");
    create2d->addMenu(buttons, "QtCreator.Group.Add.Create2D.Buttons");
    buttons->menu()->setTitle(tr("&Buttons"));
    buttons->appendGroup("QtCreator.Group.Add.Create2D.Buttons");
    buttons->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *layouts = ActionManager::createMenu("QtCreator.Menu.Add.Create2D.Layouts");
    create2d->addMenu(layouts, "QtCreator.Group.Add.Create2D.Layouts");
    layouts->menu()->setTitle(tr("&Layouts"));
    layouts->appendGroup("QtCreator.Group.Add.Create2D.Layouts");
    layouts->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *container = ActionManager::createMenu("QtCreator.Menu.Add.Create2D.Container");
    create2d->addMenu(container, "QtCreator.Group.Add.Create2D.Container");
    container->menu()->setTitle(tr("&Container"));
    container->appendGroup("QtCreator.Group.Add.Create2D.Container");
    container->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *display = ActionManager::createMenu("QtCreator.Menu.Add.Create2D.Display");
    create2d->addMenu(display, "QtCreator.Group.Add.Create2D.Display");
    display->menu()->setTitle(tr("&Display"));
    display->appendGroup("QtCreator.Group.Add.Create2D.Display");
    display->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *input = ActionManager::createMenu("QtCreator.Menu.Add.Create2D.Input");
    create2d->addMenu(input, "QtCreator.Group.Add.Create2D.Input");
    input->menu()->setTitle(tr("&Input"));
    input->appendGroup("QtCreator.Group.Add.Create2D.Input");
    input->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *instances = ActionManager::createMenu("QtCreator.Menu.Add.Create2D.Instances");
    create2d->addMenu(instances, "QtCreator.Group.Add.Create2D.Instances");
    instances->menu()->setTitle(tr("&Instances"));
    instances->appendGroup("QtCreator.Group.Add.Create2D.Instances");
    instances->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *views = ActionManager::createMenu("QtCreator.Menu.Add.Create2D.Views");
    create2d->addMenu(views, "QtCreator.Group.Add.Create2D.Views");
    views->menu()->setTitle(tr("&Views"));
    views->appendGroup("QtCreator.Group.Add.Create2D.Views");
    views->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *create3d = ActionManager::createMenu("QtCreator.Menu.Add.Create3D");
    addmenu->addMenu(create3d, "QtCreator.Group.Add.Create2D");
    create3d->menu()->setTitle(tr("&Create 3D"));
    create3d->appendGroup("QtCreator.Group.Add.Create3D");
    create3d->appendGroup("QtCreator.Group.Add.Create3D.Cameras");
    create3d->appendGroup("QtCreator.Group.Add.Create3D.Lights");
    create3d->appendGroup("QtCreator.Group.Add.Create3D.Primitives");
    create3d->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *cameras = ActionManager::createMenu("QtCreator.Menu.Add.Create3D.Cameras");
    create3d->addMenu(cameras, "QtCreator.Group.Add.Create3D.Cameras");
    cameras->menu()->setTitle(tr("&Cameras"));
    cameras->appendGroup("QtCreator.Group.Add.Create3D.Cameras");
    cameras->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *lights = ActionManager::createMenu("QtCreator.Menu.Add.Create3D.Lights");
    create3d->addMenu(lights, "QtCreator.Group.Add.Create3D.Lights");
    lights->menu()->setTitle(tr("&Lights"));
    lights->appendGroup("QtCreator.Group.Add.Create3D.Lights");
    lights->setOnAllDisabledBehavior(ActionContainer::Disable);

    ActionContainer *primitives = ActionManager::createMenu("QtCreator.Menu.Add.Create3D.Primitives");
    create3d->addMenu(primitives, "QtCreator.Group.Add.Create3D.Primitives");
    primitives->menu()->setTitle(tr("&Primitives"));
    primitives->appendGroup("QtCreator.Group.Add.Create3D.Primitives");
    primitives->setOnAllDisabledBehavior(ActionContainer::Disable);

    struct actionData{
        Utils::Id name;
        const char* text;
        Utils::Id containerId;
        Utils::Id groupId;
        Utils::Id command;
    };

    QList<actionData> actionDataList = {
        {"QtCreator.View3D", "View3D", "QtCreator.Menu.Add.Create2D", "QtCreator.Group.Add.Create2D", "QmlDesigner.CreateView3DContext2D"},
        //Animation
        {"QtCreator.FrameAnimation", "Frame Animation", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreateFrameAnimation"},
        {"QtCreator.ColorAnimation", "Color Animation", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreateColorAnimation"},
        {"QtCreator.NumberAnimation", "Number Animation", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreateNumberAnimation"},
        {"QtCreator.ParallelAnimation", "Parallel Animation", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreateParallelAnimation"},
        {"QtCreator.PauseAnimation", "Pause Animation", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreatePauseAnimation"},
        {"QtCreator.PropertyAnimation", "Property Animation", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreatePropertyAnimation"},
        {"QtCreator.ScriptAction", "Script Action", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreateScriptAction"},
        {"QtCreator.SequentialAnimation", "Sequential Animation", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreateSequentialAnimation"},
        {"QtCreator.Timer", "Timer", "QtCreator.Menu.Add.Create2D.Animation", "QtCreator.Group.Add.Create2D.Animation", "QmlDesigner.CreateTimer"},
        // Buttons
        {"QtCreator.Button", "Button", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateButton"},
        {"QtCreator.CheckBox", "Check Box", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateCheckBox"},
        {"QtCreator.CheckDelegate", "Check Delegate", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateCheckDelegate"},
        {"QtCreator.DelayButton", "Delay Button", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateDelayButton"},
        {"QtCreator.Dial", "Dial", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateDial"},
        {"QtCreator.RadioButton", "Radio Button", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateRadioButton"},
        {"QtCreator.RadioDelegate", "Radio Delegate", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateRadioDelegate"},
        {"QtCreator.RoundButton", "Round Button", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateRoundButton"},
        {"QtCreator.Switch", "Switch", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateSwitch"},
        {"QtCreator.SwitchDelegate", "Switch Delegate", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateSwitchDelegate"},
        {"QtCreator.TabButton", "Tab Button", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateTabButton"},
        {"QtCreator.ToolButton", "Tool Button", "QtCreator.Menu.Add.Create2D.Buttons", "QtCreator.Group.Add.Create2D.Buttons", "QmlDesigner.CreateToolButton"},
        // Layouts
        {"QtCreator.Column", "Column", "QtCreator.Menu.Add.Create2D.Layouts", "QtCreator.Group.Add.Create2D.Layouts", "QmlDesigner.CreateColumn"},
        {"QtCreator.Flow", "Flow", "QtCreator.Menu.Add.Create2D.Layouts", "QtCreator.Group.Add.Create2D.Layouts", "QmlDesigner.CreateFlow"},
        {"QtCreator.Grid", "Grid", "QtCreator.Menu.Add.Create2D.Layouts", "QtCreator.Group.Add.Create2D.Layouts", "QmlDesigner.CreateGrid"},
        {"QtCreator.Row", "Row", "QtCreator.Menu.Add.Create2D.Layouts", "QtCreator.Group.Add.Create2D.Layouts", "QmlDesigner.CreateRow"},
        // Containers
        {"QtCreator.Control", "Control", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreateControl"},
        {"QtCreator.Flickable", "Flickable", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreateFlickable"},
        {"QtCreator.Frame", "Frame", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreateFrame"},
        {"QtCreator.Item", "Item", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreateItem"},
        {"QtCreator.ItemDelegate", "Item Delegate", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreateItemDelegate"},
        {"QtCreator.Page", "Page", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreatePage"},
        {"QtCreator.Pane", "Pane", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreatePane"},
        {"QtCreator.TabBar", "Tab Bar", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreateTabBar"},
        {"QtCreator.ToolBar", "ToolBar", "QtCreator.Menu.Add.Create2D.Container", "QtCreator.Group.Add.Create2D.Container", "QmlDesigner.CreateToolBar"},
        //Display
        {"QtCreator.AnimatedImage", "Animated Image", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateAnimatedImage"},
        {"QtCreator.AnimatedSprite", "Animated Sprite", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateAnimatedSprite"},
        {"QtCreator.BorderImage", "Border Image", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateBorderImage"},
        {"QtCreator.BusyIndicator", "Busy Indicator", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateBusyIndicator"},
        {"QtCreator.Image", "Image", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateImage"},
        {"QtCreator.ProgressBar", "Progress Bar", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateProgressBar"},
        {"QtCreator.Rectangle", "Rectangle", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateRectangle"},
        {"QtCreator.Text", "Text", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateText"},
        {"QtCreator.TextArea", "Text Area", "QtCreator.Menu.Add.Create2D.Display", "QtCreator.Group.Add.Create2D.Display", "QmlDesigner.CreateTextArea"},
        //Input
        {"QtCreator.DropArea", "Drop Area", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateDropArea"},
        {"QtCreator.FocusScope", "Focus Scope", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateFocusScope"},
        {"QtCreator.MouseArea", "Mouse Area", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateMouseArea"},
        {"QtCreator.RangeSlider", "Range Slider", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateRangeSlider"},
        {"QtCreator.Slider", "Slider", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateSlider"},
        {"QtCreator.SpinBox", "Spin Box", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateSpinBox"},
        {"QtCreator.TextEdit", "Text Edit", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateTextEdit"},
        {"QtCreator.TextField", "Text Field", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateTextField"},
        {"QtCreator.TextInput", "Text Input", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateTextInput"},
        {"QtCreator.Tumbler", "Tumbler", "QtCreator.Menu.Add.Create2D.Input", "QtCreator.Group.Add.Create2D.Input", "QmlDesigner.CreateTumbler"},
        //Views
        {"QtCreator.GridView", "Grid View", "QtCreator.Menu.Add.Create2D.Views", "QtCreator.Group.Add.Create2D.Views", "QmlDesigner.CreateGridView"},
        {"QtCreator.ListView", "List View", "QtCreator.Menu.Add.Create2D.Views", "QtCreator.Group.Add.Create2D.Views", "QmlDesigner.CreateListView"},
        {"QtCreator.PathView", "Path View", "QtCreator.Menu.Add.Create2D.Views", "QtCreator.Group.Add.Create2D.Views", "QmlDesigner.CreatePathView"},
        {"QtCreator.ScrollView", "Scroll View", "QtCreator.Menu.Add.Create2D.Views", "QtCreator.Group.Add.Create2D.Views", "QmlDesigner.CreateScrollView"},
        {"QtCreator.StackView", "Stack View", "QtCreator.Menu.Add.Create2D.Views", "QtCreator.Group.Add.Create2D.Views", "QmlDesigner.CreateStackView"},
        {"QtCreator.SwipeDelegate", "Swipe Delegate", "QtCreator.Menu.Add.Create2D.Views", "QtCreator.Group.Add.Create2D.Views", "QmlDesigner.CreateSwipeDelegate"},
        {"QtCreator.SwipeView", "Swipe View", "QtCreator.Menu.Add.Create2D.Views", "QtCreator.Group.Add.Create2D.Views", "QmlDesigner.CreateSwipeView"},
        //Instances
        {"QtCreator.Loader", "Loader", "QtCreator.Menu.Add.Create2D.Instances", "QtCreator.Group.Add.Create2D.Instances", "QmlDesigner.CreateLoader"},
        {"QtCreator.Repeater", "Repeater", "QtCreator.Menu.Add.Create2D.Instances", "QtCreator.Group.Add.Create2D.Instances", "QmlDesigner.CreateRepeater"},
        //3D
        {"QtCreator.View3Dd", "View3D", "QtCreator.Menu.Add.Create3D", "QtCreator.Group.Add.Create3D", "QmlDesigner.CreateView3DContext3D"},
        {"QtCreator.Node", "Node", "QtCreator.Menu.Add.Create3D", "QtCreator.Group.Add.Create3D", "QmlDesigner.CreateNode"},
        //Cameras
        {"QtCreator.OrthographicCamera", "Orthographic Camera", "QtCreator.Menu.Add.Create3D.Cameras", "QtCreator.Group.Add.Create3D.Cameras", "QmlDesigner.CreateOrthographicCamera"},
        {"QtCreator.PerspectiveCamera", "Perspective Camera", "QtCreator.Menu.Add.Create3D.Cameras", "QtCreator.Group.Add.Create3D.Cameras", "QmlDesigner.CreatePerspectiveCamera"},
        //Lights
        {"QtCreator.DirectionalLight", "Directional Light", "QtCreator.Menu.Add.Create3D.Lights", "QtCreator.Group.Add.Create3D.Lights", "QmlDesigner.CreateDirectionalLight"},
        {"QtCreator.PointLight", "Point Light", "QtCreator.Menu.Add.Create3D.Lights", "QtCreator.Group.Add.Create3D.Lights", "QmlDesigner.CreatePointLight"},
        {"QtCreator.SpotLight", "Spot Light", "QtCreator.Menu.Add.Create3D.Lights", "QtCreator.Group.Add.Create3D.Lights", "QmlDesigner.CreateSpotLight"},
        {"QtCreator.ReflectionProbe", "ReflectionProbe", "QtCreator.Menu.Add.Create3D.Lights", "QtCreator.Group.Add.Create3D.Lights", "QmlDesigner.CreateReflectionProbe"},
        //Primitives
        {"QtCreator.Cone", "Cone", "QtCreator.Menu.Add.Create3D.Primitives", "QtCreator.Group.Add.Create3D.Primitives", "QmlDesigner.CreateCone"},
        {"QtCreator.Cube", "Cube", "QtCreator.Menu.Add.Create3D.Primitives", "QtCreator.Group.Add.Create3D.Primitives", "QmlDesigner.CreateCube"},
        {"QtCreator.Cylinder", "Cylinder", "QtCreator.Menu.Add.Create3D.Primitives", "QtCreator.Group.Add.Create3D.Primitives", "QmlDesigner.CreateCylinder"},
        {"QtCreator.Model", "Model", "QtCreator.Menu.Add.Create3D.Primitives", "QtCreator.Group.Add.Create3D.Primitives", "QmlDesigner.CreateModel"},
        {"QtCreator.Plane", "Plane", "QtCreator.Menu.Add.Create3D.Primitives", "QtCreator.Group.Add.Create3D.Primitives", "QmlDesigner.CreatePlane"},
        {"QtCreator.Sphere", "Sphere", "QtCreator.Menu.Add.Create3D.Primitives", "QtCreator.Group.Add.Create3D.Primitives", "QmlDesigner.CreateSphere"},
    };

    for (auto& data : actionDataList) {
        Core::ActionBuilder action(this, data.name);
        action.setText(tr(data.text));
        action.addToContainer(data.containerId, data.groupId);
        action.addOnTriggered(this, [data] {
            Core::ActionManager::command(data.command)->action()->trigger();
        });
        action.setEnabled(Core::ActionManager::command(data.command)->action()->isEnabled());

    }

    QObject::connect(treeWidget()->selectionModel(), &QItemSelectionModel::selectionChanged,
        [this, actionDataList](const QItemSelection &selected, const QItemSelection &deselected) {
            for (auto data : actionDataList) {
               Core::ActionBuilder action(this, data.name);
               action.setText(tr(data.text));
               action.addToContainer(data.containerId, data.groupId);
               action.addOnTriggered(this, [data] {
               Core::ActionManager::command(data.command)->action()->trigger();
            });
            action.setEnabled(Core::ActionManager::command(data.command)->action()->isEnabled());
        }
    });
}

void NavigatorView::clearExplorerWarnings()
{
    QList<ModelNode> allNodes;
    addNodeAndSubModelNodesToList(rootModelNode(), allNodes);
    for (ModelNode node : allNodes) {
        if (node.metaInfo().isFileComponent()) {
            const ProjectExplorer::FileNode *fnode = fileNodeForModelNode(node);
            if (fnode)
                fnode->setHasError(false);
        }
    }
}

void NavigatorView::addNodeAndSubModelNodesToList(const ModelNode &node, QList<ModelNode> &nodes)
{
    nodes.append(node);
    for (ModelNode subNode : node.allSubModelNodes()) {
        addNodeAndSubModelNodesToList(subNode, nodes);
    }
}

void NavigatorView::modelAboutToBeDetached(Model *model)
{
    QHash<QString, bool> &localExpandMap = m_expandMap[model->fileUrl()];

    // If detaching full document model, recreate expand map from scratch to remove stale entries.
    // Otherwise just update it (subcomponent edit case).
    bool fullUpdate = true;
    if (DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument())
        fullUpdate = !document->inFileComponentModelActive();
    if (fullUpdate)
        localExpandMap.clear();

    if (currentModel()) {
        // Store expand state of the navigator tree
        const ModelNode rootNode = rootModelNode();
        const QModelIndex rootIndex = indexForModelNode(rootNode);

        std::function<void(const QModelIndex &)> gatherExpandedState;
        gatherExpandedState = [&](const QModelIndex &index) {
            if (index.isValid()) {
                const int rowCount = currentModel()->rowCount(index);
                for (int i = 0; i < rowCount; ++i) {
                    const QModelIndex childIndex = currentModel()->index(i, 0, index);
                    if (const ModelNode node = modelNodeForIndex(childIndex)) {
                        // Just store collapsed states as everything is expanded by default
                        if (!treeWidget()->isExpanded(childIndex))
                            localExpandMap.insert(node.id(), false);
                        else if (!fullUpdate)
                            localExpandMap.remove(node.id());
                    }
                    gatherExpandedState(childIndex);
                }
            }
        };
        gatherExpandedState(rootIndex);
    }

    AbstractView::modelAboutToBeDetached(model);
}

void NavigatorView::importsChanged(const Imports &/*addedImports*/, const Imports &/*removedImports*/)
{
    treeWidget()->update();
}

void NavigatorView::bindingPropertiesChanged(const QList<BindingProperty> & propertyList, PropertyChangeFlags /*propertyChange*/)
{
    bool needReset = false;

    for (const BindingProperty &bindingProperty : propertyList) {
        /* If a binding property that exports an item using an alias property has
         * changed, we have to update the affected item.
         */

        if (bindingProperty.isAliasExport())
            m_currentModelInterface->notifyDataChanged(modelNodeForId(bindingProperty.expression()));

        if (bindingProperty.name() == "materials")
            needReset = true;
    }

    if (needReset) {
        m_treeModel->resetModel();
        treeWidget()->expandAll();
    }
}

void NavigatorView::dragStarted(QMimeData *mimeData)
{
    if (mimeData->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO)) {
        QByteArray data = mimeData->data(Constants::MIME_TYPE_ITEM_LIBRARY_INFO);
        QDataStream stream(data);
        ItemLibraryEntry itemLibraryEntry;
        stream >> itemLibraryEntry;

        m_widget->setDragType(itemLibraryEntry.typeName());
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_TEXTURE)) {
        qint32 internalId = mimeData->data(Constants::MIME_TYPE_TEXTURE).toInt();
        ModelNode texNode = modelNodeForInternalId(internalId);
#ifdef QDS_USE_PROJECTSTORAGE
        m_widget->setDragType(texNode.type());
#else
        m_widget->setDragType(texNode.metaInfo().typeName());
#endif
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_MATERIAL)) {
        qint32 internalId = mimeData->data(Constants::MIME_TYPE_MATERIAL).toInt();
        ModelNode matNode = modelNodeForInternalId(internalId);
#ifdef QDS_USE_PROJECTSTORAGE
        m_widget->setDragType(matNode.type());
#else
        m_widget->setDragType(matNode.metaInfo().typeName());
#endif
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_TEXTURE)) {
        m_widget->setDragType(Constants::MIME_TYPE_BUNDLE_TEXTURE);
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_MATERIAL)) {
        QByteArray data = mimeData->data(Constants::MIME_TYPE_BUNDLE_MATERIAL);
        QDataStream stream(data);
        TypeName bundleMatType;
        stream >> bundleMatType;

        m_widget->setDragType(bundleMatType);
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_ASSETS)) {
        const QStringList assetsPaths = QString::fromUtf8(mimeData->data(Constants::MIME_TYPE_ASSETS)).split(',');
        if (assetsPaths.size() > 0) {
            auto assetTypeAndData = AssetsLibraryWidget::getAssetTypeAndData(assetsPaths[0]);
            QString assetType = assetTypeAndData.first;
            if (assetType == Constants::MIME_TYPE_ASSET_EFFECT) {
                // We use arbitrary type name because at this time we don't have effect composer
                // specific type
                m_widget->update();
            } else if (assetType == Constants::MIME_TYPE_ASSET_TEXTURE3D) {
                m_widget->setDragType(Constants::MIME_TYPE_ASSET_TEXTURE3D);
                m_widget->update();
            } else if (assetType == Constants::MIME_TYPE_ASSET_IMAGE) {
                m_widget->setDragType(Constants::MIME_TYPE_ASSET_IMAGE);
                m_widget->update();
            } else if (assetType == Constants::MIME_TYPE_ASSET_NODEGRAPH) {
                m_widget->setDragType(Constants::MIME_TYPE_ASSET_NODEGRAPH);
                m_widget->update();
            }
        }
    }
}

void NavigatorView::dragEnded()
{
    m_widget->setDragType("");
    m_widget->update();
}

void NavigatorView::customNotification([[maybe_unused]] const AbstractView *view,
                                       const QString &identifier,
                                       [[maybe_unused]] const QList<ModelNode> &nodeList,
                                       [[maybe_unused]] const QList<QVariant> &data)
{
    if (identifier == "asset_import_update")
        m_currentModelInterface->notifyIconsChanged();
}

void NavigatorView::handleChangedExport(const ModelNode &modelNode, bool exported)
{
    const ModelNode rootNode = rootModelNode();
    Q_ASSERT(rootNode.isValid());
    const PropertyName modelNodeId = modelNode.id().toUtf8();
    if (rootNode.hasProperty(modelNodeId))
        rootNode.removeProperty(modelNodeId);
    if (exported) {
        executeInTransaction("NavigatorTreeModel:exportItem", [modelNode](){
            QmlObjectNode qmlObjectNode(modelNode);
            qmlObjectNode.ensureAliasExport();
        });
    }
}

bool NavigatorView::isNodeInvisible(const ModelNode &modelNode) const
{
    return QmlVisualNode(modelNode).visibilityOverride();
}

void NavigatorView::disableWidget()
{
    if (m_widget)
        m_widget->disableNavigator();
}

void NavigatorView::enableWidget()
{
    if (m_widget)
        m_widget->enableNavigator();
}

void NavigatorView::modelNodePreviewPixmapChanged(const ModelNode &node,
                                                  const QPixmap &pixmap,
                                                  const QByteArray &requestId)
{
    // There might be multiple requests for different preview pixmap sizes.
    // Here only the one with the default size is picked.
    if (requestId.isEmpty())
        m_treeModel->updateToolTipPixmap(node, pixmap);
}

ModelNode NavigatorView::modelNodeForIndex(const QModelIndex &modelIndex) const
{
    return modelIndex.model()->data(modelIndex, ModelNodeRole).value<ModelNode>();
}

void NavigatorView::nodeAboutToBeRemoved(const ModelNode & /*removedNode*/)
{
}

void NavigatorView::nodeRemoved(const ModelNode &removedNode,
                                const NodeAbstractProperty & /*parentProperty*/,
                                AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    m_currentModelInterface->notifyModelNodesRemoved({removedNode});
}

void NavigatorView::nodeReparented(const ModelNode &modelNode,
                                   const NodeAbstractProperty & /*newPropertyParent*/,
                                   const NodeAbstractProperty & oldPropertyParent,
                                   AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (!oldPropertyParent.isValid())
        m_currentModelInterface->notifyModelNodesInserted({modelNode});
    else
        m_currentModelInterface->notifyModelNodesMoved({modelNode});
    treeWidget()->expand(indexForModelNode(modelNode));

    // make sure selection is in sync again
    QTimer::singleShot(0, this, &NavigatorView::updateItemSelection);
}

void NavigatorView::nodeIdChanged(const ModelNode& modelNode, const QString & /*newId*/, const QString & /*oldId*/)
{
    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &/*propertyList*/)
{
}

void NavigatorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    QList<ModelNode> modelNodes;
    for (const AbstractProperty &property : propertyList) {
        if (property.isNodeAbstractProperty()) {
            NodeAbstractProperty nodeAbstractProperty(property.toNodeListProperty());
            modelNodes.append(nodeAbstractProperty.directSubNodes());
        }
    }

    m_currentModelInterface->notifyModelNodesRemoved(modelNodes);
}

void NavigatorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    m_currentModelInterface->notifyDataChanged(rootModelNode());
}

void NavigatorView::nodeTypeChanged(const ModelNode &modelNode, const TypeName &, int , int)
{
    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::auxiliaryDataChanged(const ModelNode &modelNode,
                                         [[maybe_unused]] AuxiliaryDataKeyView key,
                                         [[maybe_unused]] const QVariant &data)
{
    m_currentModelInterface->notifyDataChanged(modelNode);

    if (key == lockedProperty) {
        // Also notify data changed on child nodes to redraw them
        const QList<ModelNode> childNodes = modelNode.allSubModelNodes();
        for (const auto &childNode : childNodes)
            m_currentModelInterface->notifyDataChanged(childNode);
    }
}

void NavigatorView::instanceErrorChanged(const QVector<ModelNode> &errorNodeList)
{
    for (const ModelNode &modelNode : errorNodeList) {
        m_currentModelInterface->notifyDataChanged(modelNode);
        propagateInstanceErrorToExplorer(modelNode);
    }
}

void NavigatorView::nodeOrderChanged(const NodeListProperty &listProperty)
{
    m_currentModelInterface->notifyModelNodesMoved(listProperty.directSubNodes());

    // make sure selection is in sync again
    QTimer::singleShot(0, this, &NavigatorView::updateItemSelection);
}

void NavigatorView::changeToComponent(const QModelIndex &index)
{
    if (index.isValid() && currentModel()->data(index, Qt::UserRole).isValid()) {
        const ModelNode doubleClickNode = modelNodeForIndex(index);
        if (doubleClickNode.metaInfo().isFileComponent())
            Core::EditorManager::openEditor(Utils::FilePath::fromString(
                                                ModelUtils::componentFilePath(doubleClickNode)),
                                            Utils::Id(),
                                            Core::EditorManager::DoNotMakeVisible);
    }
}

QModelIndex NavigatorView::indexForModelNode(const ModelNode &modelNode) const
{
    return m_currentModelInterface->indexForModelNode(modelNode);
}

QAbstractItemModel *NavigatorView::currentModel() const
{
    return treeWidget()->model();
}

const ProjectExplorer::FileNode *NavigatorView::fileNodeForModelNode(const ModelNode &node) const
{
    QString filename = ModelUtils::componentFilePath(node);
    Utils::FilePath filePath = Utils::FilePath::fromString(filename);
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::projectForFile(
        filePath);

    if (!currentProject) {
        filePath = Utils::FilePath::fromString(node.model()->fileUrl().toLocalFile());

        /* If the component does not belong to the project then we can fallback to the current file */
        currentProject = ProjectExplorer::ProjectManager::projectForFile(filePath);
    }
    if (!currentProject)
        return nullptr;

    const auto fileNode = currentProject->nodeForFilePath(filePath);
    QTC_ASSERT(fileNode, return nullptr);

    return fileNode->asFileNode();
}

const ProjectExplorer::FileNode *NavigatorView::fileNodeForIndex(const QModelIndex &index) const
{
    if (index.isValid() && currentModel()->data(index, Qt::UserRole).isValid()) {
        ModelNode node = modelNodeForIndex(index);
        if (node.metaInfo().isFileComponent()) {
            return fileNodeForModelNode(node);
        }
    }

    return nullptr;
}

void NavigatorView::propagateInstanceErrorToExplorer(const ModelNode &modelNode) {
    QModelIndex index = indexForModelNode(modelNode);;

    do {
        const ProjectExplorer::FileNode *fnode = fileNodeForIndex(index);
        if (fnode) {
            fnode->setHasError(true);
            return;
        }
        else {
            index = index.parent();
        }
    } while (index.isValid());
}

void NavigatorView::filterToggled(bool flag)
{
    m_currentModelInterface->setFilter(flag);
    treeWidget()->expandAll();
    QmlDesignerPlugin::settings().insert(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS, flag);
}

void NavigatorView::reverseOrderToggled(bool flag)
{
    m_currentModelInterface->setOrder(flag);
    treeWidget()->expandAll();
    QmlDesignerPlugin::settings().insert(DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER, flag);
}

void NavigatorView::textFilterChanged(const QString &text)
{
    m_treeModel->setNameFilter(text);
    treeWidget()->expandAll();
}

void NavigatorView::changeSelection(const QItemSelection & /*newSelection*/, const QItemSelection &/*deselected*/)
{
    if (m_blockSelectionChangedSignal)
        return;

    QSet<ModelNode> nodeSet;

    for (const QModelIndex &index : treeWidget()->selectionModel()->selectedIndexes()) {
        const ModelNode modelNode = modelNodeForIndex(index);
        if (modelNode.isValid())
                nodeSet.insert(modelNode.isReference() ? modelNode.refNode() : modelNode);
    }

    bool blocked = blockSelectionChangedSignal(true);
    setSelectedModelNodes(Utils::toList(nodeSet));

    if(selectedModelNodes().size() == 1){
        auto node = selectedModelNodes().constFirst();
        if (node.isValid() && node.metaInfo().isQtQuick3DMaterial()){
            Utils3D::selectMaterial(node);
        }
    }

    blockSelectionChangedSignal(blocked);
}

void NavigatorView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{
    // Update selection asynchronously to ensure NavigatorTreeModel's index cache is up to date
    QTimer::singleShot(0, this, &NavigatorView::updateItemSelection);
}

void NavigatorView::updateItemSelection()
{
    if (!isAttached())
        return;

    QItemSelection itemSelection;
    for (const ModelNode &node : selectedModelNodes()) {
        const QModelIndex index = indexForModelNode(node);

        if (index.isValid()) {
            const QModelIndex beginIndex(currentModel()->index(index.row(), 0, index.parent()));
            const QModelIndex endIndex(currentModel()->index(index.row(), currentModel()->columnCount(index.parent()) - 1, index.parent()));
            if (beginIndex.isValid() && endIndex.isValid())
                itemSelection.select(beginIndex, endIndex);
        } else {
            // if the node index is invalid expand ancestors manually if they are valid.
            ModelNode parentNode = node.parentProperty().parentModelNode();
            while (parentNode) {
                QModelIndex parentIndex = indexForModelNode(parentNode);
                if (parentIndex.isValid())
                    treeWidget()->expand(parentIndex);
                else
                    break;
                parentNode = parentNode.parentProperty().parentModelNode();
            }
         }
    }

    bool blocked = blockSelectionChangedSignal(true);
    treeWidget()->selectionModel()->select(itemSelection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    blockSelectionChangedSignal(blocked);

    if (!selectedModelNodes().isEmpty())
        treeWidget()->scrollTo(indexForModelNode(selectedModelNodes().constLast()));

    // make sure selected nodes are visible
    for (const QModelIndex &selectedIndex : itemSelection.indexes()) {
        if (selectedIndex.column() == 0)
            expandAncestors(selectedIndex);
    }
}

QTreeView *NavigatorView::treeWidget() const
{
    if (m_widget)
        return m_widget->treeView();
    return nullptr;
}

NavigatorTreeModel *NavigatorView::treeModel()
{
    return m_treeModel.data();
}

// along the lines of QObject::blockSignals
bool NavigatorView::blockSelectionChangedSignal(bool block)
{
    bool oldValue = m_blockSelectionChangedSignal;
    m_blockSelectionChangedSignal = block;
    return oldValue;
}

void NavigatorView::expandAncestors(const QModelIndex &index)
{
    QModelIndex currentIndex = index.parent();
    while (currentIndex.isValid()) {
        if (!treeWidget()->isExpanded(currentIndex))
            treeWidget()->expand(currentIndex);
        currentIndex = currentIndex.parent();
    }
}

void NavigatorView::reparentAndCatch(NodeAbstractProperty property, const ModelNode &modelNode)
{
    try {
        property.reparentHere(modelNode);
    } catch (Exception &exception) {
        exception.showException();
    }
}

inline static QIcon contextIcon(const DesignerIcons::IconId &iconId)
{
    return DesignerActionManager::instance().contextIcon(iconId);
};

void NavigatorView::setupWidget()
{
    m_widget = new NavigatorWidget(this);
    m_treeModel = new NavigatorTreeModel(this);

    auto navigatorContext = new Internal::NavigatorContext(m_widget.data());
    Core::ICore::addContextObject(navigatorContext);

    m_treeModel->setView(this);
    m_widget->setTreeModel(m_treeModel.data());
    m_currentModelInterface = m_treeModel;

    connect(treeWidget()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NavigatorView::changeSelection);

    connect(m_widget.data(), &NavigatorWidget::filterToggled, this, &NavigatorView::filterToggled);
    connect(m_widget.data(), &NavigatorWidget::reverseOrderToggled, this, &NavigatorView::reverseOrderToggled);

    connect(m_widget.data(), &NavigatorWidget::textFilterChanged, this, &NavigatorView::textFilterChanged);

    const QString fontName = "qtds_propertyIconFont.ttf";
    const QSize size = QSize(32, 32);

    const QString visibilityOnUnicode = Theme::getIconUnicode(Theme::Icon::visibilityOn);
    const QString visibilityOffUnicode = Theme::getIconUnicode(Theme::Icon::visibilityOff);

    const QString aliasOnUnicode = Theme::getIconUnicode(Theme::Icon::alias);
    const QString aliasOffUnicode = aliasOnUnicode;

    const QString lockOnUnicode = Theme::getIconUnicode(Theme::Icon::lockOn);
    const QString lockOffUnicode = Theme::getIconUnicode(Theme::Icon::lockOff);

    const QString materialOnUnicode = Theme::getIconUnicode(Theme::Icon::material_medium);
    const QString materialOffUnicode = materialOnUnicode;

    const QString deleteOnUnicode = Theme::getIconUnicode(Theme::Icon::deleteMaterial);
    const QString deleteOffUnicode = deleteOnUnicode;

    auto visibilityIconOffNormal = Utils::StyleHelper::IconFontHelper(
        visibilityOffUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::Off);
    auto visibilityIconOffHover = Utils::StyleHelper::IconFontHelper(
        visibilityOffUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::Off);
    auto visibilityIconOffSelected = Utils::StyleHelper::IconFontHelper(
        visibilityOffUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::Off);
    auto visibilityIconOnNormal = Utils::StyleHelper::IconFontHelper(
        visibilityOnUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::On);
    auto visibilityIconOnHover = Utils::StyleHelper::IconFontHelper(
        visibilityOnUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::On);
    auto visibilityIconOnSelected = Utils::StyleHelper::IconFontHelper(
        visibilityOnUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::On);

    const QIcon visibilityIcon = Utils::StyleHelper::getIconFromIconFont(
                fontName, {visibilityIconOffNormal,
                           visibilityIconOffHover,
                           visibilityIconOffSelected,
                           visibilityIconOnNormal,
                           visibilityIconOnHover,
                           visibilityIconOnSelected});

    auto aliasIconOffNormal = Utils::StyleHelper::IconFontHelper(
        aliasOffUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::Off);
    auto aliasIconOffHover = Utils::StyleHelper::IconFontHelper(
        aliasOffUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::Off);
    auto aliasIconOffSelected = Utils::StyleHelper::IconFontHelper(
        aliasOffUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::Off);
    auto aliasIconOnNormal = Utils::StyleHelper::IconFontHelper(
        aliasOnUnicode, Theme::getColor(Theme::DSnavigatorAliasIconChecked), size, QIcon::Normal, QIcon::On);
    auto aliasIconOnHover = Utils::StyleHelper::IconFontHelper(
        aliasOnUnicode, Theme::getColor(Theme::DSnavigatorAliasIconChecked), size, QIcon::Active, QIcon::On);
    auto aliasIconOnSelected = Utils::StyleHelper::IconFontHelper(
        aliasOnUnicode, Theme::getColor(Theme::DSnavigatorAliasIconChecked), size, QIcon::Selected, QIcon::On);

    const QIcon aliasIcon = Utils::StyleHelper::getIconFromIconFont(
                fontName, {aliasIconOffNormal,
                           aliasIconOffHover,
                           aliasIconOffSelected,
                           aliasIconOnNormal,
                           aliasIconOnHover,
                           aliasIconOnSelected});

    auto lockIconOffNormal = Utils::StyleHelper::IconFontHelper(
        lockOffUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::Off);
    auto lockIconOffHover = Utils::StyleHelper::IconFontHelper(
        lockOffUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::Off);
    auto lockIconOffSelected = Utils::StyleHelper::IconFontHelper(
        lockOffUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::Off);
    auto lockIconOnNormal = Utils::StyleHelper::IconFontHelper(
        lockOnUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::On);
    auto lockIconOnHover = Utils::StyleHelper::IconFontHelper(
        lockOnUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::On);
    auto lockIconOnSelected = Utils::StyleHelper::IconFontHelper(
        lockOnUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::On);

    const QIcon lockIcon = Utils::StyleHelper::getIconFromIconFont(
                fontName, {lockIconOffNormal,
                           lockIconOffHover,
                           lockIconOffSelected,
                           lockIconOnNormal,
                           lockIconOnHover,
                           lockIconOnSelected});

    auto materialIconOffNormal = Utils::StyleHelper::IconFontHelper(
        materialOffUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::Off);
    auto materialIconOffHover = Utils::StyleHelper::IconFontHelper(
        materialOffUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::Off);
    auto materialIconOffSelected = Utils::StyleHelper::IconFontHelper(
        materialOffUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::Off);
    auto materialIconOnNormal = Utils::StyleHelper::IconFontHelper(
        materialOnUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::On);
    auto materialIconOnHover = Utils::StyleHelper::IconFontHelper(
        materialOnUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::On);
    auto materialIconOnSelected = Utils::StyleHelper::IconFontHelper(
        materialOnUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::On);

    const QIcon materialIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName, {materialIconOffNormal,
         materialIconOffHover,
         materialIconOffSelected,
         materialIconOnNormal,
         materialIconOnHover,
         materialIconOnSelected});

    auto deleteIconOffNormal = Utils::StyleHelper::IconFontHelper(
        deleteOffUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::Off);
    auto deleteIconOffHover = Utils::StyleHelper::IconFontHelper(
        deleteOffUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::Off);
    auto deleteIconOffSelected = Utils::StyleHelper::IconFontHelper(
        deleteOffUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::Off);
    auto deleteIconOnNormal = Utils::StyleHelper::IconFontHelper(
        deleteOnUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::On);
    auto deleteIconOnHover = Utils::StyleHelper::IconFontHelper(
        deleteOnUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::On);
    auto deleteIconOnSelected = Utils::StyleHelper::IconFontHelper(
        deleteOnUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::On);

    const QIcon deleteIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName, {deleteIconOffNormal,
         deleteIconOffHover,
         deleteIconOffSelected,
         deleteIconOnNormal,
         deleteIconOnHover,
         deleteIconOnSelected});

    auto idDelegate = new NameItemDelegate(this);

    auto visibilityDelegate = new IconCheckboxItemDelegate(this, visibilityIcon);
    auto editDelegate = new IconCheckboxItemDelegate(this, materialIcon);
    auto deleteDelegate = new IconCheckboxItemDelegate(this, deleteIcon);
    auto aliasDelegate = new IconCheckboxItemDelegate(this, aliasIcon);
    auto lockDelegate = new IconCheckboxItemDelegate(this, lockIcon);

    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Name, idDelegate);
    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Edit, editDelegate);
    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Delete, deleteDelegate);
    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Alias, aliasDelegate);
    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Visibility, visibilityDelegate);
    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Lock, lockDelegate);
}

} // namespace QmlDesigner
