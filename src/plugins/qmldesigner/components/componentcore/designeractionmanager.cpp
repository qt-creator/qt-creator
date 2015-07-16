/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "designeractionmanager.h"
#include "modelnodecontextmenu_helper.h"
#include <nodeproperty.h>
#include <nodemetainfo.h>
#include "designeractionmanagerview.h"

#include <documentmanager.h>
#include <qmldesignerplugin.h>

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

AbstractView *DesignerActionManager::view()
{
    return m_designerActionManagerView;
}

class VisiblityModelNodeAction : public ModelNodeAction
{
public:
    VisiblityModelNodeAction(const QString &description, const QByteArray &category, int priority,
            ModelNodeOperations::SelectionAction action,
            SelectionContextFunction enabled = &SelectionContextFunctors::always,
            SelectionContextFunction visibility = &SelectionContextFunctors::always) :
        ModelNodeAction(description, category, priority, action, enabled, visibility)
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

class FillLayoutModelNodeAction : public ModelNodeAction
{
public:
    FillLayoutModelNodeAction(const QString &description, const QByteArray &category, int priority,
            ModelNodeOperations::SelectionAction action,
            SelectionContextFunction enabled = &SelectionContextFunctors::always,
            SelectionContextFunction visibility = &SelectionContextFunctors::always) :
        ModelNodeAction(description, category, priority, action, enabled, visibility)
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
    FillWidthModelNodeAction(const QString &description, const QByteArray &category, int priority,
            ModelNodeOperations::SelectionAction action,
            SelectionContextFunction enabled = &SelectionContextFunctors::always,
            SelectionContextFunction visibility = &SelectionContextFunctors::always) :
        FillLayoutModelNodeAction(description, category, priority, action, enabled, visibility)
    {
        m_propertyName = "Layout.fillWidth";
    }
};

class FillHeightModelNodeAction : public FillLayoutModelNodeAction
{
public:
    FillHeightModelNodeAction(const QString &description, const QByteArray &category, int priority,
            ModelNodeOperations::SelectionAction action,
            SelectionContextFunction enabled = &SelectionContextFunctors::always,
            SelectionContextFunction visibility = &SelectionContextFunctors::always) :
        FillLayoutModelNodeAction(description, category, priority, action, enabled, visibility)
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

char xProperty[] = "x";
char yProperty[] = "y";
char zProperty[] = "z";
char widthProperty[] = "width";
char heightProperty[] = "height";

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

    return metaInfo.isSubclassOf("QtQuick.Layouts.Layout", -1, -1);
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

    return metaInfo.isSubclassOf("QtQuick.Layouts.Layout", -1, -1);
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

    return metaInfo.isSubclassOf("<cpp>.QDeclarativeBasePositioner", -1, -1)
            || metaInfo.isSubclassOf("QtQuick.Positioner", -1, -1);
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

    addDesignerAction(new SelectionModelNodeAction(selectionCategoryDisplayName, selectionCategory, prioritySelectionCategory));

    addDesignerAction(new ActionGroup(stackCategoryDisplayName, stackCategory, priorityStackCategory, &selectionNotEmpty));
        addDesignerAction(new ModelNodeAction
                   (toFrontDisplayName, stackCategory, 200, &toFront, &singleSelection));
        addDesignerAction(new ModelNodeAction
                   (toBackDisplayName, stackCategory, 180, &toBack, &singleSelection));
        addDesignerAction(new ModelNodeAction
                   (raiseDisplayName, stackCategory, 160, &raise, &selectionNotEmpty));
        addDesignerAction(new ModelNodeAction
                   (lowerDisplayName, stackCategory, 140, &lower, &selectionNotEmpty));
        addDesignerAction(new SeperatorDesignerAction(stackCategory, 120));
        addDesignerAction(new ModelNodeAction
                   (resetZDisplayName, stackCategory, 100, &resetZ, &selectionNotEmptyAndHasZProperty));

    addDesignerAction(new ActionGroup(editCategoryDisplayName, editCategory, priorityEditCategory, &selectionNotEmpty));
        addDesignerAction(new ModelNodeAction
                   (resetPositionDisplayName, editCategory, 200, &resetPosition, &selectionNotEmptyAndHasXorYProperty));
        addDesignerAction(new ModelNodeAction
                   (resetSizeDisplayName, editCategory, 180, &resetSize, &selectionNotEmptyAndHasWidthOrHeightProperty));
        addDesignerAction(new VisiblityModelNodeAction
                   (visibilityDisplayName, editCategory, 160, &setVisible, &singleSelectedItem));

    addDesignerAction(new ActionGroup(anchorsCategoryDisplayName, anchorsCategory,
                    priorityAnchorsCategory, &singleSelectionAndInBaseState));
        addDesignerAction(new ModelNodeAction
                   (anchorsFillDisplayName, anchorsCategory, 200, &anchorsFill, &singleSelectionItemIsNotAnchoredAndSingleSelectionNotRoot));
        addDesignerAction(new ModelNodeAction
                   (anchorsResetDisplayName, anchorsCategory, 180, &anchorsReset, &singleSelectionItemIsAnchored));

        addDesignerAction(new ActionGroup(positionCategoryDisplayName, positionCategory,
                                          priorityPositionCategory, &positionOptionVisible));
        addDesignerAction(new ActionGroup(layoutCategoryDisplayName, layoutCategory,
                    priorityLayoutCategory, &layoutOptionVisible));

        addDesignerAction(new ModelNodeAction
                          (removePositionerDisplayName,
                           positionCategory,
                           210,
                           &removePositioner,
                           &isPositioner,
                           &isPositioner));

        addDesignerAction(new ModelNodeAction
                   (layoutRowPositionerDisplayName,
                    positionCategory,
                    200,
                    &layoutRowPositioner,
                    &selectionCanBeLayouted,
                    &selectionCanBeLayouted));

        addDesignerAction(new ModelNodeAction
                   (layoutColumnPositionerDisplayName,
                    positionCategory,
                    180,
                    &layoutColumnPositioner,
                    &selectionCanBeLayouted,
                    &selectionCanBeLayouted));

        addDesignerAction(new ModelNodeAction
                   (layoutGridPositionerDisplayName,
                    positionCategory,
                    160,
                    &layoutGridPositioner,
                    &selectionCanBeLayouted,
                    &selectionCanBeLayouted));

        addDesignerAction(new ModelNodeAction
                   (layoutFlowPositionerDisplayName,
                    positionCategory,
                    140,
                    &layoutFlowPositioner,
                    &selectionCanBeLayouted,
                    &selectionCanBeLayouted));

        addDesignerAction(new SeperatorDesignerAction(layoutCategory, 120));

        addDesignerAction(new ModelNodeAction
                          (removeLayoutDisplayName,
                           layoutCategory,
                           110,
                           &removeLayout,
                           &isLayout,
                           &isLayout));

        addDesignerAction(new ModelNodeAction
                   (layoutRowLayoutDisplayName,
                    layoutCategory,
                    100,
                    &layoutRowLayout,
                    &selectionCanBeLayoutedAndQtQuickLayoutPossible,
                    &selectionCanBeLayoutedAndQtQuickLayoutPossible));

        addDesignerAction(new ModelNodeAction
                   (layoutColumnLayoutDisplayName,
                    layoutCategory,
                    80,
                    &layoutColumnLayout,
                    &selectionCanBeLayoutedAndQtQuickLayoutPossible,
                    &selectionCanBeLayoutedAndQtQuickLayoutPossible));

        addDesignerAction(new ModelNodeAction
                   (layoutGridLayoutDisplayName,
                    layoutCategory,
                    60,
                    &layoutGridLayout,
                    &selectionCanBeLayoutedAndQtQuickLayoutPossible,
                    &selectionCanBeLayoutedAndQtQuickLayoutPossible));

        addDesignerAction(new FillWidthModelNodeAction
                          (layoutFillWidthDisplayName,
                           layoutCategory,
                           40,
                           &setFillWidth,
                           &singleSelectionAndInQtQuickLayout,
                           &singleSelectionAndInQtQuickLayout));

        addDesignerAction(new FillHeightModelNodeAction
                          (layoutFillHeightDisplayName,
                           layoutCategory,
                           20,
                           &setFillHeight,
                           &singleSelectionAndInQtQuickLayout,
                           &singleSelectionAndInQtQuickLayout));

    addDesignerAction(new SeperatorDesignerAction(rootCategory, priorityTopLevelSeperator));
    addDesignerAction(new ModelNodeAction
               (goIntoComponentDisplayName, rootCategory, priorityGoIntoComponent, &goIntoComponent, &selectionIsComponent));
    addDesignerAction(new ModelNodeAction
               (goToImplementation, rootCategory, 42, &gotoImplementation, &singleSelectedAndUiFile, &singleSelectedAndUiFile));

}

void DesignerActionManager::addDesignerAction(ActionInterface *newAction)
{
    m_designerActions.append(QSharedPointer<ActionInterface>(newAction));
    m_designerActionManagerView->setDesignerActionList(designerActions());
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

} //QmlDesigner
