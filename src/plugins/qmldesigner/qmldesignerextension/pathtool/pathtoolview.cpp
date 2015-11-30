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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "pathtoolview.h"

#include <nodeproperty.h>
#include <variantproperty.h>
#include <modelnode.h>
#include <metainfo.h>

#include "pathtool.h"

#include <QtDebug>

namespace QmlDesigner {

PathToolView::PathToolView(PathTool *pathTool)
    : AbstractView(),
      m_pathTool(pathTool)
{
}

static bool isInEditedPath(const NodeAbstractProperty &propertyParent, const ModelNode &editingPathViewModelNode)
{
    if (editingPathViewModelNode.isValid()) {
        if (editingPathViewModelNode.hasNodeProperty("path")) {
            ModelNode pathModelNode = editingPathViewModelNode.nodeProperty("path").modelNode();
            if (pathModelNode.metaInfo().isSubclassOf("QtQuick.Path", -1, -1)) {
                if (propertyParent.name() == "pathElements" && propertyParent.parentModelNode() == pathModelNode)
                    return true;
            }
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
    foreach (const VariantProperty variantProperty, propertyList) {
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
