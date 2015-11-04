/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    QObject(parent),
    m_treeModel(0),
    m_modelTreeView(0)
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
        MObject *object = dynamic_cast<MObject *>(m_treeModel->element(index));
        if (object && !object->owner()) {
            return true;
        }
    }
    return false;
}

MObject *TreeModelManager::selectedObject() const
{
    MObject *object = 0;
    if (m_modelTreeView->currentSourceModelIndex().isValid()) {
        MElement *element = m_treeModel->element(m_modelTreeView->currentSourceModelIndex());
        if (element) {
            object = dynamic_cast<MObject *>(element);
        }
    }
    return object;
}

MPackage *TreeModelManager::selectedPackage() const
{
    if (m_modelTreeView->currentSourceModelIndex().isValid())
    {
        MElement *element = m_treeModel->element(m_modelTreeView->currentSourceModelIndex());
        QMT_CHECK(element);
        if (MPackage *package = dynamic_cast<MPackage *>(element)) {
            return package;
        } else if (MObject *object = dynamic_cast<MObject *>(element)) {
            package = dynamic_cast<MPackage *>(object->owner());
            if (package) {
                return package;
            }
        }
    }
    return m_treeModel->modelController()->rootPackage();
}

MSelection TreeModelManager::selectedObjects() const
{
    MSelection modelSelection;
    foreach (const QModelIndex &index, m_modelTreeView->selectedSourceModelIndexes()) {
        MElement *element = m_treeModel->element(index);
        if (MObject *object = dynamic_cast<MObject *>(element)) {
            modelSelection.append(object->uid(), m_treeModel->modelController()->ownerKey(object));
        } else if (MRelation *relation = dynamic_cast<MRelation *>(element)) {
            modelSelection.append(relation->uid(), m_treeModel->modelController()->ownerKey(relation));
        }
    }
    return modelSelection;
}

}
