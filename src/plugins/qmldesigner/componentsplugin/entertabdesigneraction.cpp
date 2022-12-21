// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "entertabdesigneraction.h"

#include <abstractaction.h>
#include <documentmanager.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>
#include <nodemetainfo.h>
#include <qmlitemnode.h>

#include <QCoreApplication>

namespace QmlDesigner {

class EnterTabAction : public DefaultAction
{

public:
    EnterTabAction(const QString &description)
        : DefaultAction(description)
    { }

public:
    void actionTriggered(bool) override
    {
        DocumentManager::goIntoComponent(m_selectionContext.targetNode());
    }
};

EnterTabDesignerAction::EnterTabDesignerAction()
 : AbstractActionGroup(QCoreApplication::translate("TabViewToolAction", "Step into Tab"))
{
}

QByteArray EnterTabDesignerAction::category() const
{
    return QByteArray();
}

QByteArray EnterTabDesignerAction::menuId() const
{
    return "TabViewAction";
}

int EnterTabDesignerAction::priority() const
{
    //Editing tabs is above adding tabs
    return CustomActionsPriority + 10;
}

void EnterTabDesignerAction::updateContext()
{
    menu()->clear();
    if (selectionContext().isValid()) {

        action()->setEnabled(isEnabled(selectionContext()));
        action()->setVisible(isVisible(selectionContext()));

        if (action()->isEnabled()) {
            const ModelNode selectedModelNode = selectionContext().currentSingleSelectedNode();
            if (selectedModelNode.metaInfo().isValid()
                && selectedModelNode.metaInfo().isQtQuickControlsTabView()) {
                const NodeAbstractProperty defaultProperty = selectedModelNode
                                                                 .defaultNodeAbstractProperty();
                const QList<QmlDesigner::ModelNode> childModelNodes = defaultProperty.directSubNodes();
                for (const QmlDesigner::ModelNode &childModelNode : childModelNodes) {
                    createActionForTab(childModelNode);
                }
            }
        }
    }
}

bool EnterTabDesignerAction::isVisible(const SelectionContext &selectionContext) const
{
    if (selectionContext.singleNodeIsSelected()) {
        ModelNode selectedModelNode = selectionContext.currentSingleSelectedNode();
        return selectedModelNode.metaInfo().isQtQuickControlsTabView();
    }

    return false;
}

static bool tabViewIsNotEmpty(const SelectionContext &selectionContext)
{
    return  selectionContext.currentSingleSelectedNode().defaultNodeAbstractProperty().isNodeListProperty();
}

bool EnterTabDesignerAction::isEnabled(const SelectionContext &selectionContext) const
{
    return isVisible(selectionContext) && tabViewIsNotEmpty(selectionContext);
}

void EnterTabDesignerAction::createActionForTab(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isQtQuickControlsTab()) {
        QmlDesigner::QmlItemNode itemNode(modelNode);

        if (itemNode.isValid()) {
            QString what = tr("Step into: %1").
                    arg(itemNode.instanceValue("title").toString());
            auto selectionAction = new EnterTabAction(what);

            SelectionContext nodeSelectionContext = selectionContext();
            nodeSelectionContext.setTargetNode(modelNode);
            selectionAction->setSelectionContext(nodeSelectionContext);

            menu()->addAction(selectionAction);

        }
    }
}

} // namespace QmlDesigner
