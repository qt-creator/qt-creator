/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "treemodelmanager.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mrelation.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model_controller/mselection.h"
#include "qmt/model_ui/treemodel.h"
#include "qmt/model_ui/modeltreeviewinterface.h"

#include <QItemSelection>

namespace qmt {

TreeModelManager::TreeModelManager(QObject *parent) :
    QObject(parent)
{
}

TreeModelManager::~TreeModelManager()
{
}

void TreeModelManager::setTreeModel(TreeModel *treeModel)
{
    m_treeModel = treeModel;
}

void TreeModelManager::setModelTreeView(ModelTreeViewInterface *modelTreeView)
{
    m_modelTreeView = modelTreeView;
}

bool TreeModelManager::isRootPackageSelected() const
{
    foreach (const QModelIndex &index, m_modelTreeView->selectedSourceModelIndexes()) {
        auto object = dynamic_cast<MObject *>(m_treeModel->element(index));
        if (object && !object->owner())
            return true;
    }
    return false;
}

MObject *TreeModelManager::selectedObject() const
{
    MObject *object = nullptr;
    if (m_modelTreeView->currentSourceModelIndex().isValid()) {
        MElement *element = m_treeModel->element(m_modelTreeView->currentSourceModelIndex());
        if (element)
            object = dynamic_cast<MObject *>(element);
    }
    return object;
}

MPackage *TreeModelManager::selectedPackage() const
{
    if (m_modelTreeView->currentSourceModelIndex().isValid())
    {
        MElement *element = m_treeModel->element(m_modelTreeView->currentSourceModelIndex());
        QMT_ASSERT(element, return nullptr);
        if (auto package = dynamic_cast<MPackage *>(element)) {
            return package;
        } else if (auto object = dynamic_cast<MObject *>(element)) {
            package = dynamic_cast<MPackage *>(object->owner());
            if (package)
                return package;
        }
    }
    return m_treeModel->modelController()->rootPackage();
}

MSelection TreeModelManager::selectedObjects() const
{
    MSelection modelSelection;
    foreach (const QModelIndex &index, m_modelTreeView->selectedSourceModelIndexes()) {
        MElement *element = m_treeModel->element(index);
        if (auto object = dynamic_cast<MObject *>(element))
            modelSelection.append(object->uid(), m_treeModel->modelController()->ownerKey(object));
        else if (auto relation = dynamic_cast<MRelation *>(element))
            modelSelection.append(relation->uid(), m_treeModel->modelController()->ownerKey(relation));
    }
    return modelSelection;
}

} // namespace qmt
