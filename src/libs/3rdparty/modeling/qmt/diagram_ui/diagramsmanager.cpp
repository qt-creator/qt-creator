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

class DiagramsManager::ManagedDiagram {
public:
    ManagedDiagram(DiagramSceneModel *diagram_scene_model, const QString &diagram_name);

    ~ManagedDiagram();

public:
    DiagramSceneModel *getDiagramSceneModel() const { return m_diagramSceneModel.data(); }

    QString getDiagramName() const { return m_diagramName; }

    void setDiagramName(const QString &name) { m_diagramName = name; }

private:
    QScopedPointer<DiagramSceneModel> m_diagramSceneModel;
    QString m_diagramName;
};

DiagramsManager::ManagedDiagram::ManagedDiagram(DiagramSceneModel *diagram_scene_model, const QString &diagram_name)
    : m_diagramSceneModel(diagram_scene_model),
      m_diagramName(diagram_name)
{
}

DiagramsManager::ManagedDiagram::~ManagedDiagram()
{
}



DiagramsManager::DiagramsManager(QObject *parent)
    : QObject(parent),
      m_diagramsView(0),
      m_diagramController(0),
      m_diagramSceneController(0),
      m_styleController(0),
      m_stereotypeController(0)
{
}

DiagramsManager::~DiagramsManager()
{
    qDeleteAll(m_diagramUidToManagedDiagramMap);
}

void DiagramsManager::setModel(TreeModel *model)
{
    if (m_model) {
        connect(m_model, 0, this, 0);
    }
    m_model = model;
    if (model) {
        connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(onDataChanged(QModelIndex,QModelIndex)));
    }
}

void DiagramsManager::setDiagramsView(DiagramsViewInterface *diagrams_view)
{
    m_diagramsView = diagrams_view;
}

void DiagramsManager::setDiagramController(DiagramController *diagram_controller)
{
    if (m_diagramController) {
        connect(m_diagramController, 0, this, 0);
    }
    m_diagramController = diagram_controller;
    if (diagram_controller) {
        connect(diagram_controller, SIGNAL(diagramAboutToBeRemoved(const MDiagram*)), this, SLOT(removeDiagram(const MDiagram*)));
    }
}

void DiagramsManager::setDiagramSceneController(DiagramSceneController *diagram_scene_controller)
{
    m_diagramSceneController = diagram_scene_controller;
}

void DiagramsManager::setStyleController(StyleController *style_controller)
{
    m_styleController = style_controller;
}

void DiagramsManager::setStereotypeController(StereotypeController *stereotype_controller)
{
    m_stereotypeController = stereotype_controller;
}

DiagramSceneModel *DiagramsManager::bindDiagramSceneModel(MDiagram *diagram)
{
    if (!m_diagramUidToManagedDiagramMap.contains(diagram->getUid())) {
        DiagramSceneModel *diagram_scene_model = new DiagramSceneModel();
        diagram_scene_model->setDiagramController(m_diagramController);
        diagram_scene_model->setDiagramSceneController(m_diagramSceneController);
        diagram_scene_model->setStyleController(m_styleController);
        diagram_scene_model->setStereotypeController(m_stereotypeController);
        diagram_scene_model->setDiagram(diagram);
        connect(diagram_scene_model, SIGNAL(diagramSceneActivated(const MDiagram*)), this, SIGNAL(diagramActivated(const MDiagram*)));
        connect(diagram_scene_model, SIGNAL(selectionChanged(const MDiagram*)), this, SIGNAL(diagramSelectionChanged(const MDiagram*)));
        ManagedDiagram *managed_diagram = new ManagedDiagram(diagram_scene_model, diagram->getName());
        m_diagramUidToManagedDiagramMap.insert(diagram->getUid(), managed_diagram);
    }
    return getDiagramSceneModel(diagram);
}

DiagramSceneModel *DiagramsManager::getDiagramSceneModel(const MDiagram *diagram) const
{
    const ManagedDiagram *managed_diagram = m_diagramUidToManagedDiagramMap.value(diagram->getUid());
    QMT_CHECK(managed_diagram);
    return managed_diagram->getDiagramSceneModel();
}

void DiagramsManager::unbindDiagramSceneModel(const MDiagram *diagram)
{
    QMT_CHECK(diagram);
    ManagedDiagram *managed_diagram = m_diagramUidToManagedDiagramMap.take(diagram->getUid());
    QMT_CHECK(managed_diagram);
    delete managed_diagram;
}

void DiagramsManager::openDiagram(MDiagram *diagram)
{
    if (m_diagramsView) {
        m_diagramsView->openDiagram(diagram);
    }
}

void DiagramsManager::removeDiagram(const MDiagram *diagram)
{
    if (diagram) {
        ManagedDiagram *managed_diagram = m_diagramUidToManagedDiagramMap.value(diagram->getUid());
        if (managed_diagram) {
            if (m_diagramsView) {
                // closeDiagram() must call unbindDiagramSceneModel()
                m_diagramsView->closeDiagram(diagram);
            }
        }
    }
}

void DiagramsManager::removeAllDiagrams()
{
    if (m_diagramsView) {
        m_diagramsView->closeAllDiagrams();
    }
    qDeleteAll(m_diagramUidToManagedDiagramMap);
    m_diagramUidToManagedDiagramMap.clear();

}

void DiagramsManager::onDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright)
{
    for (int row = topleft.row(); row <= bottomright.row(); ++row) {
        QModelIndex index = m_model->index(row, 0, topleft.parent());
        MDiagram *diagram = dynamic_cast<MDiagram *>(m_model->getElement(index));
        if (diagram) {
            ManagedDiagram *managed_diagram = m_diagramUidToManagedDiagramMap.value(diagram->getUid());
            if (managed_diagram && managed_diagram->getDiagramName() != diagram->getName()) {
                managed_diagram->setDiagramName(diagram->getName());
                if (m_diagramsView) {
                    m_diagramsView->onDiagramRenamed(diagram);
                }
            }
        }
    }
}

}
