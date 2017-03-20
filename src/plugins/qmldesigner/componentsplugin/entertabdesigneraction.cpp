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
                    && selectedModelNode.metaInfo().isSubclassOf("QtQuick.Controls.TabView")) {

                const NodeAbstractProperty defaultProperty = selectedModelNode.defaultNodeAbstractProperty();
                foreach (const QmlDesigner::ModelNode &childModelNode, defaultProperty.directSubNodes()) {
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
        return selectedModelNode.metaInfo().isValid() && selectedModelNode.metaInfo().isTabView();
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
    if (modelNode.metaInfo().isValid()
            && modelNode.metaInfo().isSubclassOf("QtQuick.Controls.Tab")) {

        QmlDesigner::QmlItemNode itemNode(modelNode);

        if (itemNode.isValid()) {
            QString what = tr("Step into: %1").
                    arg(itemNode.instanceValue("title").toString());
            EnterTabAction *selectionAction = new EnterTabAction(what);

            SelectionContext nodeSelectionContext = selectionContext();
            nodeSelectionContext.setTargetNode(modelNode);
            selectionAction->setSelectionContext(nodeSelectionContext);

            menu()->addAction(selectionAction);

        }
    }
}

} // namespace QmlDesigner
