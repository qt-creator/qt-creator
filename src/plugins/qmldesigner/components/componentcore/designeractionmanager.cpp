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
#include "modelnodecontextmenu_helper.h"
#include <bindingproperty.h>
#include <nodeproperty.h>
#include <nodehints.h>
#include <nodemetainfo.h>
#include "designeractionmanagerview.h"
#include "qmldesignerconstants.h"

#include <formeditortoolbutton.h>

#include <documentmanager.h>
#include <qmldesignerplugin.h>
#include <viewmanager.h>

#include <QHBoxLayout>
#include <QGraphicsLinearLayout>

#include <coreplugin/actionmanager/actionmanager.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

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
    DesignerActionToolBar *toolBar = new DesignerActionToolBar(parent);

    QList<ActionInterface* > categories = Utils::filtered(designerActions(),
                                                          [](ActionInterface *action) { return action->type() ==  ActionInterface::ContextMenu; });

    Utils::sort(categories, [](ActionInterface *l, ActionInterface *r) {
        return l->priority() > r->priority();
    });

    for (auto *categoryAction : categories) {
        QList<ActionInterface* > actions = Utils::filtered(designerActions(), [categoryAction](ActionInterface *action) {
                return action->category() == categoryAction->menuId();
        });

        Utils::sort(actions, [](ActionInterface *l, ActionInterface *r) {
            return l->priority() > r->priority();
        });

        bool addSeparator = false;

        for (auto *action : actions) {
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

    for (auto *action : actions) {
        if (!action->menuId().isEmpty()) {
            const QString id =
                    QString("QmlDesigner.%1").arg(QString::fromLatin1(action->menuId()));

            Core::Command *cmd = Core::ActionManager::registerAction(action->action(), id.toLatin1().constData(), qmlDesignerFormEditorContext);

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

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    toolbar->setLayout(layout);

    for (ActionInterface *action : actions) {
        FormEditorToolButton *button = new FormEditorToolButton(action->action(), toolbar);
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

class VisiblityModelNodeAction : public ModelNodeContextMenuAction
{
public:
    VisiblityModelNodeAction(const QByteArray &id, const QString &description, const QByteArray &category, const QKeySequence &key, int priority,
                             SelectionContextOperation action,
                             SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                             SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        ModelNodeContextMenuAction(id, description, category, key, priority, action, enabled, visibility)
    {}

    virtual void updateContext()
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
        ModelNodeContextMenuAction(id, description, category, key, priority, action, enabled, visibility)
    {}
    virtual void updateContext()
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

    virtual void updateContext()
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

                ActionTemplate *selectionAction = new ActionTemplate(QString(), &ModelNodeOperations::select);
                selectionAction->setParent(menu());

                parentNode = selectionContext().currentSingleSelectedNode().parentProperty().parentModelNode();

                selectionAction->setText(QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select parent: %1")).arg(
                                             captionForModelNode(parentNode)));

                SelectionContext nodeSelectionContext = selectionContext();
                nodeSelectionContext.setTargetNode(parentNode);
                selectionAction->setSelectionContext(nodeSelectionContext);

                menu()->addAction(selectionAction);
            }
            foreach (const ModelNode &node, selectionContext().view()->allModelNodes()) {
                if (node != selectionContext().currentSingleSelectedNode()
                        && node != parentNode
                        && contains(node, selectionContext().scenePosition())
                        && !node.isRootNode()) {
                    selectionContext().setTargetNode(node);
                    QString what = QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select: %1")).arg(captionForModelNode(node));
                    ActionTemplate *selectionAction = new ActionTemplate(what, &ModelNodeOperations::select);

                    SelectionContext nodeSelectionContext = selectionContext();
                    nodeSelectionContext.setTargetNode(node);
                    selectionAction->setSelectionContext(nodeSelectionContext);

                    menu()->addAction(selectionAction);
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

using namespace SelectionContextFunctors;

bool multiSelection(const SelectionContext &context)
{
    return !singleSelection(context) && selectionNotEmpty(context);
}

bool singleSelectionAndInBaseState(const SelectionContext &context)
{
    return singleSelection(context) && inBaseState(context);
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

bool isNotInLayout(const SelectionContext &context)
{
    if (selectionNotEmpty(context)) {
        ModelNode selectedModelNode = context.selectedModelNodes().first();
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

void DesignerActionManager::createDefaultDesignerActions()
{
    using namespace SelectionContextFunctors;
    using namespace ComponentCoreConstants;
    using namespace ModelNodeOperations;

    addDesignerAction(new SelectionModelNodeAction(
                          selectionCategoryDisplayName,
                          selectionCategory,
                          prioritySelectionCategory));

    addDesignerAction(new ActionGroup(
                          stackCategoryDisplayName,
                          stackCategory,
                          priorityStackCategory,
                          &selectionNotEmpty));

    addDesignerAction(new ModelNodeContextMenuAction(
                          toFrontCommandId,
                          toFrontDisplayName,
                          stackCategory,
                          QKeySequence(),
                          200,
                          &toFront,
                          &singleSelection));

    addDesignerAction(new ModelNodeContextMenuAction(
                          toBackCommandId,
                          toBackDisplayName,
                          stackCategory,
                          QKeySequence(),
                          180,
                          &toBack,
                          &singleSelection));

    addDesignerAction(new ModelNodeAction(
                          raiseCommandId, raiseDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/lower.png", Utils::Theme::IconsBaseColor}}).icon(),
                          raiseToolTip,
                          stackCategory,
                          QKeySequence("Ctrl+Up"),
                          160,
                          &raise,
                          &selectionNotEmpty));

    addDesignerAction(new ModelNodeAction(
                          lowerCommandId,
                          lowerDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/raise.png", Utils::Theme::IconsBaseColor}}).icon(),
                          lowerToolTip,
                          stackCategory,
                          QKeySequence("Ctrl+Down"),
                          140,
                          &lower,
                          &selectionNotEmpty));

    addDesignerAction(new SeperatorDesignerAction(stackCategory, 120));

    addDesignerAction(new ModelNodeContextMenuAction(
                          resetZCommandId,
                          resetZDisplayName,
                          stackCategory,
                          QKeySequence(),
                          100,
                          &resetZ,
                          &selectionNotEmptyAndHasZProperty));

    addDesignerAction(new ActionGroup(editCategoryDisplayName, editCategory, priorityEditCategory, &selectionNotEmpty));

    addDesignerAction(new SeperatorDesignerAction(editCategory, 220));

    addDesignerAction(new ModelNodeAction(
                          resetPositionCommandId,
                          resetPositionDisplayName,
                          Utils::Icon({{":/utils/images/pan.png", Utils::Theme::IconsBaseColor},
                                      {":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}}).icon(),
                          resetPositionTooltip, editCategory, QKeySequence("Ctrl+d"),
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
                          QKeySequence("Ctrl+f"),
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

    addDesignerAction(new ActionGroup(
                          anchorsCategoryDisplayName,
                          anchorsCategory,
                          priorityAnchorsCategory,
                          &singleSelectionAndInBaseState));

    addDesignerAction(new ModelNodeAction(
                          anchorsFillCommandId,
                          anchorsFillDisplayName,
                          Utils::Icon({{":/qmldesigner/images/anchor_fill.png", Utils::Theme::IconsBaseColor}}).icon(),
                          anchorsFillToolTip,
                          anchorsCategory,
                          QKeySequence(QKeySequence("Ctrl+f")),
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
                          QKeySequence(QKeySequence("Ctrl+Shift+f")),
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

    addDesignerAction(new ActionGroup(
                          stackCategoryDisplayName,
                          stackedContainerCategory,
                          priorityStackedContainerCategory,
                          &isStackedContainer));

    addDesignerAction(new ModelNodeContextMenuAction(
                          removePositionerCommandId,
                          removePositionerDisplayName,
                          positionCategory,
                          QKeySequence("Ctrl+Shift+p"),
                          210,
                          &removePositioner,
                          &isPositioner,
                          &isPositioner));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutRowPositionerCommandId,
                          layoutRowPositionerDisplayName,
                          positionCategory,
                          QKeySequence(),
                          200,
                          &layoutRowPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutColumnPositionerCommandId,
                          layoutColumnPositionerDisplayName,
                          positionCategory,
                          QKeySequence(),
                          180,
                          &layoutColumnPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutGridPositionerCommandId,
                          layoutGridPositionerDisplayName,
                          positionCategory,
                          QKeySequence(),
                          160,
                          &layoutGridPositioner,
                          &selectionCanBeLayouted,
                          &selectionCanBeLayouted));

    addDesignerAction(new ModelNodeContextMenuAction(
                          layoutFlowPositionerCommandId,
                          layoutFlowPositionerDisplayName,
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
                          layoutCategory,
                          QKeySequence("Ctrl+Shift+u"),
                          110,
                          &removeLayout,
                          &isLayout,
                          &isLayout));

    const Utils::Icon prevIcon({
        {QLatin1String(":/utils/images/prev.png"), Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    const Utils::Icon nextIcon({
        {QLatin1String(":/utils/images/next.png"), Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);

    const Utils::Icon addIcon({
        {QLatin1String(":/utils/images/plus.png"), Utils::Theme::QmlDesigner_FormEditorForegroundColor}}, Utils::Icon::MenuTintedStyle);


    addDesignerAction(new ModelNodeFormEditorAction(
                          addItemToStackedContainerCommandId,
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
                          QKeySequence("Ctrl+i"),
                          80,
                          &layoutColumnLayout,
                          &selectionCanBeLayoutedAndQtQuickLayoutPossible));

    addDesignerAction(new ModelNodeAction(
                          layoutGridLayoutCommandId,
                          layoutGridLayoutDisplayName,
                          Utils::Icon({{":/qmldesigner/icon/designeractions/images/grid.png", Utils::Theme::IconsBaseColor}}).icon(),
                          layoutGridLayoutToolTip,
                          layoutCategory,
                          QKeySequence("Ctrl+h"),
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
                          rootCategory,
                          QKeySequence(),
                          priorityGoIntoComponent,
                          &goIntoComponentOperation,
                          &selectionIsComponent));

    addDesignerAction(new ModelNodeContextMenuAction(
                          goToImplementationCommandId,
                          goToImplementationDisplayName,
                          rootCategory,
                          QKeySequence(),
                          42,
                          &goImplementation,
                          &singleSelectedAndUiFile,
                          &singleSelectedAndUiFile));

    addDesignerAction(new ModelNodeContextMenuAction(
                          addSignalHandlerCommandId,
                          addSignalHandlerDisplayName,
                          rootCategory, QKeySequence(),
                          42, &addNewSignalHandler,
                          &singleSelectedAndUiFile,
                          &singleSelectedAndUiFile));

    addDesignerAction(new ModelNodeContextMenuAction(
                          moveToComponentCommandId,
                          moveToComponentDisplayName,
                          rootCategory,
                          QKeySequence(),
                          44,
                          &moveToComponent,
                          &singleSelection,
                          &singleSelection));

    addDesignerAction(new ActionGroup(
                          "",
                          genericToolBarCategory,
                          priorityGenericToolBar));

    addDesignerAction(new ChangeStyleAction());
}

void DesignerActionManager::addDesignerAction(ActionInterface *newAction)
{
    m_designerActions.append(QSharedPointer<ActionInterface>(newAction));
    m_designerActionManagerView->setDesignerActionList(designerActions());
}

void DesignerActionManager::addCreatorCommand(Core::Command *command, const QByteArray &category, int priority,
                                              const QIcon &overrideIcon)
{
    addDesignerAction(new CommandAction(command, category, priority, overrideIcon));
}

QList<ActionInterface* > DesignerActionManager::designerActions() const
{
    QList<ActionInterface* > list;
    foreach (const QSharedPointer<ActionInterface> &pointer, m_designerActions) {
        list.append(pointer.data());
    }

    return list;
}

DesignerActionManager::DesignerActionManager(DesignerActionManagerView *designerActionManagerView)
    : m_designerActionManagerView(designerActionManagerView)
{
}

DesignerActionManager::~DesignerActionManager()
{
}

DesignerActionToolBar::DesignerActionToolBar(QWidget *parentWidget) : Utils::StyledBar(parentWidget),
    m_toolBar(new QToolBar("ActionToolBar", this))
{
    m_toolBar->setContentsMargins(0, 0, 0, 0);
    m_toolBar->setFloatable(true);
    m_toolBar->setMovable(true);
    m_toolBar->setOrientation(Qt::Horizontal);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(this);

    horizontalLayout->setMargin(0);
    horizontalLayout->setSpacing(0);

    horizontalLayout->setMargin(0);
    horizontalLayout->setSpacing(0);

    horizontalLayout->addWidget(m_toolBar);
}

void DesignerActionToolBar::registerAction(ActionInterface *action)
{
    m_toolBar->addAction(action->action());
}

void DesignerActionToolBar::addSeparator()
{
    QAction *separatorAction = new QAction(m_toolBar);
    separatorAction->setSeparator(true);
    m_toolBar->addAction(separatorAction);
}

} //QmlDesigner
