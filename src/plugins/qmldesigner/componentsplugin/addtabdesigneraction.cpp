// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    return modelNode.metaInfo().isQtQuickControlsTabView();
}

bool isTabAndParentIsTabView(const ModelNode &modelNode)
{
    return modelNode.metaInfo().isQtQuickControlsTab() && modelNode.hasParentProperty()
           && modelNode.parentProperty().parentModelNode().metaInfo().isQtQuickControlsTabView();
}

AddTabDesignerAction::AddTabDesignerAction()
    : AbstractAction(QCoreApplication::translate("TabViewToolAction","Add Tab..."))
{
    connect(action(), &QAction::triggered, this, &AddTabDesignerAction::addNewTab);
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
    if (currentModelNode.metaInfo().isQtQuickControlsTabView())
        return currentModelNode;
    else
        return findTabViewModelNode(currentModelNode.parentProperty().parentModelNode());
}

void AddTabDesignerAction::addNewTab()
{
    QString tabName = AddTabToTabViewDialog::create(QStringLiteral("Tab"),
                                                    Core::ICore::dialogParent());

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
                newTabModelNode.setIdWithRefactoring(newTabModelNode.model()->generateNewId(tabName));
                tabViewModelNode.defaultNodeAbstractProperty().reparentHere(newTabModelNode);
            }
        }
    }
}

} // namespace QmlDesigner
