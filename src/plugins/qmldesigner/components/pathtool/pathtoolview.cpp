// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pathtoolview.h"

#include <nodeproperty.h>
#include <variantproperty.h>
#include <modelnode.h>
#include <metainfo.h>

#include "pathtool.h"

#include <QtDebug>

namespace QmlDesigner {

PathToolView::PathToolView(PathTool *pathTool, ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_pathTool(pathTool)
{
}

static bool isInEditedPath(const NodeAbstractProperty &propertyParent, const ModelNode &editingPathViewModelNode)
{
    if (editingPathViewModelNode.hasNodeProperty("path")) {
        ModelNode pathModelNode = editingPathViewModelNode.nodeProperty("path").modelNode();
        if (pathModelNode.metaInfo().isQtQuickPath()) {
            if (propertyParent.name() == "pathElements"
                && propertyParent.parentModelNode() == pathModelNode)
                return true;
        }
    }

    return false;
}

void PathToolView::nodeReparented(const ModelNode & /*node*/,
                                  const NodeAbstractProperty & newPropertyParent,
                                  const NodeAbstractProperty & /*oldPropertyParent*/,
                                  AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isInEditedPath(newPropertyParent, m_pathTool->editingPathViewModelNode()))
        m_pathTool->pathChanged();
}

bool variantPropertyInEditedPath(const VariantProperty &variantProperty, const ModelNode &editingPathViewModelNode)
{
    ModelNode pathElementModelNode = variantProperty.parentModelNode();
    if (pathElementModelNode.hasParentProperty()) {
        if (isInEditedPath(pathElementModelNode.parentProperty(), editingPathViewModelNode))
            return true;
    }

    return false;
}

bool changesEditedPath(const QList<VariantProperty> &propertyList, const ModelNode &editingPathViewModelNode)
{
    for (const VariantProperty &variantProperty : propertyList) {
        if (variantPropertyInEditedPath(variantProperty, editingPathViewModelNode))
            return true;
    }

    return false;
}

void PathToolView::variantPropertiesChanged(const QList<VariantProperty> &propertyList, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (changesEditedPath(propertyList,  m_pathTool->editingPathViewModelNode()))
        m_pathTool->pathChanged();
}

} // namespace QmlDesigner
