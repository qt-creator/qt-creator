/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "designeractionmanager.h"
#include "modelnodecontextmenu_helper.h"
#include <nodeproperty.h>
#include <nodemetainfo.h>

namespace QmlDesigner {

static inline QString captionForModelNode(const ModelNode &modelNode)
{
    if (modelNode.id().isEmpty())
        return modelNode.simplifiedTypeName();

    return modelNode.id();
}

static inline bool contains(const QmlItemNode &node, const QPoint &position)
{
    return node.isValid() && node.instanceSceneTransform().mapRect(node.instanceBoundingRect()).contains(position);
}

namespace Internal {

class DesignerActionManagerView : public QmlModelView
{
public:
    DesignerActionManagerView() : QmlModelView(0), m_isInRewriterTransaction(false), m_setupContextDirty(false)
    {}

    void modelAttached(Model *model) QTC_OVERRIDE
    {
        QmlModelView::modelAttached(model);
        setupContext();
    }

    void modelAboutToBeDetached(Model *model) QTC_OVERRIDE
    {
        QmlModelView::modelAboutToBeDetached(model);
        setupContext();
    }

    virtual void nodeCreated(const ModelNode &) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void nodeAboutToBeRemoved(const ModelNode &) QTC_OVERRIDE
    {}

    virtual void nodeRemoved(const ModelNode &, const NodeAbstractProperty &, PropertyChangeFlags) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void nodeAboutToBeReparented(const ModelNode &,
                                         const NodeAbstractProperty &,
                                         const NodeAbstractProperty &,
                                         AbstractView::PropertyChangeFlags ) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void nodeReparented(const ModelNode &, const NodeAbstractProperty &,
                                const NodeAbstractProperty &,
                                AbstractView::PropertyChangeFlags) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void nodeIdChanged(const ModelNode&, const QString&, const QString&) QTC_OVERRIDE
    {}

    virtual void propertiesAboutToBeRemoved(const QList<AbstractProperty>&) QTC_OVERRIDE
    {}

    virtual void propertiesRemoved(const QList<AbstractProperty>&) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void variantPropertiesChanged(const QList<VariantProperty>&, PropertyChangeFlags) QTC_OVERRIDE
    {}

    virtual void bindingPropertiesChanged(const QList<BindingProperty>&, PropertyChangeFlags) QTC_OVERRIDE
    {}

    virtual void rootNodeTypeChanged(const QString &, int , int ) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &) QTC_OVERRIDE
    {}

    virtual void instancesCompleted(const QVector<ModelNode> &) QTC_OVERRIDE
    {}

    virtual void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &) QTC_OVERRIDE
    {}

    virtual void instancesRenderImageChanged(const QVector<ModelNode> &) QTC_OVERRIDE
    {}

    virtual void instancesPreviewImageChanged(const QVector<ModelNode> &) QTC_OVERRIDE
    {}

    virtual void instancesChildrenChanged(const QVector<ModelNode> &) QTC_OVERRIDE
    {}

    virtual void instancesToken(const QString &, int , const QVector<ModelNode> &) QTC_OVERRIDE
    {}

    virtual void nodeSourceChanged(const ModelNode &, const QString &) QTC_OVERRIDE
    {}

    virtual void rewriterBeginTransaction() QTC_OVERRIDE
    {
        m_isInRewriterTransaction = true;
    }

    virtual void rewriterEndTransaction() QTC_OVERRIDE
    {
        m_isInRewriterTransaction = false;

        if (m_setupContextDirty)
            setupContext();
    }

    virtual void actualStateChanged(const ModelNode &) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void selectedNodesChanged(const QList<ModelNode> &,
                                      const QList<ModelNode> &) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void nodeOrderChanged(const NodeListProperty &, const ModelNode &, int ) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void importsChanged(const QList<Import> &, const QList<Import> &) QTC_OVERRIDE
    {
        setupContext();
    }

    virtual void scriptFunctionsChanged(const ModelNode &, const QStringList &) QTC_OVERRIDE
    {}

    void setDesignerActionList(const QList<AbstractDesignerAction* > &designerActionList)
    {
        m_designerActionList = designerActionList;
    }

protected:
    void setupContext()
    {
        if (m_isInRewriterTransaction) {
            m_setupContextDirty = true;
            return;
        }
        SelectionContext selectionContext(this);
        foreach (AbstractDesignerAction* action, m_designerActionList) {
            action->currentContextChanged(selectionContext);
        }
        m_setupContextDirty = false;
    }

    QList<AbstractDesignerAction* > m_designerActionList;
    bool m_isInRewriterTransaction;
    bool m_setupContextDirty;
};

} //Internal

DesignerActionManager *DesignerActionManager::m_instance = 0;

void DesignerActionManager::addDesignerAction(AbstractDesignerAction *newAction)
{
    instance()->addDesignerActionInternal(newAction);
}

QList<AbstractDesignerAction* > DesignerActionManager::designerActions()
{
    return instance()->factoriesInternal();
}

QmlModelView *DesignerActionManager::view()
{
    return instance()->m_view;
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
                    || itemNode.propertyAffectedByCurrentState(m_propertyName))
                flag = itemNode.modelValue(m_propertyName).toBool();
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

class SelectionModelNodeAction : public MenuDesignerAction
{
public:
    SelectionModelNodeAction(const QString &displayName, const QByteArray &menuId, int priority) :
        MenuDesignerAction(displayName, menuId, priority,
                           &SelectionContextFunctors::always, &SelectionContextFunctors::selectionEnabled)

    {}

    virtual void updateContext()
    {
        m_menu->clear();
        if (m_selectionContext.isValid()) {
            m_action->setEnabled(isEnabled(m_selectionContext));
            m_action->setVisible(isVisible(m_selectionContext));
        } else {
            return;
        }
        if (m_action->isEnabled()) {
            ModelNode parentNode;
            if (m_selectionContext.singleNodeIsSelected() && !m_selectionContext.currentSingleSelectedNode().isRootNode()) {
                ActionTemplate *selectionAction = new ActionTemplate(QString(), &ModelNodeOperations::select);
                selectionAction->setParent(m_menu.data());

                parentNode = m_selectionContext.currentSingleSelectedNode().parentProperty().parentModelNode();
                m_selectionContext.setTargetNode(parentNode);
                selectionAction->setText(QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select parent: %1")).arg(
                                             captionForModelNode(parentNode)));
                selectionAction->setSelectionContext(m_selectionContext);

                m_menu->addAction(selectionAction);
            }
            foreach (const ModelNode &node, m_selectionContext.qmlModelView()->allModelNodes()) {
                if (node != m_selectionContext.currentSingleSelectedNode()
                        && node != parentNode
                        && contains(node, m_selectionContext.scenePos())
                        && !node.isRootNode()) {
                    m_selectionContext.setTargetNode(node);
                    QString what = QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select: %1")).arg(captionForModelNode(node));
                    ActionTemplate *selectionAction = new ActionTemplate(what, &ModelNodeOperations::select);
                    selectionAction->setSelectionContext(m_selectionContext);

                    m_menu->addAction(selectionAction);
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
    return selectionHasSameParentAndInBaseState(context)
            && inBaseState(context)
            && isNotInLayout(context);
}

bool hasQtQuickLayoutImport(const SelectionContext &context)
{
    if (context.qmlModelView() && context.qmlModelView()->model()) {
        Import import = Import::createLibraryImport(QLatin1String("QtQuick.Layouts"), QLatin1String("1.0"));
        return context.qmlModelView()->model()->hasImport(import, true, true);
    }

    return false;
}

bool selectionCanBeLayoutedAndasQtQuickLayoutImport(const SelectionContext &context)
{
    return selectionCanBeLayouted(context) && hasQtQuickLayoutImport(context);
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

bool layoutOptionVisible(const SelectionContext &context)
{
    return multiSelectionAndInBaseState(context)
            || singleSelectionAndInQtQuickLayout(context);
}

void DesignerActionManager::createDefaultDesignerActions()
{
    using namespace SelectionContextFunctors;
    using namespace ComponentCoreConstants;
    using namespace ModelNodeOperations;

    addDesignerAction(new SelectionModelNodeAction(selectionCategoryDisplayName, selectionCategory, prioritySelectionCategory));

    addDesignerAction(new MenuDesignerAction(stackCategoryDisplayName, stackCategory, priorityStackCategory, &selectionNotEmpty));
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

    addDesignerAction(new MenuDesignerAction(editCategoryDisplayName, editCategory, priorityEditCategory, &selectionNotEmpty));
        addDesignerAction(new ModelNodeAction
                   (resetPositionDisplayName, editCategory, 200, &resetPosition, &selectionNotEmptyAndHasXorYProperty));
        addDesignerAction(new ModelNodeAction
                   (resetSizeDisplayName, editCategory, 180, &resetSize, &selectionNotEmptyAndHasWidthOrHeightProperty));
        addDesignerAction(new VisiblityModelNodeAction
                   (visibilityDisplayName, editCategory, 160, &setVisible, &singleSelectedItem));

    addDesignerAction(new MenuDesignerAction(anchorsCategoryDisplayName, anchorsCategory,
                    priorityAnchorsCategory, &singleSelectionAndInBaseState));
        addDesignerAction(new ModelNodeAction
                   (anchorsFillDisplayName, anchorsCategory, 200, &anchorsFill, &singleSelectionItemIsNotAnchored));
        addDesignerAction(new ModelNodeAction
                   (anchorsResetDisplayName, anchorsCategory, 180, &anchorsReset, &singleSelectionItemIsAnchored));

    addDesignerAction(new MenuDesignerAction(layoutCategoryDisplayName, layoutCategory,
                    priorityLayoutCategory, &layoutOptionVisible));
        addDesignerAction(new ModelNodeAction
                   (layoutRowPositionerDisplayName,
                    layoutCategory,
                    200,
                    &layoutRowPositioner,
                    &selectionCanBeLayouted,
                    selectionCanBeLayouted));

        addDesignerAction(new ModelNodeAction
                   (layoutColumnPositionerDisplayName,
                    layoutCategory,
                    180,
                    &layoutColumnPositioner,
                    &selectionCanBeLayouted,
                    selectionCanBeLayouted));

        addDesignerAction(new ModelNodeAction
                   (layoutGridPositionerDisplayName,
                    layoutCategory,
                    160,
                    &layoutGridPositioner,
                    &selectionCanBeLayouted,
                    selectionCanBeLayouted));

        addDesignerAction(new ModelNodeAction
                   (layoutFlowPositionerDisplayName,
                    layoutCategory,
                    140,
                    &layoutFlowPositioner,
                    &selectionCanBeLayouted,
                    selectionCanBeLayouted));

        addDesignerAction(new SeperatorDesignerAction(layoutCategory, 120));

        addDesignerAction(new ModelNodeAction
                   (layoutRowLayoutDisplayName,
                    layoutCategory,
                    100,
                    &layoutRowLayout,
                    &selectionCanBeLayoutedAndasQtQuickLayoutImport,
                    &selectionCanBeLayoutedAndasQtQuickLayoutImport));

        addDesignerAction(new ModelNodeAction
                   (layoutColumnLayoutDisplayName,
                    layoutCategory,
                    80,
                    &layoutColumnLayout,
                    &selectionCanBeLayoutedAndasQtQuickLayoutImport,
                    &selectionCanBeLayoutedAndasQtQuickLayoutImport));

        addDesignerAction(new ModelNodeAction
                   (layoutGridLayoutDisplayName,
                    layoutCategory,
                    60,
                    &layoutGridLayout,
                    &selectionCanBeLayoutedAndasQtQuickLayoutImport,
                    &selectionCanBeLayoutedAndasQtQuickLayoutImport));

        addDesignerAction(new FillWidthModelNodeAction
                          (layoutFillWidthDisplayName,
                           layoutCategory,
                           40,
                           &setFillWidth,
                           singleSelectionAndInQtQuickLayout,
                           singleSelectionAndInQtQuickLayout));

        addDesignerAction(new FillHeightModelNodeAction
                          (layoutFillHeightDisplayName,
                           layoutCategory,
                           20,
                           &setFillHeight,
                           singleSelectionAndInQtQuickLayout,
                           singleSelectionAndInQtQuickLayout));

    addDesignerAction(new SeperatorDesignerAction(rootCategory, priorityTopLevelSeperator));
    addDesignerAction(new ModelNodeAction
               (goIntoComponentDisplayName, rootCategory, priorityGoIntoComponent, &goIntoComponent, &selectionIsComponent));
}

DesignerActionManager *DesignerActionManager::instance()
{
    if (!m_instance) {
        m_instance = new DesignerActionManager;
        createDefaultDesignerActions();
    }

    return m_instance;
}

void DesignerActionManager::addDesignerActionInternal(AbstractDesignerAction *newAction)
{
    m_designerActions.append(QSharedPointer<AbstractDesignerAction>(newAction));
    m_view->setDesignerActionList(designerActions());
}

QList<AbstractDesignerAction* > DesignerActionManager::factoriesInternal() const
{
    QList<AbstractDesignerAction* > list;
    foreach (const QSharedPointer<AbstractDesignerAction> &pointer, m_designerActions) {
        list.append(pointer.data());
    }

    return list;
}

DesignerActionManager::DesignerActionManager() : m_view(new Internal::DesignerActionManagerView)
{
}

DesignerActionManager::~DesignerActionManager()
{
    delete m_view;
}

} //QmlDesigner
