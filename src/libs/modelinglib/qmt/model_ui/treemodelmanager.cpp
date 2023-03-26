// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    const QList<QModelIndex> indices = m_modelTreeView->selectedSourceModelIndexes();
    for (const QModelIndex &index : indices) {
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
    const QList<QModelIndex> indices = m_modelTreeView->selectedSourceModelIndexes();
    for (const QModelIndex &index : indices) {
        MElement *element = m_treeModel->element(index);
        if (auto object = dynamic_cast<MObject *>(element))
            modelSelection.append(object->uid(), m_treeModel->modelController()->ownerKey(object));
        else if (auto relation = dynamic_cast<MRelation *>(element))
            modelSelection.append(relation->uid(), m_treeModel->modelController()->ownerKey(relation));
    }
    return modelSelection;
}

} // namespace qmt
