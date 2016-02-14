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

#include "propertiesview.h"
#include "propertiesviewmview.h"

#include "qmt/model_controller/modelcontroller.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mrelation.h"
#include "qmt/model/mdiagram.h"
#include "qmt/diagram_controller/diagramcontroller.h"

#include "qmt/diagram/delement.h"

namespace qmt {

PropertiesView::PropertiesView(QObject *parent)
    : QObject(parent),
      m_modelController(0),
      m_diagramController(0),
      m_stereotypeController(0),
      m_styleController(0),
      m_viewFactory([=](PropertiesView *propertiesView) { return new MView(propertiesView); }),
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
        if (m_modelController)
            disconnect(m_modelController, 0, this, 0);
        m_modelController = modelController;
        if (m_modelController) {
            connect(m_modelController, &ModelController::beginResetModel,
                    this, &PropertiesView::onBeginResetModel);
            connect(m_modelController, &ModelController::endResetModel,
                    this, &PropertiesView::onEndResetModel);

            connect(m_modelController, &ModelController::beginInsertObject,
                    this, &PropertiesView::onBeginInsertObject);
            connect(m_modelController, &ModelController::endInsertObject,
                    this, &PropertiesView::onEndInsertObject);
            connect(m_modelController, &ModelController::beginUpdateObject,
                    this, &PropertiesView::onBeginUpdateObject);
            connect(m_modelController, &ModelController::endUpdateObject,
                    this, &PropertiesView::onEndUpdateObject);
            connect(m_modelController, &ModelController::beginRemoveObject,
                    this, &PropertiesView::onBeginRemoveObject);
            connect(m_modelController, &ModelController::endRemoveObject,
                    this, &PropertiesView::onEndRemoveObject);
            connect(m_modelController, &ModelController::beginMoveObject,
                    this, &PropertiesView::onBeginMoveObject);
            connect(m_modelController, &ModelController::endMoveObject,
                    this, &PropertiesView::onEndMoveObject);

            connect(m_modelController, &ModelController::beginInsertRelation,
                    this, &PropertiesView::onBeginInsertRelation);
            connect(m_modelController, &ModelController::endInsertRelation,
                    this, &PropertiesView::onEndInsertRelation);
            connect(m_modelController, &ModelController::beginUpdateRelation,
                    this, &PropertiesView::onBeginUpdateRelation);
            connect(m_modelController, &ModelController::endUpdateRelation,
                    this, &PropertiesView::onEndUpdateRelation);
            connect(m_modelController, &ModelController::beginRemoveRelation,
                    this, &PropertiesView::onBeginRemoveRelation);
            connect(m_modelController, &ModelController::endRemoveRelation,
                    this, &PropertiesView::onEndRemoveRelation);
            connect(m_modelController, &ModelController::beginMoveRelation,
                    this, &PropertiesView::onBeginMoveRelation);
            connect(m_modelController, &ModelController::endMoveRelation,
                    this, &PropertiesView::onEndMoveRelation);

            connect(m_modelController, &ModelController::relationEndChanged,
                    this, &PropertiesView::onRelationEndChanged);
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
            connect(m_diagramController, &DiagramController::beginResetAllDiagrams,
                    this, &PropertiesView::onBeginResetAllDiagrams);
            connect(m_diagramController, &DiagramController::endResetAllDiagrams,
                    this, &PropertiesView::onEndResetAllDiagrams);

            connect(m_diagramController, &DiagramController::beginResetDiagram,
                    this, &PropertiesView::onBeginResetDiagram);
            connect(m_diagramController, &DiagramController::endResetDiagram,
                    this, &PropertiesView::onEndResetDiagram);

            connect(m_diagramController, &DiagramController::beginUpdateElement,
                    this, &PropertiesView::onBeginUpdateElement);
            connect(m_diagramController, &DiagramController::endUpdateElement,
                    this, &PropertiesView::onEndUpdateElement);
            connect(m_diagramController, &DiagramController::beginInsertElement,
                    this, &PropertiesView::onBeginInsertElement);
            connect(m_diagramController, &DiagramController::endInsertElement,
                    this, &PropertiesView::onEndInsertElement);
            connect(m_diagramController, &DiagramController::beginRemoveElement,
                    this, &PropertiesView::onBeginRemoveElement);
            connect(m_diagramController, &DiagramController::endRemoveElement,
                    this, &PropertiesView::onEndRemoveElement);
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

void PropertiesView::setMViewFactory(std::function<MView *(PropertiesView *)> factory)
{
    m_viewFactory = factory;
}

void PropertiesView::setSelectedModelElements(const QList<MElement *> &modelElements)
{
    QMT_CHECK(modelElements.size() > 0);

    if (m_selectedModelElements != modelElements) {
        m_selectedModelElements = modelElements;
        m_selectedDiagramElements.clear();
        m_selectedDiagram = 0;
        m_mview.reset(m_viewFactory(this));
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
        m_mview.reset(m_viewFactory(this));
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
    if (m_selectedModelElements.size() == 1 || (m_selectedDiagramElements.size() == 1 && m_selectedDiagram))
        m_mview->edit();
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
    if (mobject && m_selectedModelElements.contains(mobject))
        m_mview->update(m_selectedModelElements);
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
    if (mobject && m_selectedModelElements.contains(mobject))
        clearSelection();
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
    if (mobject && m_selectedModelElements.contains(mobject))
        m_mview->update(m_selectedModelElements);
}

void PropertiesView::onBeginUpdateRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onEndUpdateRelation(int row, const MObject *parent)
{
    MRelation *mrelation = parent->relations().at(row);
    if (mrelation && m_selectedModelElements.contains(mrelation))
        m_mview->update(m_selectedModelElements);
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
    if (mrelation && m_selectedModelElements.contains(mrelation))
        clearSelection();
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
    if (mrelation && m_selectedModelElements.contains(mrelation))
        m_mview->update(m_selectedModelElements);
}

void PropertiesView::onRelationEndChanged(MRelation *relation, MObject *endObject)
{
    Q_UNUSED(endObject);
    if (relation && m_selectedModelElements.contains(relation))
        m_mview->update(m_selectedModelElements);
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
    if (diagram == m_selectedDiagram && m_selectedDiagramElements.size() > 0)
        m_mview->update(m_selectedDiagramElements, m_selectedDiagram);
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
        if (m_selectedDiagramElements.contains(delement))
            m_mview->update(m_selectedDiagramElements, m_selectedDiagram);
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
        if (m_selectedDiagramElements.contains(delement))
            clearSelection();
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

    if (auto object = dynamic_cast<MObject *>(modelElement)) {
        m_modelController->startUpdateObject(object);
    } else if (auto relation = dynamic_cast<MRelation *>(modelElement)) {
        m_modelController->startUpdateRelation(relation);
    } else {
        QMT_CHECK(false);
    }
}

void PropertiesView::endUpdate(MElement *modelElement, bool cancelled)
{
    QMT_CHECK(modelElement);

    if (auto object = dynamic_cast<MObject *>(modelElement)) {
        m_modelController->finishUpdateObject(object, cancelled);
    } else if (auto relation = dynamic_cast<MRelation *>(modelElement)) {
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

    m_diagramController->startUpdateElement(diagramElement, m_selectedDiagram, DiagramController::UpdateMinor);
}

void PropertiesView::endUpdate(DElement *diagramElement, bool cancelled)
{
    QMT_CHECK(diagramElement);
    QMT_CHECK(m_selectedDiagram != 0);
    QMT_CHECK(m_diagramController->findElement(diagramElement->uid(), m_selectedDiagram) == diagramElement);

    m_diagramController->finishUpdateElement(diagramElement, m_selectedDiagram, cancelled);
}

} // namespace qmt
