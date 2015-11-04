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

#include "propertiesview.h"
#include "propertiesviewmview.h"

#include "qmt/model_controller/modelcontroller.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mrelation.h"
#include "qmt/model/mdiagram.h"
#include "qmt/diagram_controller/diagramcontroller.h"

#include "qmt/diagram/delement.h"

#include <qdebug.h>


namespace qmt {

PropertiesView::PropertiesView(QObject *parent)
    : QObject(parent),
      m_modelController(0),
      m_diagramController(0),
      m_stereotypeController(0),
      m_styleController(0),
      m_selectedDiagram(0),
      m_widget(0)
{
}

PropertiesView::~PropertiesView()
{
}

void PropertiesView::setModelController(ModelController *modelController)
{
    if (m_modelController != modelController) {
        if (m_modelController) {
            disconnect(m_modelController, 0, this, 0);
        }
        m_modelController = modelController;
        if (m_modelController) {
            connect(m_modelController, SIGNAL(beginResetModel()), this, SLOT(onBeginResetModel()));
            connect(m_modelController, SIGNAL(endResetModel()), this, SLOT(onEndResetModel()));

            connect(m_modelController, SIGNAL(beginInsertObject(int,const MObject*)), this, SLOT(onBeginInsertObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(endInsertObject(int,const MObject*)), this, SLOT(onEndInsertObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginUpdateObject(int,const MObject*)), this, SLOT(onBeginUpdateObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(endUpdateObject(int,const MObject*)), this, SLOT(onEndUpdateObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginRemoveObject(int,const MObject*)), this, SLOT(onBeginRemoveObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(endRemoveObject(int,const MObject*)), this, SLOT(onEndRemoveObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginMoveObject(int,const MObject*)), this, SLOT(onBeginMoveObject(int,const MObject*)));
            connect(m_modelController, SIGNAL(endMoveObject(int,const MObject*)), this, SLOT(onEndMoveObject(int,const MObject*)));

            connect(m_modelController, SIGNAL(beginInsertRelation(int,const MObject*)), this, SLOT(onBeginInsertRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(endInsertRelation(int,const MObject*)), this, SLOT(onEndInsertRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginUpdateRelation(int,const MObject*)), this, SLOT(onBeginUpdateRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(endUpdateRelation(int,const MObject*)), this, SLOT(onEndUpdateRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginRemoveRelation(int,const MObject*)), this, SLOT(onBeginRemoveRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(endRemoveRelation(int,const MObject*)), this, SLOT(onEndRemoveRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(beginMoveRelation(int,const MObject*)), this, SLOT(onBeginMoveRelation(int,const MObject*)));
            connect(m_modelController, SIGNAL(endMoveRelation(int,const MObject*)), this, SLOT(onEndMoveRelation(int,const MObject*)));

            connect(m_modelController, SIGNAL(relationEndChanged(MRelation*,MObject*)), this, SLOT(onRelationEndChanged(MRelation*,MObject*)));
        }
    }
}

void PropertiesView::setDiagramController(DiagramController *diagramController)
{
    if (m_diagramController != diagramController) {
        if (m_diagramController) {
            disconnect(m_diagramController, 0, this, 0);
            m_diagramController = 0;
        }
        m_diagramController = diagramController;
        if (diagramController) {
            connect(m_diagramController, SIGNAL(beginResetAllDiagrams()), this, SLOT(onBeginResetAllDiagrams()));
            connect(m_diagramController, SIGNAL(endResetAllDiagrams()), this, SLOT(onEndResetAllDiagrams()));

            connect(m_diagramController, SIGNAL(beginResetDiagram(const MDiagram*)), this, SLOT(onBeginResetDiagram(const MDiagram*)));
            connect(m_diagramController, SIGNAL(endResetDiagram(const MDiagram*)), this, SLOT(onEndResetDiagram(const MDiagram*)));

            connect(m_diagramController, SIGNAL(beginUpdateElement(int,const MDiagram*)), this, SLOT(onBeginUpdateElement(int,const MDiagram*)));
            connect(m_diagramController, SIGNAL(endUpdateElement(int,const MDiagram*)), this, SLOT(onEndUpdateElement(int,const MDiagram*)));
            connect(m_diagramController, SIGNAL(beginInsertElement(int,const MDiagram*)), this, SLOT(onBeginInsertElement(int,const MDiagram*)));
            connect(m_diagramController, SIGNAL(endInsertElement(int,const MDiagram*)), this, SLOT(onEndInsertElement(int,const MDiagram*)));
            connect(m_diagramController, SIGNAL(beginRemoveElement(int,const MDiagram*)), this, SLOT(onBeginRemoveElement(int,const MDiagram*)));
            connect(m_diagramController, SIGNAL(endRemoveElement(int,const MDiagram*)), this, SLOT(onEndRemoveElement(int,const MDiagram*)));
        }
    }
}

void PropertiesView::setStereotypeController(StereotypeController *stereotypeController)
{
    m_stereotypeController = stereotypeController;
}

void PropertiesView::setStyleController(StyleController *styleController)
{
    m_styleController = styleController;
}

void PropertiesView::setSelectedModelElements(const QList<MElement *> &modelElements)
{
    QMT_CHECK(modelElements.size() > 0);

    if (m_selectedModelElements != modelElements) {
        m_selectedModelElements = modelElements;
        m_selectedDiagramElements.clear();
        m_selectedDiagram = 0;
        m_mview.reset(new MView(this));
        m_mview->update(m_selectedModelElements);
        m_widget = m_mview->topLevelWidget();
    }
}

void PropertiesView::setSelectedDiagramElements(const QList<DElement *> &diagramElements, MDiagram *diagram)
{
    QMT_CHECK(diagramElements.size() > 0);
    QMT_CHECK(diagram);

    if (m_selectedDiagramElements != diagramElements || m_selectedDiagram != diagram) {
        m_selectedDiagramElements = diagramElements;
        m_selectedDiagram = diagram;
        m_selectedModelElements.clear();
        m_mview.reset(new MView(this));
        m_mview->update(m_selectedDiagramElements, m_selectedDiagram);
        m_widget = m_mview->topLevelWidget();
    }
}

void PropertiesView::clearSelection()
{
    m_selectedModelElements.clear();
    m_selectedDiagramElements.clear();
    m_selectedDiagram = 0;
    m_mview.reset();
    m_widget = 0;
}

QWidget *PropertiesView::widget() const
{
    return m_widget;
}

void PropertiesView::editSelectedElement()
{
    if (m_selectedModelElements.size() == 1 || (m_selectedDiagramElements.size() == 1 && m_selectedDiagram)) {
        m_mview->edit();
    }
}

void PropertiesView::onBeginResetModel()
{
    clearSelection();
}

void PropertiesView::onEndResetModel()
{
}

void PropertiesView::onBeginUpdateObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onEndUpdateObject(int row, const MObject *parent)
{
    MObject *mobject = m_modelController->object(row, parent);
    if (mobject && m_selectedModelElements.contains(mobject)) {
        m_mview->update(m_selectedModelElements);
    }
}

void PropertiesView::onBeginInsertObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onEndInsertObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onBeginRemoveObject(int row, const MObject *parent)
{
    MObject *mobject = m_modelController->object(row, parent);
    if (mobject && m_selectedModelElements.contains(mobject)) {
        clearSelection();
    }
}

void PropertiesView::onEndRemoveObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onBeginMoveObject(int formerRow, const MObject *formerOwner)
{
    Q_UNUSED(formerRow);
    Q_UNUSED(formerOwner);
}

void PropertiesView::onEndMoveObject(int row, const MObject *owner)
{
    MObject *mobject = m_modelController->object(row, owner);
    if (mobject && m_selectedModelElements.contains(mobject)) {
        m_mview->update(m_selectedModelElements);
    }
}

void PropertiesView::onBeginUpdateRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onEndUpdateRelation(int row, const MObject *parent)
{
    MRelation *mrelation = parent->relations().at(row);
    if (mrelation && m_selectedModelElements.contains(mrelation)) {
        m_mview->update(m_selectedModelElements);
    }
}

void PropertiesView::onBeginInsertRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onEndInsertRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onBeginRemoveRelation(int row, const MObject *parent)
{
    MRelation *mrelation = parent->relations().at(row);
    if (mrelation && m_selectedModelElements.contains(mrelation)) {
        clearSelection();
    }
}

void PropertiesView::onEndRemoveRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onBeginMoveRelation(int formerRow, const MObject *formerOwner)
{
    Q_UNUSED(formerRow);
    Q_UNUSED(formerOwner);
}

void PropertiesView::onEndMoveRelation(int row, const MObject *owner)
{
    MRelation *mrelation = owner->relations().at(row);
    if (mrelation && m_selectedModelElements.contains(mrelation)) {
        m_mview->update(m_selectedModelElements);
    }
}

void PropertiesView::onRelationEndChanged(MRelation *relation, MObject *endObject)
{
    Q_UNUSED(endObject);
    if (relation && m_selectedModelElements.contains(relation)) {
        m_mview->update(m_selectedModelElements);
    }
}

void PropertiesView::onBeginResetAllDiagrams()
{
    clearSelection();
}

void PropertiesView::onEndResetAllDiagrams()
{
}

void PropertiesView::onBeginResetDiagram(const MDiagram *diagram)
{
    Q_UNUSED(diagram);
}

void PropertiesView::onEndResetDiagram(const MDiagram *diagram)
{
    if (diagram == m_selectedDiagram && m_selectedDiagramElements.size() > 0) {
        m_mview->update(m_selectedDiagramElements, m_selectedDiagram);
    }
}

void PropertiesView::onBeginUpdateElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
}

void PropertiesView::onEndUpdateElement(int row, const MDiagram *diagram)
{
    if (diagram == m_selectedDiagram) {
        DElement *delement = diagram->diagramElements().at(row);
        if (m_selectedDiagramElements.contains(delement)) {
            m_mview->update(m_selectedDiagramElements, m_selectedDiagram);
        }
    }
}

void PropertiesView::onBeginInsertElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
}

void PropertiesView::onEndInsertElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
}

void PropertiesView::onBeginRemoveElement(int row, const MDiagram *diagram)
{
    if (diagram == m_selectedDiagram) {
        DElement *delement = diagram->diagramElements().at(row);
        if (m_selectedDiagramElements.contains(delement)) {
            clearSelection();
        }
    }
}

void PropertiesView::onEndRemoveElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
}

void PropertiesView::beginUpdate(MElement *modelElement)
{
    QMT_CHECK(modelElement);

    if (MObject *object = dynamic_cast<MObject *>(modelElement)) {
        m_modelController->startUpdateObject(object);
    } else if (MRelation *relation = dynamic_cast<MRelation *>(modelElement)) {
        m_modelController->startUpdateRelation(relation);
    } else {
        QMT_CHECK(false);
    }
}

void PropertiesView::endUpdate(MElement *modelElement, bool cancelled)
{
    QMT_CHECK(modelElement);

    if (MObject *object = dynamic_cast<MObject *>(modelElement)) {
        m_modelController->finishUpdateObject(object, cancelled);
    } else if (MRelation *relation = dynamic_cast<MRelation *>(modelElement)) {
        m_modelController->finishUpdateRelation(relation, cancelled);
    } else {
        QMT_CHECK(false);
    }
}

void PropertiesView::beginUpdate(DElement *diagramElement)
{
    QMT_CHECK(diagramElement);
    QMT_CHECK(m_selectedDiagram != 0);
    QMT_CHECK(m_diagramController->findElement(diagramElement->uid(), m_selectedDiagram) == diagramElement);

    m_diagramController->startUpdateElement(diagramElement, m_selectedDiagram, DiagramController::UPDATE_MINOR);
}

void PropertiesView::endUpdate(DElement *diagramElement, bool cancelled)
{
    QMT_CHECK(diagramElement);
    QMT_CHECK(m_selectedDiagram != 0);
    QMT_CHECK(m_diagramController->findElement(diagramElement->uid(), m_selectedDiagram) == diagramElement);

    m_diagramController->finishUpdateElement(diagramElement, m_selectedDiagram, cancelled);
}

}
