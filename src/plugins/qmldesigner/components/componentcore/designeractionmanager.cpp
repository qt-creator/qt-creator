/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "designeractionmanager.h"

#include "changestyleaction.h"
#include "designeractionmanagerview.h"
#include "modelnodecontextmenu_helper.h"
#include "qmldesignerconstants.h"
#include "rewritingexception.h"
#include <bindingproperty.h>
#include <nodehints.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>

#include <formeditortoolbutton.h>

#include <documentmanager.h>
#include <qmldesignerplugin.h>
#include <viewmanager.h>

#include <listmodeleditor/listmodeleditordialog.h>
#include <listmodeleditor/listmodeleditormodel.h>


#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QGraphicsLinearLayout>
#include <QHBoxLayout>
#include <QMessageBox>

#include <exception>

namespace QmlDesigner {

static inline QString captionForModelNode(const ModelNode &modelNode)
{
    if (modelNode.id().isEmpty())
        return modelNode.simplifiedTypeName();

    return modelNode.id();
}

static inline bool contains(const QmlItemNode &node, const QPointF &position)
{
    return node.isValid() && node.instanceSceneTransform().mapRect(node.instanceBoundingRect()).contains(position);
}

DesignerActionManagerView *DesignerActionManager::view()
{
    return m_designerActionManagerView;
}

DesignerActionToolBar *DesignerActionManager::createToolBar(QWidget *parent) const
{
    auto toolBar = new DesignerActionToolBar(parent);

    QList<ActionInterface* > categories = Utils::filtered(designerActions(), [](ActionInterface *action) {
            return action->type() ==  ActionInterface::ContextMenu;
    });

    Utils::sort(categories, [](ActionInterface *l, ActionInterface *r) {
        return l->priority() > r->priority();
    });

    for (auto *categoryAction : qAsConst(categories)) {
        QList<ActionInterface* > actions = Utils::filtered(designerActions(), [categoryAction](ActionInterface *action) {
                return action->category() == categoryAction->menuId();
        });

        Utils::sort(actions, [](ActionInterface *l, ActionInterface *r) {
            return l->priority() > r->priority();
        });

        bool addSeparator = false;

        for (auto *action : qAsConst(actions)) {
            if ((action->type() == ActionInterface::Action || action->type() == ActionInterface::ToolBarAction)
                    && action->action()) {
                toolBar->registerAction(action);
                addSeparator = true;
            } else if (addSeparator && action->action()->isSeparator()) {
                toolBar->registerAction(action);
            }
        }
    }

    return toolBar;
}

void DesignerActionManager::polishActions() const
{
    QList<ActionInterface* > actions =  Utils::filtered(designerActions(),
                                                        [](ActionInterface *action) { return action->type() != ActionInterface::ContextMenu; });

    Core::Context qmlDesignerFormEditorContext(Constants::C_QMLFORMEDITOR);
    Core::Context qmlDesignerEditor3DContext(Constants::C_QMLEDITOR3D);
    Core::Context qmlDesignerNavigatorContext(Constants::C_QMLNAVIGATOR);

    Core::Context qmlDesignerUIContext;
    qmlDesignerUIContext.add(qmlDesignerFormEditorContext);
    qmlDesignerUIContext.add(qmlDesignerEditor3DContext);
    qmlDesignerUIContext.add(qmlDesignerNavigatorContext);

    for (auto *action : actions) {
        if (!action->menuId().isEmpty()) {
            const QString id = QString("QmlDesigner.%1").arg(QString::fromLatin1(action->menuId()));

            Core::Command *cmd = Core::ActionManager::registerAction(action->action(), id.toLatin1().constData(), qmlDesignerUIContext);

            cmd->setDefaultKeySequence(action->action()->shortcut());
            cmd->setDescription(action->action()->toolTip());

            action->action()->setToolTip(cmd->action()->toolTip());
            action->action()->setShortcut(cmd->action()->shortcut());
            action->action()->setShortcutContext(Qt::WidgetShortcut); //Hack to avoid conflicting shortcuts. We use the Core::Command for the shortcut.
        }
    }
}

QGraphicsWidget *DesignerActionManager::createFormEditorToolBar(QGraphicsItem *parent)
{
    QList<ActionInterface* > actions = Utils::filtered(designerActions(),
                                                       [](ActionInterface *action) {
            return action->type() ==  ActionInterface::FormEditorAction
                && action->action()->isVisible();
    });

    Utils::sort(actions, [](ActionInterface *l, ActionInterface *r) {
        return l->priority() > r->priority();
    });

    QGraphicsWidget *toolbar = new QGraphicsWidget(parent);

    auto layout = new QGraphicsLinearLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    toolbar->setLayout(layout);

    for (ActionInterface *action : qAsConst(actions)) {
        auto button = new FormEditorToolButton(action->action(), toolbar);
        layout->addItem(button);
    }

    toolbar->resize(toolbar->preferredSize());

    layout->invalidate();
    layout->activate();

    toolbar->update();

    return toolbar;
}

DesignerActionManager &DesignerActionManager::instance()
{
    return QmlDesignerPlugin::instance()->viewManager().designerActionManager();
}

void DesignerActionManager::setupContext()
{
    m_designerActionManagerView->setupContext();
}

QList<AddResourceHandler> DesignerActionManager::addResourceHandler() const
{
    return m_addResourceHandler;
}

void DesignerActionManager::registerAddResourceHandler(const AddResourceHandler &handler)
{
    m_addResourceHandler.append(handler);
}

void DesignerActionManager::unregisterAddResourceHandlers(const QString &category)
{
    for (int i = m_addResourceHandler.size() - 1; i >= 0 ; --i) {
        const AddResourceHandler &handler = m_addResourceHandler[i];
        if (handler.category == category)
            m_addResourceHandler.removeAt(i);
    }
}

void DesignerActionManager::registerModelNodePreviewHandler(const ModelNodePreviewImageHandler &handler)
{
    m_modelNodePreviewImageHandlers.append(handler);
}

bool DesignerActionManager::hasModelNodePreviewHandler(const ModelNode &node) const
{
    const bool isComponent = node.isComponent();
    for (const auto &handler : qAsConst(m_modelNodePreviewImageHandlers)) {
        if ((isComponent || !handler.componentOnly) && node.isSubclassOf(handler.type))
            return true;
    }
    return false;
}

ModelNodePreviewImageOperation DesignerActionManager::modelNodePreviewOperation(const ModelNode &node) const
{
    ModelNodePreviewImageOperation op = nullptr;
    int prio = -1;
    const bool isComponent = node.isComponent();
    for (const auto &handler : qAsConst(m_modelNodePreviewImageHandlers)) {
        if ((isComponent || !handler.componentOnly) && handler.priority > prio
                && node.isSubclassOf(handler.type)) {
            op = handler.operation;
            prio = handler.priority;
        }
    }
    return op;
}

class VisiblityModelNodeAction : public ModelNodeContextMenuAction
{
public:
    VisiblityModelNodeAction(const QByteArray &id, const QString &description, const QByteArray &category, const QKeySequence &key, int priority,
                             SelectionContextOperation action,
                             SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                             SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        ModelNodeContextMenuAction(id, description, {}, category, key, priority, action, enabled, visibility)
    {}

    void updateContext() override
    {
        defaultAction()->setSelectionContext(selectionContext());
        if (selectionContext().isValid()) {
            defaultAction()->setEnabled(isEnabled(selectionContext()));
            defaultAction()->setVisible(isVisible(selectionContext()));

            defaultAction()->setCheckable(true);
            QmlItemNode itemNode = QmlItemNode(selectionContext().currentSingleSelectedNode());
            if (itemNode.isValid())
                defaultAction()->setChecked(itemNode.instanceValue("visible").toBool());
            else
                defaultAction()->setEnabled(false);
        }
    }
};

class FillLayoutModelNodeAction : public ModelNodeContextMenuAction
{
public:
    FillLayoutModelNodeAction(const QByteArray &id, const QString &description, const QByteArray &category, const QKeySequence &key, int priority,
                              SelectionContextOperation action,
                              SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                              SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        ModelNodeContextMenuAction(id, description, {}, category, key, priority, action, enabled, visibility)
    {}
    void updateContext() override
    {
        defaultAction()->setSelectionContext(selectionContext());
        if (selectionContext().isValid()) {
            defaultAction()->setEnabled(isEnabled(selectionContext()));
            defaultAction()->setVisible(isVisible(selectionContext()));

            defaultAction()->setCheckable(true);
            QmlItemNode itemNode = QmlItemNode(selectionContext().currentSingleSelectedNode());
            if (itemNode.isValid()) {
                bool flag = false;
                if (itemNode.modelNode().hasProperty(m_propertyName)
                        || itemNode.propertyAffectedByCurrentState(m_propertyName)) {
                    flag = itemNode.modelValue(m_propertyName).toBool();
                }
                defaultAction()->setChecked(flag);
            } else {
                defaultAction()->setEnabled(false);
            }
        }
    }
protected:
    PropertyName m_propertyName;
};

class FillWidthModelNodeAction : public FillLayoutModelNodeAction
{
public:
    FillWidthModelNodeAction(const QByteArray &id, const QString &description, const QByteArray &category, const QKeySequence &key, int priority,
                             SelectionContextOperation action,
                             SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                             SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        FillLayoutModelNodeAction(id, description, category, key, priority, action, enabled, visibility)
    {
        m_propertyName = "Layout.fillWidth";
    }
};

class FillHeightModelNodeAction : public FillLayoutModelNodeAction
{
public:
    FillHeightModelNodeAction(const QByteArray &id, const QString &description, const QByteArray &category, const QKeySequence &key, int priority,
                              SelectionContextOperation action,
                              SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                              SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        FillLayoutModelNodeAction(id, description, category, key, priority, action, enabled, visibility)
    {
        m_propertyName = "Layout.fillHeight";
    }
};

class SelectionModelNodeAction : public ActionGroup
{
public:
    SelectionModelNodeAction(const QString &displayName, const QByteArray &menuId, int priority) :
        ActionGroup(displayName, menuId, priority,
                           &SelectionContextFunctors::always, &SelectionContextFunctors::selectionEnabled)

    {}

    void updateContext() override
    {
        menu()->clear();
        if (selectionContext().isValid()) {
            action()->setEnabled(isEnabled(selectionContext()));
            action()->setVisible(isVisible(selectionContext()));
        } else {
            return;
        }
        if (action()->isEnabled()) {
            ModelNode parentNode;
            if (selectionContext().singleNodeIsSelected()
                    && !selectionContext().currentSingleSelectedNode().isRootNode()
                    && selectionContext().currentSingleSelectedNode().hasParentProperty()) {

                parentNode = selectionContext().currentSingleSelectedNode().parentProperty().parentModelNode();

                if (!ModelNode::isThisOrAncestorLocked(parentNode)) {
                    ActionTemplate *selectionAction = new ActionTemplate("SELECTION", {}, &ModelNodeOperations::select);
                    selectionAction->setParent(menu());
                    selectionAction->setText(QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select parent: %1")).arg(
                                                 captionForModelNode(parentNode)));

                    SelectionContext nodeSelectionContext = selectionContext();
                    nodeSelectionContext.setTargetNode(parentNode);
                    selectionAction->setSelectionContext(nodeSelectionContext);

                    menu()->addAction(selectionAction);
                }
            }
            for (const ModelNode &node : selectionContext().view()->allModelNodes()) {
                if (node != selectionContext().currentSingleSelectedNode()
                        && node != parentNode
                        && contains(node, selectionContext().scenePosition())
                        && !node.isRootNode()
                        && !ModelNode::isThisOrAncestorLocked(node)) {
                    selectionContext().setTargetNode(node);
                    QString what = QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select: %1")).arg(captionForModelNode(node));
                    ActionTemplate *selectionAction = new ActionTemplate("SELECT", what, &ModelNodeOperations::select);

                    SelectionContext nodeSelectionContext = selectionContext();
                    nodeSelectionContext.setTargetNode(node);
                    selectionAction->setSelectionContext(nodeSelectionContext);

                    menu()->addAction(selectionAction);
                }
            }

            if (menu()->isEmpty())
                action()->setEnabled(false);
        }
    }
};

class DocumentError : public std::exception
{
public:
    const char *what() const noexcept override { return "Current document contains errors."; }
};

class EditListModelAction final : public ModelNodeContextMenuAction
{
public:
    EditListModelAction()
        : ModelNodeContextMenuAction("EditListModel",
                                     ComponentCoreConstants::editListModelDisplayName,
                                     {},
                                     ComponentCoreConstants::rootCategory,
                                     QKeySequence("Alt+e"),
                                     1001,
                                     &openDialog,
                                     &isListViewInBaseState,
                                     &isListViewInBaseState)
    {}

    static bool isListViewInBaseState(const SelectionContext &selectionState)
    {
        return selectionState.isInBaseState() && selectionState.singleNodeIsSelected()
               && (selectionState.currentSingleSelectedNode().metaInfo().isSubclassOf(
                       "QtQuick.ListView")
                   || selectionState.currentSingleSelectedNode().metaInfo().isSubclassOf(
                       "QtQuick.GridView"));
    }

    bool isEnabled(const SelectionContext &) const override { return true; }

    static void openDialog(const SelectionContext &selectionState)
    {
        ModelNode targetNode = selectionState.targetNode();
        if (!targetNode.isValid())
            targetNode = selectionState.currentSingleSelectedNode();
        if (!targetNode.isValid())
            return;

        AbstractView *view = targetNode.view();
        NodeMetaInfo modelMetaInfo = view->model()->metaInfo("ListModel");
        NodeMetaInfo elementMetaInfo = view->model()->metaInfo("ListElement");

        ListModelEditorModel model{[&] {
                                       return view->createModelNode(modelMetaInfo.typeName(),
                                                                    modelMetaInfo.majorVersion(),
                                                                    modelMetaInfo.minorVersion());
                                   },
                                   [&] {
                                       return view->createModelNode(elementMetaInfo.typeName(),
                                                                    elementMetaInfo.majorVersion(),
                                                                    elementMetaInfo.minorVersion());
                                   },
                                   [&](const ModelNode &node) {
                                       bool isNowInComponent = ModelNodeOperations::goIntoComponent(
                                           node);

                                       Model *currentModel = QmlDesignerPlugin::instance()
                                                                 ->currentDesignDocument()
                                                                 ->currentModel();

                                       if (currentModel->rewriterView()
                                           && currentModel->rewriterView()->inErrorState()) {
                                           throw DocumentError{};
                                       }

                                       if (isNowInComponent)
                                           return view->rootModelNode();

                                       return node;
                                   }};

        model.setListView(targetNode);

        ListModelEditorDialog dialog{Core::ICore::mainWindow()};
        dialog.setModel(&model);

        try {
            dialog.exec();
        } catch (const DocumentError &) {
            QMessageBox::warning(
                Core::ICore::mainWindow(),
                QCoreApplication::translate("DesignerActionManager", "Document Has Errors"),
                QCoreApplication::translate("DesignerActionManager",
                                            "The document which contains the list model "
                                            "contains errors. So we cannot edit it."));
        } catch (const RewritingException &) {
            QMessageBox::warning(
                Core::ICore::mainWindow(),
                QCoreApplication::translate("DesignerActionManager", "Document Cannot Be Written"),
                QCoreApplication::translate("DesignerActionManager",
                                            "An error occurred during a write attemp."));
        }
    }
};

bool flowOptionVisible(const SelectionContext &context)
{
    return QmlFlowViewNode::isValidQmlFlowViewNode(context.rootNode());
}

bool isFlowItem(const SelectionContext &context)
{
    return context.singleNodeIsSelected()
           && QmlFlowItemNode::isValidQmlFlowItemNode(context.currentSingleSelectedNode());
}

bool isFlowTarget(const SelectionContext &context)
{
    return context.singleNodeIsSelected()
           && QmlFlowTargetNode::isFlowEditorTarget(context.currentSingleSelectedNode());
}

bool isFlowTransitionItem(const SelectionContext &context)
{
    return context.singleNodeIsSelected()
           && QmlFlowItemNode::isFlowTransition(context.currentSingleSelectedNode());
}

bool isFlowTransitionItemWithEffect(const SelectionContext &context)
{
    if (!isFlowTransitionItem(context))
        return false;

    ModelNode node = context.currentSingleSelectedNode();

    return node.hasNodeProperty("effect");
}

bool isFlowActionItemItem(const SelectionContext &context)
{
    const ModelNode selectedNode = context.currentSingleSelectedNode();

    return context.singleNodeIsSelected()
            && (QmlFlowActionAreaNode::isValidQmlFlowActionAreaNode(selectedNode)
                || QmlVisualNode::isFlowDecision(selectedNode)
                || QmlVisualNode::isFlowWildcard(selectedNode));
}

bool isFlowTargetOrTransition(const SelectionContext &context)
{
    return isFlowTarget(context) || isFlowTransitionItem(context);
}

class FlowActionConnectAction : public ActionGroup
{
public:
    FlowActionConnectAction(const QString &displayName, const QByteArray &menuId, int priority) :
        ActionGroup(displayName, menuId, priority,
                    &isFlowActionItemItem, &flowOptionVisible)

    {}

    void updateContext() override
    {
        menu()->clear();
        if (selectionContext().isValid()) {
            action()->setEnabled(isEnabled(selectionContext()));
            action()->setVisible(isVisible(selectionContext()));
        } else {
            return;
        }
        if (action()->isEnabled()) {
            for (const QmlFlowItemNode &node : QmlFlowViewNode(selectionContext().rootNode()).flowItems()) {
                if (node != selectionContext().currentSingleSelectedNode().parentProperty().parentModelNode()) {
                    QString what = QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Connect: %1")).arg(captionForModelNode(node));
                    ActionTemplate *connectionAction = new ActionTemplate("CONNECT", what, &ModelNodeOperations::addTransition);

                    SelectionContext nodeSelectionContext = selectionContext();
                    nodeSelectionContext.setTargetNode(node);
                    connectionAction->setSelectionContext(nodeSelectionContext);

                    menu()->addAction(connectionAction);
                }
            }
        }
    }
};

const char xProperty[] = "x";
const char yProperty[] = "y";
const char zProperty[] = "z";
const char widthProperty[] = "width";
const char heightProperty[] = "height";
const char triggerSlot[] = "trigger";

using namespace SelectionContextFunctors;

bool multiSelection(const SelectionContext &context)
{
    return !singleSelection(context) && selectionNotEmpty(context);
}

bool multiSelectionAndInBaseState(const SelectionContext &context)
{
    return multiSelection(context) && inBaseState(context);
}

bool selectionHasProperty1or2(const SelectionContext &context, const char *x, const char *y)
{
    return selectionHasProperty(context, x) || selectionHasProperty(context, y);
}

bool selectionHasSameParentAndInBaseState(const SelectionContext &context)
{
    return selectionHasSameParent(context) && inBaseState(context);
}

bool multiSelectionAndHasSameParent(const SelectionContext &context)
{
    return multiSelection(context) && selectionHasSameParent(context);
}

bool isNotInLayout(const SelectionContext &context)
{
    if (selectionNotEmpty(context)) {
        const ModelNode selectedModelNode = context.selectedModelNodes().constFirst();
        ModelNode parentModelNode;

        if (selectedModelNode.hasParentProperty())
            parentModelNode = selectedModelNode.parentProperty().parentModelNode();

        if (parentModelNode.isValid() && parentModelNode.metaInfo().isValid())
            return !parentModelNode.metaInfo().isLayoutable();
    }

    return true;
}

bool selectionCanBeLayouted(const SelectionContext &context)
{
    return  multiSelection(context)
            && selectionHasSameParentAndInBaseState(context)
            && inBaseState(context)
            && isNotInLayout(context);
}

bool selectionCanBeLayoutedAndQtQuickLayoutPossible(const SelectionContext &context)
{
    return selectionCanBeLayouted(context) && context.view()->majorQtQuickVersion() > 1;
}

bool selectionNotEmptyAndHasZProperty(const SelectionContext &context)
{
    return selectionNotEmpty(context) && selectionHasProperty(context, zProperty);
}

bool selectionNotEmptyAndHasWidthOrHeightProperty(const SelectionContext &context)
{
    return selectionNotEmpty(context)
        && selectionHasProperty1or2(context, widthProperty, heightProperty);
}

bool singleSelectionItemIsNotAnchoredAndSingleSelectionNotRoot(const SelectionContext &context)
{
    return singleSelectionItemIsNotAnchored(context)
            && singleSelectionNotRoot(context);
}

bool selectionNotEmptyAndHasXorYProperty(const SelectionContext &context)
{
    return selectionNotEmpty(context)
        && selectionHasProperty1or2(context, xProperty, yProperty);
}

bool singleSelectionAndHasSlotTrigger(const SelectionContext &context)
{
    if (!singleSelection(context))
        return false;

    return selectionHasSlot(context, triggerSlot);
}

bool singleSelectionAndInQtQuickLayout(const SelectionContext &context)
{
    if (!singleSelection(context))
            return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();
    if (!currentSelectedNode.isValid())
        return false;

    if (!currentSelectedNode.hasParentProperty())
        return false;

    ModelNode parentModelNode = currentSelectedNode.parentProperty().parentModelNode();

    NodeMetaInfo metaInfo = parentModelNode.metaInfo();

    if (!metaInfo.isValid())
        return false;

    return metaInfo.isSubclassOf("QtQuick.Layouts.Layout");
}

bool isStackedContainer(const SelectionContext &context)
{
    if (!singleSelection(context))
            return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    return NodeHints::fromModelNode(currentSelectedNode).isStackedContainer();
}

bool isStackedContainerWithoutTabBar(const SelectionContext &context)
{
    if (!isStackedContainer(context))
        return false;

    if (!context.view()->model())
        return false;

    if (!context.view()->model()->metaInfo("QtQuick.Controls.TabBar", -1, -1).isValid())
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    const PropertyName propertyName = ModelNodeOperations::getIndexPropertyName(currentSelectedNode);

    QTC_ASSERT(currentSelectedNode.metaInfo().hasProperty(propertyName), return false);

    BindingProperty binding = currentSelectedNode.bindingProperty(propertyName);

    /* There is already a TabBar or something similar attached */
    return !(binding.isValid() && binding.resolveToProperty().isValid());
}

bool isStackedContainerAndIndexCanBeDecreased(const SelectionContext &context)
{
    if (!isStackedContainer(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    const PropertyName propertyName = ModelNodeOperations::getIndexPropertyName(currentSelectedNode);

    QTC_ASSERT(currentSelectedNode.metaInfo().hasProperty(propertyName), return false);

    QmlItemNode containerItemNode(currentSelectedNode);
    QTC_ASSERT(containerItemNode.isValid(), return false);

    const int value = containerItemNode.instanceValue(propertyName).toInt();

    return value > 0;
}

bool isStackedContainerAndIndexCanBeIncreased(const SelectionContext &context)
{
    if (!isStackedContainer(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    const PropertyName propertyName = ModelNodeOperations::getIndexPropertyName(currentSelectedNode);

    QTC_ASSERT(currentSelectedNode.metaInfo().hasProperty(propertyName), return false);

    QmlItemNode containerItemNode(currentSelectedNode);
    QTC_ASSERT(containerItemNode.isValid(), return false);

    const int value = containerItemNode.instanceValue(propertyName).toInt();

    const int maxValue = currentSelectedNode.directSubModelNodes().count() - 1;

    return value < maxValue;
}

bool isGroup(const SelectionContext &context)
{
    if (!inBaseState(context))
        return false;

    if (!singleSelection(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    if (!currentSelectedNode.isValid())
        return false;

    NodeMetaInfo metaInfo = currentSelectedNode.metaInfo();

    if (!metaInfo.isValid())
        return false;

    return metaInfo.isSubclassOf("QtQuick.Studio.Components.GroupItem");
}

bool isLayout(const SelectionContext &context)
{
    if (!inBaseState(context))
        return false;

    if (!singleSelection(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    if (!currentSelectedNode.isValid())
        return false;

    NodeMetaInfo metaInfo = currentSelectedNode.metaInfo();

    if (!metaInfo.isValid())
        return false;

    /* Stacked containers have different semantics */
    if (isStackedContainer(context))
            return false;

    return metaInfo.isSubclassOf("QtQuick.Layouts.Layout");
}

bool isPositioner(const SelectionContext &context)
{
     if (!inBaseState(context))
         return false;

    if (!singleSelection(context))
        return false;

    ModelNode currentSelectedNode = context.currentSingleSelectedNode();

    if (!currentSelectedNode.isValid())
        return false;

    NodeMetaInfo metaInfo = currentSelectedNode.metaInfo();

    if (!metaInfo.isValid())
        return false;

    return metaInfo.isSubclassOf("<cpp>.QDeclarativeBasePositioner")
            || metaInfo.isSubclassOf("QtQuick.Positioner");
}

bool layoutOptionVisible(const SelectionContext &context)
{
    return multiSelectionAndInBaseState(context)
            || singleSelectionAndInQtQuickLayout(context)
            || isLayout(context);
}

bool positionOptionVisible(const SelectionContext &context)
{
    return multiSelectionAndInBaseState(context)
            || isPositioner(context);
}

bool studioComponentsAvailable(const SelectionContext &context)
{
    const Import import = Import::createLibraryImport("QtQuick.Studio.Components", "1.0");
    return context.view()->model()->isImportPossible(import, true, true);
}

bool studioComponentsAvailableAndSelectionCanBeLayouted(const SelectionContext &context)
{
    return selectionCanBeLayouted(context) && studioComponentsAvailable(context);
}

bool singleSelectedAndUiFile(const SelectionContext &context)
{
    if (!singleSelection(context))
            return false;

    DesignDocument *designDocument = QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();

    if (!designDocument)
        return false;

    return designDocument->fileName().toFileInfo().completeSuffix()
            == QLatin1String("ui.qml");
}

bool lowerAvailable(const SelectionContext &selectionState)
{
    if (!singleSelection(selectionState))
        return false;

    ModelNode modelNode = selectionState.currentSingleSelectedNode();

    if (modelNode.isRootNode())
        return false;

    if (!modelNode.hasParentProperty())
        return false;

    if (!modelNode.parentProperty().isNodeListProperty())
        return false;

    NodeListProperty parentProperty = modelNode.parentProperty().toNodeListProperty();
    return parentProperty.indexOf(modelNode) > 0;
}

bool raiseAvailable(const SelectionContext &selectionState)
{
    if (!singleSelection(selectionState))
        return false;

    ModelNode modelNode = selectionState.currentSingleSelectedNode();

    if (modelNode.isRootNode())
        return false;

    if (!modelNode.hasParentProperty())
        return false;

    if (!modelNode.parentProperty().isNodeListProperty())
        return false;

    NodeListProperty parentProperty = modelNode.parentProperty().toNodeListProperty();
    return parentProperty.indexOf(modelNode) < parentProperty.count() - 1;
}

bool anchorsMenuEnabled(const SelectionContext &context)
{
    return singleSelectionItemIsNotAnchoredAndSingleSelectionNotRoot(context)
           || singleSelectionItemIsAnchored(context);
}

void DesignerActionManager::createDefaultDesignerActions()
{
    using namespace SelectionContextFunctors;
    using namespace ComponentCoreConstants;
    using namespace ModelNodeOperations;

    const Utils::Icon prevIcon({
        {QLatin1String(":/utils/images/prev.png"), Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    const Utils::Icon nextIcon({
        {QLatin1String(":/utils/images/next.png"), Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    const Utils::Icon addIcon({
        {QLatin1String(":/utils/images/plus.png"), Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    addDesignerAction(new SelectionModelNodeAction(
                          selectionCategoryDisplayName,
                          selectionCategory,
                          prioritySelectionCategory));

    addDesignerAction(new ActionGroup(
                          arrangeCategoryDisplayName,
                          arrangeCategory,
                          priorityStackCategory,
                          &selectionNotEmpty));

    addDesignerAction(new ModelNodeContextMenuAction(
                          toFrontCommandId,
                          toFrontDisplayName,
                          {},
                          arrangeCategory,
                          QKeySequence(),
                          200,
                          &toFront,
                          &raiseAvailable));

    addDesignerAction(new ModelNodeContextMenuAction(
                          raiseCommandId,
                          raiseDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/raise.png", Utils::Theme::IconsBaseColor}}).icon(),
                          arrangeCategory,
                          QKeySequence(),
                          180,
                          &raise,
                          &raiseAvailable));

    addDesignerAction(new ModelNodeContextMenuAction(
                          lowerCommandId,
                          lowerDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/lower.png", Utils::Theme::IconsBaseColor}}).icon(),
                          arrangeCategory,
                          QKeySequence(),
                          160,
                          &lower,
                          &lowerAvailable));

    addDesignerAction(new ModelNodeContextMenuAction(
                          toBackCommandId,
                          toBackDisplayName,
                          {},
                          arrangeCategory,
                          QKeySequence(),
                          140,
                          &toBack,
                          &lowerAvailable));

    addDesignerAction(new ModelNodeContextMenuAction(
                          reverseCommandId,
                          reverseDisplayName,
                          {},
                          arrangeCategory,
                          QKeySequence(),
                          100,
                          &reverse,
                          &multiSelectionAndHasSameParent));

    addDesignerAction(new ActionGroup(editCategoryDisplayName, editCategory, priorityEditCategory, &selectionNotEmpty));

    addDesignerAction(new SeperatorDesignerAction(editCategory, 220));

    addDesignerAction(new ModelNodeAction(
                          resetPositionCommandId,
                          resetPositionDisplayName,
                          Utils::Icon({{":/utils/images/pan.png", Utils::Theme::IconsBaseColor},
                                      {":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}}).icon(),
                          resetPositionTooltip,
                          editCategory,
                          QKeySequence("Ctrl+d"),
                          200,
                          &resetPosition,
                          &selectionNotEmptyAndHasXorYProperty));

    addDesignerAction(new ModelNodeAction(
                          resetSizeCommandId,
                          resetSizeDisplayName,
                          Utils::Icon({{":/utils/images/fittoview.png", Utils::Theme::IconsBaseColor},
                                      {":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}}).icon(),
                          resetSizeToolTip,
                          editCategory,
                          QKeySequence("shift+s"),
                          180,
                          &resetSize,
                          &selectionNotEmptyAndHasWidthOrHeightProperty));

    addDesignerAction(new SeperatorDesignerAction(editCategory, 170));

    addDesignerAction(new VisiblityModelNodeAction(
                          visiblityCommandId,
                          visibilityDisplayName,
                          editCategory,
                          QKeySequence("Ctrl+g"),
                          160,
                          &setVisible,
                          &singleSelectedItem));

    addDesignerAction(new ActionGroup(anchorsCategoryDisplayName,
                                      anchorsCategory,
                                      priorityAnchorsCategory,
                                      &anchorsMenuEnabled));

    addDesignerAction(new ModelNodeAction(
                          anchorsFillCommandId,
                          anchorsFillDisplayName,
                          Utils::Icon({{":/qmldesigner/images/anchor_fill.png", Utils::Theme::IconsBaseColor}}).icon(),
                          anchorsFillToolTip,
                          anchorsCategory,
                          QKeySequence(QKeySequence("shift+f")),
                          200,
                          &anchorsFill,
                          &singleSelectionItemIsNotAnchoredAndSingleSelectionNotRoot));

    addDesignerAction(new ModelNodeAction(
                          anchorsResetCommandId,
                          anchorsResetDisplayName,
                          Utils::Icon({{":/qmldesigner/images/anchor_fill.png", Utils::Theme::IconsBaseColor},
                                       {":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}}).icon(),
                          anchorsResetToolTip,
                          anchorsCategory,
                          QKeySequence(QKeySequence("Ctrl+Shift+r")),
                          180,
                          &anchorsReset,
                          &singleSelectionItemIsAnchored));

    addDesignerAction(new SeperatorDesignerAction(anchorsCategory, 170));

    addDesignerAction(new ActionGroup(
                          positionCategoryDisplayName,
                          positionCategory,
                          priorityPositionCategory,
                          &positionOptionVisible));

    addDesignerAction(new ActionGroup(
                          layoutCategoryDisplayName,
                          layoutCategory,
                          priorityLayoutCategory,
                          &layoutOptionVisible));

    addDesignerAction(new ActionGroup(groupCategoryDisplayName,
                                      groupCategory,
                                      priorityGroupCategory,
                                      &studioComponentsAvailableAndSelectionCanBeLayouted));

    addDesignerAction(new ActionGroup(
        flowCategoryDisplayName,
        flowCategory,
        priorityFlowCategory,
        &isFlowTargetOrTransition,
        &flowOptionVisible));


    auto effectMenu = new ActionGroup(
                flowEffectCategoryDisplayName,
                flowEffectCategory,
                priorityFlowCategory,
                &isFlowTransitionItem,
                &flowOptionVisible);

    effectMenu->setCategory(flowCategory);
    addDesignerAction(effectMenu);

    addDesignerAction(new ModelNodeFormEditorAction(
        createFlowActionAreaCommandId,
        createFlowActionAreaDisplayName,
        addIcon.icon(),
        addFlowActionToolTip,
        flowCategory,
        {},
        priorityFirst,
        &createFlowActionArea,
        &isFlowItem,
        &flowOptionVisible));

    addDesignerAction(new ModelNodeContextMenuAction(
                          setFlowStartCommandId,
                          setFlowStartDisplayName,
                          {},
                          flowCategory,
                          priorityFirst,
                          {},
                          &setFlowStartItem,
                          &isFlowItem,
                          &flowOptionVisible));

    addDesignerAction(new FlowActionConnectAction(
        flowConnectionCategoryDisplayName,
        flowConnectionCategory,
        priorityFlowCategory));


    const QList<TypeName> transitionTypes = {"FlowFadeEffect",
                                   "FlowPushEffect",
                                   "FlowMoveEffect",
                                   "None"};

    for (const TypeName &typeName : transitionTypes)
        addTransitionEffectAction(typeName);

    addCustomTransitionEffectAction();

    addDesignerAction(new ModelNodeContextMenuAction(
        selectFlowEffectCommandId,
        selectEffectDisplayName,
        {},
        flowCategory,
        {},
        priorityFlowCategory,
        &selectFlowEffect,
        &isFlowTransitionItemWithEffect));

    addDesignerAction(new ActionGroup(
                          stackedContainerCategoryDisplayName,
                          stackedContainerCategory,
                          priorityStackedContainerCategory,
                          &isStackedContainer));

    addDesignerAction(new ModelNodeContextMenuAction(
                          removePositionerCommandId,
                          removePositionerDisplayName,
                          {},
                          positionCategory,
                          QKeySequence("Ctrl+Shift+p"),
                          210,
                          &removePositioner,
                          &isPositioner,
                          &isPositioner));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutRowPositionerCommandId,
                          layoutRowPositionerDisplayName,
                          {},
                          positionCategory,
                          QKeySequence(),
                          200,
                          &layoutRowPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutColumnPositionerCommandId,
                          layoutColumnPositionerDisplayName,
                          {},
                          positionCategory,
                          QKeySequence(),
                          180,
                          &layoutColumnPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutGridPositionerCommandId,
                          layoutGridPositionerDisplayName,
                          {},
                          positionCategory,
                          QKeySequence(),
                          160,
                          &layoutGridPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutFlowPositionerCommandId,
                          layoutFlowPositionerDisplayName,
                          {},
                          positionCategory,
                          QKeySequence("Ctrl+m"),
                          140,
                          &layoutFlowPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new SeperatorDesignerAction(layoutCategory, 120));

    addDesignerAction(new ModelNodeContextMenuAction(
                          removeLayoutCommandId,
                          removeLayoutDisplayName,
                          {},
                          layoutCategory,
                          QKeySequence(),
                          110,
                          &removeLayout,
                          &isLayout,
                          &isLayout));

    addDesignerAction(new ModelNodeContextMenuAction(addToGroupItemCommandId,
                                                     addToGroupItemDisplayName,
                                                     {},
                                                     groupCategory,
                                                     QKeySequence("Ctrl+Shift+g"),
                                                     110,
                                                     &addToGroupItem,
                                                     &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(removeGroupItemCommandId,
                                                     removeGroupItemDisplayName,
                                                     {},
                                                     groupCategory,
                                                     QKeySequence(),
                                                     110,
                                                     &removeGroup,
                                                     &isGroup));

    addDesignerAction(new ModelNodeFormEditorAction(addItemToStackedContainerCommandId,
                                                    addItemToStackedContainerDisplayName,
                                                    addIcon.icon(),
                                                    addItemToStackedContainerToolTip,
                                                    stackedContainerCategory,
                                                    QKeySequence("Ctrl+Shift+a"),
                                                    110,
                                                    &addItemToStackedContainer,
                                                    &isStackedContainer,
                                                    &isStackedContainer));

    addDesignerAction(new ModelNodeContextMenuAction(
                          addTabBarToStackedContainerCommandId,
                          addTabBarToStackedContainerDisplayName,
                          {},
                          stackedContainerCategory,
                          QKeySequence("Ctrl+Shift+t"),
                          100,
                          &addTabBarToStackedContainer,
                          &isStackedContainerWithoutTabBar,
                          &isStackedContainer));

    addDesignerAction(new ModelNodeFormEditorAction(
                          decreaseIndexOfStackedContainerCommandId,
                          decreaseIndexToStackedContainerDisplayName,
                          prevIcon.icon(),
                          decreaseIndexOfStackedContainerToolTip,
                          stackedContainerCategory,
                          QKeySequence("Ctrl+Shift+Left"),
                          80,
                          &decreaseIndexOfStackedContainer,
                          &isStackedContainerAndIndexCanBeDecreased,
                          &isStackedContainer));

    addDesignerAction(new ModelNodeFormEditorAction(
                          increaseIndexOfStackedContainerCommandId,
                          increaseIndexToStackedContainerDisplayName,
                          nextIcon.icon(),
                          increaseIndexOfStackedContainerToolTip,
                          stackedContainerCategory,
                          QKeySequence("Ctrl+Shift+Right"),
                          80,
                          &increaseIndexOfStackedContainer,
                          &isStackedContainerAndIndexCanBeIncreased,
                          &isStackedContainer));

    addDesignerAction(new ModelNodeAction(
                          layoutRowLayoutCommandId,
                          layoutRowLayoutDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/row.png", Utils::Theme::IconsBaseColor}}).icon(),
                          layoutRowLayoutToolTip,
                          layoutCategory,
                          QKeySequence("Ctrl+u"),
                          100,
                          &layoutRowLayout,
                          &selectionCanBeLayoutedAndQtQuickLayoutPossible));

    addDesignerAction(new ModelNodeAction(
                          layoutColumnLayoutCommandId,
                          layoutColumnLayoutDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/column.png", Utils::Theme::IconsBaseColor}}).icon(),
                          layoutColumnLayoutToolTip,
                          layoutCategory,
                          QKeySequence("Ctrl+l"),
                          80,
                          &layoutColumnLayout,
                          &selectionCanBeLayoutedAndQtQuickLayoutPossible));

    addDesignerAction(new ModelNodeAction(
                          layoutGridLayoutCommandId,
                          layoutGridLayoutDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/grid.png", Utils::Theme::IconsBaseColor}}).icon(),
                          layoutGridLayoutToolTip,
                          layoutCategory,
                          QKeySequence("shift+g"),
                          60,
                          &layoutGridLayout,
                          &selectionCanBeLayoutedAndQtQuickLayoutPossible));

    addDesignerAction(new SeperatorDesignerAction(layoutCategory, 50));

    addDesignerAction(new FillWidthModelNodeAction(
                          layoutFillWidthCommandId,
                          layoutFillWidthDisplayName,
                          layoutCategory,
                          QKeySequence(),
                          40,
                          &setFillWidth,
                          &singleSelectionAndInQtQuickLayout,
                          &singleSelectionAndInQtQuickLayout));

    addDesignerAction(new FillHeightModelNodeAction(
                          layoutFillHeightCommandId,
                          layoutFillHeightDisplayName,
                          layoutCategory,
                          QKeySequence(),
                          20,
                          &setFillHeight,
                          &singleSelectionAndInQtQuickLayout,
                          &singleSelectionAndInQtQuickLayout));

    addDesignerAction(new SeperatorDesignerAction(rootCategory, priorityTopLevelSeperator));

    addDesignerAction(new ModelNodeContextMenuAction(
                          goIntoComponentCommandId,
                          goIntoComponentDisplayName,
                          {},
                          rootCategory,
                          QKeySequence(Qt::Key_F2),
                          priorityGoIntoComponent,
                          &goIntoComponentOperation,
                          &selectionIsComponent));

    addDesignerAction(new ModelNodeContextMenuAction(
                          editAnnotationCommandId,
                          editAnnotationDisplayName,
                          {},
                          rootCategory,
                          QKeySequence(),
                          (priorityLast+6),
                          &editAnnotation,
                          &singleSelection,
                          &singleSelection));

    addDesignerAction(new ModelNodeContextMenuAction(
                          goToImplementationCommandId,
                          goToImplementationDisplayName,
                          {},
                          rootCategory,
                          QKeySequence(),
                          42,
                          &goImplementation,
                          &singleSelectedAndUiFile,
                          &singleSelectedAndUiFile));

    addDesignerAction(new ModelNodeContextMenuAction(
                          addSignalHandlerCommandId,
                          addSignalHandlerDisplayName,
                          {},
                          rootCategory, QKeySequence(),
                          42, &addNewSignalHandler,
                          &singleSelectedAndUiFile,
                          &singleSelectedAndUiFile));

    addDesignerAction(new ModelNodeContextMenuAction(
                          moveToComponentCommandId,
                          moveToComponentDisplayName,
                          {},
                          rootCategory,
                          QKeySequence(),
                          44,
                          &moveToComponent,
                          &singleSelection,
                          &singleSelection));

    addDesignerAction(new ModelNodeContextMenuAction(mergeTemplateCommandId,
                                                     mergeTemplateDisplayName,
                                                     {},
                                                     rootCategory,
                                                     {},
                                                     50,
                                                     &mergeWithTemplate,
                                                     &SelectionContextFunctors::always));

    addDesignerAction(new ActionGroup(
                          "",
                          genericToolBarCategory,
                          priorityGenericToolBar));

    addDesignerAction(new ChangeStyleAction());

    addDesignerAction(new EditListModelAction);

    addDesignerAction(new ModelNodeContextMenuAction(
                          openSignalDialogCommandId,
                          openSignalDialogDisplayName,
                          {},
                          rootCategory,
                          QKeySequence(),
                          66,
                          &openSignalDialog,
                          &singleSelectionAndHasSlotTrigger));

    addDesignerAction(new ModelNodeContextMenuAction(
                          update3DAssetCommandId,
                          update3DAssetDisplayName,
                          {},
                          rootCategory,
                          QKeySequence(),
                          priorityGenericToolBar,
                          &updateImported3DAsset,
                          &selectionIsImported3DAsset,
                          &selectionIsImported3DAsset));
}

void DesignerActionManager::createDefaultAddResourceHandler()
{
    auto registerHandlers = [this](const QStringList &exts, AddResourceOperation op,
                                   const QString &category) {
        for (const QString &ext : exts)
            registerAddResourceHandler(AddResourceHandler(category, ext, op));
    };

    // The filters will be displayed in reverse order to these lists in file dialog,
    // so declare most common types last
    registerHandlers({"*.webp",  "*.hdr", "*.svg", "*.bmp", "*.jpg", "*.png"},
                     ModelNodeOperations::addImageToProject,
                     ComponentCoreConstants::addImagesDisplayString);
    registerHandlers({"*.otf", "*.ttf"},
                     ModelNodeOperations::addFontToProject,
                     ComponentCoreConstants::addFontsDisplayString);
    registerHandlers({"*.wav"},
                     ModelNodeOperations::addSoundToProject,
                     ComponentCoreConstants::addSoundsDisplayString);
    registerHandlers({"*.glsl", "*.glslv", "*.glslf", "*.vsh", "*.fsh", "*.vert", "*.frag"},
                     ModelNodeOperations::addShaderToProject,
                     ComponentCoreConstants::addShadersDisplayString);
}

void DesignerActionManager::createDefaultModelNodePreviewImageHandlers()
{
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("QtQuick.Image",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("QtQuick.BorderImage",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("Qt.SafeRenderer.SafeRendererImage",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("Qt.SafeRenderer.SafeRendererPicture",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("QtQuick3D.Texture",
                                             ModelNodeOperations::previewImageDataForImageNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("QtQuick3D.Material",
                                             ModelNodeOperations::previewImageDataForGenericNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("QtQuick3D.Model",
                                             ModelNodeOperations::previewImageDataForGenericNode));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("QtQuick3D.Node",
                                             ModelNodeOperations::previewImageDataForGenericNode,
                                             true));
    registerModelNodePreviewHandler(
                ModelNodePreviewImageHandler("QtQuick.Item",
                                             ModelNodeOperations::previewImageDataForGenericNode,
                                             true));

    // TODO - Disabled until QTBUG-86616 is fixed
//    registerModelNodePreviewHandler(
//                ModelNodePreviewImageHandler("QtQuick3D.Effect",
//                                             ModelNodeOperations::previewImageDataFor3DNode));
}

void DesignerActionManager::addDesignerAction(ActionInterface *newAction)
{
    m_designerActions.append(QSharedPointer<ActionInterface>(newAction));
}

void DesignerActionManager::addCreatorCommand(Core::Command *command, const QByteArray &category, int priority,
                                              const QIcon &overrideIcon)
{
    addDesignerAction(new CommandAction(command, category, priority, overrideIcon));
}

QList<QSharedPointer<ActionInterface> > DesignerActionManager::actionsForTargetView(const ActionInterface::TargetView &target)
{
    QList<QSharedPointer<ActionInterface> > out;
    for (auto interface : qAsConst(m_designerActions))
        if (interface->targetView() == target)
            out << interface;

    return out;
}

QList<ActionInterface* > DesignerActionManager::designerActions() const
{
    return Utils::transform(m_designerActions, [](const QSharedPointer<ActionInterface> &pointer) {
        return pointer.data();
    });
}

ActionInterface *DesignerActionManager::actionByMenuId(const QByteArray &id)
{
    for (const auto &action : m_designerActions)
        if (action->menuId() == id)
            return action.data();
    return nullptr;
}

DesignerActionManager::DesignerActionManager(DesignerActionManagerView *designerActionManagerView)
    : m_designerActionManagerView(designerActionManagerView)
{
}

DesignerActionManager::~DesignerActionManager() = default;

void DesignerActionManager::addTransitionEffectAction(const TypeName &typeName)
{
    addDesignerAction(new ModelNodeContextMenuAction(
        QByteArray(ComponentCoreConstants::flowAssignEffectCommandId) + typeName,
        QLatin1String(ComponentCoreConstants::flowAssignEffectDisplayName) + typeName,
        {},
        ComponentCoreConstants::flowEffectCategory,
        {},
        typeName == "None" ? 100 : 140,
        [typeName](const SelectionContext &context)
        { ModelNodeOperations::addFlowEffect(context, typeName); },
    &isFlowTransitionItem));
}

void DesignerActionManager::addCustomTransitionEffectAction()
{
    addDesignerAction(new ModelNodeContextMenuAction(
        QByteArray(ComponentCoreConstants::flowAssignEffectCommandId),
        ComponentCoreConstants::flowAssignCustomEffectDisplayName,
        {},
        ComponentCoreConstants::flowEffectCategory,
        {},
        80,
        &ModelNodeOperations::addCustomFlowEffect,
    &isFlowTransitionItem));
}

DesignerActionToolBar::DesignerActionToolBar(QWidget *parentWidget) : Utils::StyledBar(parentWidget),
    m_toolBar(new QToolBar("ActionToolBar", this))
{
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    m_toolBar->addWidget(empty);

    m_toolBar->setContentsMargins(0, 0, 0, 0);
    m_toolBar->setFloatable(true);
    m_toolBar->setMovable(true);
    m_toolBar->setOrientation(Qt::Horizontal);

    auto horizontalLayout = new QHBoxLayout(this);

    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);

    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);

    horizontalLayout->addWidget(m_toolBar);
}

void DesignerActionToolBar::registerAction(ActionInterface *action)
{
    m_toolBar->addAction(action->action());
}

void DesignerActionToolBar::addSeparator()
{
    auto separatorAction = new QAction(m_toolBar);
    separatorAction->setSeparator(true);
    m_toolBar->addAction(separatorAction);
}

} //QmlDesigner
