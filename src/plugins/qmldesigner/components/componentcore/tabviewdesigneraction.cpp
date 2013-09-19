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

#include "tabviewdesigneraction.h"

#include <nodemetainfo.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>

#include <QCoreApplication>

namespace QmlDesigner {

bool isTabView(const ModelNode &modelNode)
{
    return modelNode.metaInfo().isSubclassOf("QtQuick.Controls.TabView", -1, -1);
}

bool isTabAndParentIsTabView(const ModelNode &modelNode)
{
    return modelNode.metaInfo().isSubclassOf("QtQuick.Controls.Tab", -1, -1)
            && modelNode.hasParentProperty()
            && modelNode.parentProperty().parentModelNode().metaInfo().isSubclassOf("QtQuick.Controls.TabView", -1, -1);
}

TabViewDesignerAction::TabViewDesignerAction()
    : DefaultDesignerAction(QCoreApplication::translate("TabViewToolAction","Edit Tabs"))
{
}

QByteArray TabViewDesignerAction::category() const
{
    return QByteArray();
}

QByteArray TabViewDesignerAction::menuId() const
{
    return "TabViewAction";
}

int TabViewDesignerAction::priority() const
{
    return CustomActionsPriority;
}

AbstractDesignerAction::Type TabViewDesignerAction::type() const
{
    return Action;
}

bool TabViewDesignerAction::isVisible(const SelectionContext &selectionContext) const
{
    if (selectionContext.scenePosition().isNull())
        return false;

    if (selectionContext.singleNodeIsSelected()) {
        ModelNode selectedModelNode = selectionContext.currentSingleSelectedNode();
        return isTabView(selectedModelNode) || isTabAndParentIsTabView(selectedModelNode);
    }

    return false;
}

bool TabViewDesignerAction::isEnabled(const SelectionContext &selectionContext) const
{
    return isVisible(selectionContext);
}

} // namespace QmlDesigner
