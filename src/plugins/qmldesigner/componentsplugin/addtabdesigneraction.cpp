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

#include "addtabdesigneraction.h"
#include "addtabtotabviewdialog.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QUrl>
#include <QFileInfo>
#include <coreplugin/icore.h>

#include <coreplugin/messagebox.h>

#include <documentmanager.h>
#include <nodemetainfo.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

bool isTabView(const ModelNode &modelNode)
{
    return modelNode.metaInfo().isSubclassOf("QtQuick.Controls.TabView");
}

bool isTabAndParentIsTabView(const ModelNode &modelNode)
{
    return modelNode.metaInfo().isSubclassOf("QtQuick.Controls.Tab")
            && modelNode.hasParentProperty()
            && modelNode.parentProperty().parentModelNode().metaInfo().isSubclassOf("QtQuick.Controls.TabView");
}

AddTabDesignerAction::AddTabDesignerAction()
    : AbstractAction(QCoreApplication::translate("TabViewToolAction","Add Tab..."))
{
    connect(action(), SIGNAL(triggered()), this, SLOT(addNewTab()));
}

QByteArray AddTabDesignerAction::category() const
{
    return QByteArray();
}

QByteArray AddTabDesignerAction::menuId() const
{
    return "TabViewAction";
}

int AddTabDesignerAction::priority() const
{
    return CustomActionsPriority;
}

ActionInterface::Type AddTabDesignerAction::type() const
{
    return ContextMenuAction;
}

bool AddTabDesignerAction::isVisible(const SelectionContext &selectionContext) const
{
    if (selectionContext.singleNodeIsSelected()) {
        ModelNode selectedModelNode = selectionContext.currentSingleSelectedNode();
        return isTabView(selectedModelNode) || isTabAndParentIsTabView(selectedModelNode);
    }

    return false;
}

bool AddTabDesignerAction::isEnabled(const SelectionContext &selectionContext) const
{
    return isVisible(selectionContext);
}

static ModelNode findTabViewModelNode(const ModelNode &currentModelNode)
{
    if (currentModelNode.metaInfo().isSubclassOf("QtQuick.Controls.TabView"))
        return currentModelNode;
    else
        return findTabViewModelNode(currentModelNode.parentProperty().parentModelNode());
}

void AddTabDesignerAction::addNewTab()
{
    QString tabName = AddTabToTabViewDialog::create(QStringLiteral("Tab"), Core::ICore::mainWindow());

    if (!tabName.isEmpty()) {
        QString directoryPath = QFileInfo(selectionContext().view()->model()->fileUrl().toLocalFile()).absolutePath();
        QString newFilePath = directoryPath +QStringLiteral("/") + tabName + QStringLiteral(".qml");

        if (QFileInfo::exists(newFilePath)) {
           Core::AsynchronousMessageBox::warning(tr("Naming Error"), tr("Component already exists."));
        } else {
            const QString sourceString = QStringLiteral("import QtQuick 2.1\nimport QtQuick.Controls 1.0\n\nItem {\n    anchors.fill: parent\n}");
            bool fileCreated = DocumentManager::createFile(newFilePath, sourceString);

            if (fileCreated) {
                DocumentManager::addFileToVersionControl(directoryPath, newFilePath);

                ModelNode tabViewModelNode = findTabViewModelNode(selectionContext().currentSingleSelectedNode());

                PropertyListType propertyList;
                propertyList.append(QPair<PropertyName, QVariant>("source", QString(tabName + QStringLiteral(".qml"))));
                propertyList.append(QPair<PropertyName, QVariant>("title", tabName));

                ModelNode newTabModelNode = selectionContext().view()->createModelNode("QtQuick.Controls.Tab",
                                                                                       tabViewModelNode.majorVersion(),
                                                                                       tabViewModelNode.minorVersion(),
                                                                                       propertyList);
                newTabModelNode.setIdWithRefactoring(newTabModelNode.view()->generateNewId(tabName));
                tabViewModelNode.defaultNodeAbstractProperty().reparentHere(newTabModelNode);
            }
        }
    }
}

} // namespace QmlDesigner
