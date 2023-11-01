// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodecontextmenu_helper.h"

#include <bindingproperty.h>
#include <model/modelutils.h>
#include <modelnode.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>

#include <QFile>

namespace QmlDesigner {

static inline bool itemsHaveSameParent(const QList<ModelNode> &siblingList)
{
    if (siblingList.isEmpty())
        return false;


    const QmlItemNode &item = siblingList.constFirst();
    if (!item.isValid())
        return false;

    if (item.isRootModelNode())
        return false;

    QmlItemNode parent = item.instanceParent().toQmlItemNode();
    if (!parent.isValid())
        return false;

    for (const ModelNode &node : siblingList) {
        QmlItemNode currentItem(node);
        if (!currentItem.isValid())
            return false;
        QmlItemNode currentParent = currentItem.instanceParent().toQmlItemNode();
        if (!currentParent.isValid())
            return false;
        if (currentItem.instanceIsInLayoutable())
            return false;
        if (currentParent != parent)
            return false;
    }
    return true;
}

namespace SelectionContextFunctors {

bool singleSelectionItemIsAnchored(const SelectionContext &selectionState)
{
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool anchored = itemNode.instanceHasAnchors();
        return anchored;
    }
    return false;
}

bool singleSelectionItemIsNotAnchored(const SelectionContext &selectionState)
{
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool anchored = itemNode.instanceHasAnchors();
        return !anchored;
    }
    return false;
}

bool selectionHasSameParent(const SelectionContext &selectionState)
{
    return !selectionState.selectedModelNodes().isEmpty() && itemsHaveSameParent(selectionState.selectedModelNodes());
}

bool fileComponentExists(const ModelNode &modelNode)
{
    if (!modelNode.metaInfo().isFileComponent()) {
        return true;
    }

    const QString fileName = ModelUtils::componentFilePath(modelNode);

    if (fileName.contains("qml/QtQuick"))
        return false;

    return QFileInfo::exists(fileName);
}

bool selectionIsComponent(const SelectionContext &selectionState)
{
    return selectionState.currentSingleSelectedNode().isComponent()
           && fileComponentExists(selectionState.currentSingleSelectedNode());
}

bool selectionIsImported3DAsset(const SelectionContext &selectionState)
{
    ModelNode node = selectionState.currentSingleSelectedNode();
    if (selectionState.view() && node.hasMetaInfo()) {
        QString fileName = ModelUtils::componentFilePath(node);

        if (fileName.isEmpty()) {
            // Node is not a file component, so we have to check if the current doc itself is
            fileName = node.model()->fileUrl().toLocalFile();
        }
        if (fileName.contains(Constants::QUICK_3D_ASSETS_FOLDER))
            return true;
    }
    return false;
}

} //SelectionStateFunctors

} //QmlDesigner
