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
#include "modelnodeoperations.h"
#include "componentcore_constants.h"
#include <nodeproperty.h>

#include <QMenu>

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

    void modelAttached(Model *model)
    {
        QmlModelView::modelAttached(model);
        setupContext();
    }

    void modelAboutToBeDetached(Model *model)
    {
        QmlModelView::modelAboutToBeDetached(model);
        setupContext();
    }

    virtual void nodeCreated(const ModelNode &)
    {
        setupContext();
    }

    virtual void nodeAboutToBeRemoved(const ModelNode &)
    {}

    virtual void nodeRemoved(const ModelNode &, const NodeAbstractProperty &, PropertyChangeFlags)
    {
        setupContext();
    }

    virtual void nodeAboutToBeReparented(const ModelNode &,
                                         const NodeAbstractProperty &,
                                         const NodeAbstractProperty &,
                                         AbstractView::PropertyChangeFlags )
    {
        setupContext();
    }

    virtual void nodeReparented(const ModelNode &, const NodeAbstractProperty &,
                                const NodeAbstractProperty &,
                                AbstractView::PropertyChangeFlags)
    {
        setupContext();
    }

    virtual void nodeIdChanged(const ModelNode&, const QString&, const QString&)
    {}

    virtual void propertiesAboutToBeRemoved(const QList<AbstractProperty>&)
    {}

    virtual void propertiesRemoved(const QList<AbstractProperty>&)
    {
        setupContext();
    }

    virtual void variantPropertiesChanged(const QList<VariantProperty>&, PropertyChangeFlags)
    {}

    virtual void bindingPropertiesChanged(const QList<BindingProperty>&, PropertyChangeFlags)
    {}

    virtual void rootNodeTypeChanged(const QString &, int , int )
    {
        setupContext();
    }

    virtual void instancePropertyChange(const QList<QPair<ModelNode, QString> > &)
    {}

    virtual void instancesCompleted(const QVector<ModelNode> &)
    {}

    virtual void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &)
    {}

    virtual void instancesRenderImageChanged(const QVector<ModelNode> &)
    {}

    virtual void instancesPreviewImageChanged(const QVector<ModelNode> &)
    {}

    virtual void instancesChildrenChanged(const QVector<ModelNode> &)
    {}

    virtual void instancesToken(const QString &, int , const QVector<ModelNode> &)
    {}

    virtual void nodeSourceChanged(const ModelNode &, const QString &)
    {}

    virtual void rewriterBeginTransaction()
    {
        m_isInRewriterTransaction = true;
    }

    virtual void rewriterEndTransaction()
    {
        m_isInRewriterTransaction = false;

        if (m_setupContextDirty)
            setupContext();
    }

    virtual void actualStateChanged(const ModelNode &)
    {
        setupContext();
    }

    virtual void selectedNodesChanged(const QList<ModelNode> &,
                                      const QList<ModelNode> &)
    {
        setupContext();
    }

    virtual void nodeOrderChanged(const NodeListProperty &, const ModelNode &, int )
    {
        setupContext();
    }

    virtual void importsChanged(const QList<Import> &, const QList<Import> &)
    {
        setupContext();
    }

    virtual void scriptFunctionsChanged(const ModelNode &, const QStringList &)
    {}

    void setDesignerActionList(const QList<AbstractDesignerAction* > &designerActionList)
    {
        m_designerActionList = designerActionList;
    }

    QWidget *widget()
    {
        return 0;
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
            action->setCurrentContext(selectionContext);
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
    return instance()->m_view.data();
}

class VisiblityModelNodeAction : public ModelNodeAction
{
public:
    VisiblityModelNodeAction(const QString &description, const QString &category, int priority,
            ModelNodeOperations::SelectionAction action,
            SelectionContextFunction enabled = &SelectionContextFunctors::always,
            SelectionContextFunction visibility = &SelectionContextFunctors::always) :
        ModelNodeAction(description, category, priority, action, enabled, visibility)
    {}
    virtual void updateContext()
    {
        m_action->setSelectionContext(m_selectionContext);
        if (m_selectionContext.isValid()) {
            m_action->setEnabled(isEnabled(m_selectionContext));
            m_action->setVisible(isVisible(m_selectionContext));

            m_action->setCheckable(true);
            QmlItemNode itemNode = QmlItemNode(m_selectionContext.currentSingleSelectedNode());
            if (itemNode.isValid())
                m_action->setChecked(itemNode.instanceValue("visible").toBool());
            else
                m_action->setEnabled(false);
        }
    }
};

class SelectionModelNodeAction : public MenuDesignerAction
{
public:
    SelectionModelNodeAction(const QString &displayName, const QString &menuId, int priority) :
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
            if (m_selectionContext.singleSelected() && !m_selectionContext.currentSingleSelectedNode().isRootNode()) {
                ActionTemplate *selectionAction = new ActionTemplate(QString(), &ModelNodeOperations::select);
                selectionAction->setParent(m_menu.data());

                parentNode = m_selectionContext.currentSingleSelectedNode().parentProperty().parentModelNode();
                m_selectionContext.setTargetNode(parentNode);
                selectionAction->setText(QString(QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select parent: %1")).arg(
                                             captionForModelNode(parentNode)));
                selectionAction->setSelectionContext(m_selectionContext);

                m_menu->addAction(selectionAction);
            }
            foreach (const ModelNode &node, m_selectionContext.view()->allModelNodes()) {
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
    return !singleSelection(context);
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

bool selectionCanBeLayouted(const SelectionContext &context)
{
    return selectionHasSameParentAndInBaseState(context) && inBaseState(context);
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

void DesignerActionManager::createDefaultDesignerActions()
{
    using namespace SelectionContextFunctors;
    using namespace ComponentCoreConstants;
    using namespace ModelNodeOperations;

    addDesignerAction(new SelectionModelNodeAction(selectionCategoryDisplayName, selectionCategory, prioritySelectionCategory));

    addDesignerAction(new MenuDesignerAction(stackCategoryDisplayName, stackCategory, priorityStackCategory, &always));
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
                    priorityLayoutCategory, &multiSelectionAndInBaseState));
        addDesignerAction(new ModelNodeAction
                   (layoutRowDisplayName, layoutCategory, 200, &layoutRow, &selectionCanBeLayouted));
        addDesignerAction(new ModelNodeAction
                   (layoutColumnDisplayName, layoutCategory, 180, &layoutColumn, &selectionCanBeLayouted));
        addDesignerAction(new ModelNodeAction
                   (layoutGridDisplayName, layoutCategory, 160, &layoutGrid, &selectionCanBeLayouted));
        addDesignerAction(new ModelNodeAction
                   (layoutFlowDisplayName, layoutCategory, 140, &layoutFlow, &selectionCanBeLayouted));

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

} //QmlDesigner
