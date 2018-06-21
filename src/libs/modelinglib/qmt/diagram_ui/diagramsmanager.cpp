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

#include "diagramsmanager.h"

#include "diagramsviewinterface.h"

#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/model_ui/treemodel.h"
#include "qmt/model/mdiagram.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dselection.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <QModelIndex>

namespace qmt {

class DiagramsManager::ManagedDiagram
{
public:
    ManagedDiagram(DiagramSceneModel *diagramSceneModel, const QString &diagramName);
    ~ManagedDiagram();

    DiagramSceneModel *diagramSceneModel() const { return m_diagramSceneModel.data(); }
    QString diagramName() const { return m_diagramName; }
    void setDiagramName(const QString &name) { m_diagramName = name; }

private:
    QScopedPointer<DiagramSceneModel> m_diagramSceneModel;
    QString m_diagramName;
};

DiagramsManager::ManagedDiagram::ManagedDiagram(DiagramSceneModel *diagramSceneModel, const QString &diagramName)
    : m_diagramSceneModel(diagramSceneModel),
      m_diagramName(diagramName)
{
}

DiagramsManager::ManagedDiagram::~ManagedDiagram()
{
}

DiagramsManager::DiagramsManager(QObject *parent)
    : QObject(parent)
{
}

DiagramsManager::~DiagramsManager()
{
    qDeleteAll(m_diagramUidToManagedDiagramMap);
}

void DiagramsManager::setModel(TreeModel *model)
{
    if (m_model)
        connect(m_model, nullptr, this, nullptr);
    m_model = model;
    if (model) {
        connect(model, &QAbstractItemModel::dataChanged,
                this, &DiagramsManager::onDataChanged);
    }
}

void DiagramsManager::setDiagramsView(DiagramsViewInterface *diagramsView)
{
    m_diagramsView = diagramsView;
}

void DiagramsManager::setDiagramController(DiagramController *diagramController)
{
    if (m_diagramController)
        connect(m_diagramController, nullptr, this, nullptr);
    m_diagramController = diagramController;
    if (diagramController) {
        connect(diagramController, &DiagramController::diagramAboutToBeRemoved,
                this, &DiagramsManager::removeDiagram);
    }
}

void DiagramsManager::setDiagramSceneController(DiagramSceneController *diagramSceneController)
{
    m_diagramSceneController = diagramSceneController;
}

void DiagramsManager::setStyleController(StyleController *styleController)
{
    m_styleController = styleController;
}

void DiagramsManager::setStereotypeController(StereotypeController *stereotypeController)
{
    m_stereotypeController = stereotypeController;
}

DiagramSceneModel *DiagramsManager::bindDiagramSceneModel(MDiagram *diagram)
{
    if (!m_diagramUidToManagedDiagramMap.contains(diagram->uid())) {
        auto diagramSceneModel = new DiagramSceneModel();
        diagramSceneModel->setDiagramController(m_diagramController);
        diagramSceneModel->setDiagramSceneController(m_diagramSceneController);
        diagramSceneModel->setStyleController(m_styleController);
        diagramSceneModel->setStereotypeController(m_stereotypeController);
        diagramSceneModel->setDiagram(diagram);
        connect(diagramSceneModel, &DiagramSceneModel::diagramSceneActivated,
                this, &DiagramsManager::diagramActivated);
        connect(diagramSceneModel, &DiagramSceneModel::selectionHasChanged,
                this, &DiagramsManager::diagramSelectionChanged);
        auto managedDiagram = new ManagedDiagram(diagramSceneModel, diagram->name());
        m_diagramUidToManagedDiagramMap.insert(diagram->uid(), managedDiagram);
    }
    return diagramSceneModel(diagram);
}

DiagramSceneModel *DiagramsManager::diagramSceneModel(const MDiagram *diagram) const
{
    const ManagedDiagram *managedDiagram = m_diagramUidToManagedDiagramMap.value(diagram->uid());
    QMT_ASSERT(managedDiagram, return nullptr);
    return managedDiagram->diagramSceneModel();
}

void DiagramsManager::unbindDiagramSceneModel(const MDiagram *diagram)
{
    QMT_ASSERT(diagram, return);
    ManagedDiagram *managedDiagram = m_diagramUidToManagedDiagramMap.take(diagram->uid());
    QMT_CHECK(managedDiagram);
    delete managedDiagram;
}

void DiagramsManager::openDiagram(MDiagram *diagram)
{
    if (m_diagramsView)
        m_diagramsView->openDiagram(diagram);
}

void DiagramsManager::removeDiagram(const MDiagram *diagram)
{
    if (diagram) {
        ManagedDiagram *managedDiagram = m_diagramUidToManagedDiagramMap.value(diagram->uid());
        if (managedDiagram) {
            if (m_diagramsView) {
                // closeDiagram() must call unbindDiagramSceneModel()
                m_diagramsView->closeDiagram(diagram);
            }
        }
    }
}

void DiagramsManager::removeAllDiagrams()
{
    if (m_diagramsView)
        m_diagramsView->closeAllDiagrams();
    qDeleteAll(m_diagramUidToManagedDiagramMap);
    m_diagramUidToManagedDiagramMap.clear();

}

void DiagramsManager::onDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright)
{
    for (int row = topleft.row(); row <= bottomright.row(); ++row) {
        QModelIndex index = m_model->QStandardItemModel::index(row, 0, topleft.parent());
        auto diagram = dynamic_cast<MDiagram *>(m_model->element(index));
        if (diagram) {
            ManagedDiagram *managedDiagram = m_diagramUidToManagedDiagramMap.value(diagram->uid());
            if (managedDiagram && managedDiagram->diagramName() != diagram->name()) {
                managedDiagram->setDiagramName(diagram->name());
                if (m_diagramsView)
                    m_diagramsView->onDiagramRenamed(diagram);
            }
        }
    }
}

} // namespace qmt
