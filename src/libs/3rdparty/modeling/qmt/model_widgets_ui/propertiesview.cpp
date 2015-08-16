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
      _model_controller(0),
      _diagram_controller(0),
      _stereotype_controller(0),
      _style_controller(0),
      _selected_diagram(0),
      _widget(0)
{
}

PropertiesView::~PropertiesView()
{
}

void PropertiesView::setModelController(ModelController *model_controller)
{
    if (_model_controller != model_controller) {
        if (_model_controller) {
            disconnect(_model_controller, 0, this, 0);
        }
        _model_controller = model_controller;
        if (_model_controller) {
            connect(_model_controller, SIGNAL(beginResetModel()), this, SLOT(onBeginResetModel()));
            connect(_model_controller, SIGNAL(endResetModel()), this, SLOT(onEndResetModel()));

            connect(_model_controller, SIGNAL(beginInsertObject(int,const MObject*)), this, SLOT(onBeginInsertObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(endInsertObject(int,const MObject*)), this, SLOT(onEndInsertObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginUpdateObject(int,const MObject*)), this, SLOT(onBeginUpdateObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(endUpdateObject(int,const MObject*)), this, SLOT(onEndUpdateObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginRemoveObject(int,const MObject*)), this, SLOT(onBeginRemoveObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(endRemoveObject(int,const MObject*)), this, SLOT(onEndRemoveObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginMoveObject(int,const MObject*)), this, SLOT(onBeginMoveObject(int,const MObject*)));
            connect(_model_controller, SIGNAL(endMoveObject(int,const MObject*)), this, SLOT(onEndMoveObject(int,const MObject*)));

            connect(_model_controller, SIGNAL(beginInsertRelation(int,const MObject*)), this, SLOT(onBeginInsertRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(endInsertRelation(int,const MObject*)), this, SLOT(onEndInsertRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginUpdateRelation(int,const MObject*)), this, SLOT(onBeginUpdateRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(endUpdateRelation(int,const MObject*)), this, SLOT(onEndUpdateRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginRemoveRelation(int,const MObject*)), this, SLOT(onBeginRemoveRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(endRemoveRelation(int,const MObject*)), this, SLOT(onEndRemoveRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(beginMoveRelation(int,const MObject*)), this, SLOT(onBeginMoveRelation(int,const MObject*)));
            connect(_model_controller, SIGNAL(endMoveRelation(int,const MObject*)), this, SLOT(onEndMoveRelation(int,const MObject*)));

            connect(_model_controller, SIGNAL(relationEndChanged(MRelation*,MObject*)), this, SLOT(onRelationEndChanged(MRelation*,MObject*)));
        }
    }
}

void PropertiesView::setDiagramController(DiagramController *diagram_controller)
{
    if (_diagram_controller != diagram_controller) {
        if (_diagram_controller) {
            disconnect(_diagram_controller, 0, this, 0);
            _diagram_controller = 0;
        }
        _diagram_controller = diagram_controller;
        if (diagram_controller) {
            connect(_diagram_controller, SIGNAL(beginResetAllDiagrams()), this, SLOT(onBeginResetAllDiagrams()));
            connect(_diagram_controller, SIGNAL(endResetAllDiagrams()), this, SLOT(onEndResetAllDiagrams()));

            connect(_diagram_controller, SIGNAL(beginResetDiagram(const MDiagram*)), this, SLOT(onBeginResetDiagram(const MDiagram*)));
            connect(_diagram_controller, SIGNAL(endResetDiagram(const MDiagram*)), this, SLOT(onEndResetDiagram(const MDiagram*)));

            connect(_diagram_controller, SIGNAL(beginUpdateElement(int,const MDiagram*)), this, SLOT(onBeginUpdateElement(int,const MDiagram*)));
            connect(_diagram_controller, SIGNAL(endUpdateElement(int,const MDiagram*)), this, SLOT(onEndUpdateElement(int,const MDiagram*)));
            connect(_diagram_controller, SIGNAL(beginInsertElement(int,const MDiagram*)), this, SLOT(onBeginInsertElement(int,const MDiagram*)));
            connect(_diagram_controller, SIGNAL(endInsertElement(int,const MDiagram*)), this, SLOT(onEndInsertElement(int,const MDiagram*)));
            connect(_diagram_controller, SIGNAL(beginRemoveElement(int,const MDiagram*)), this, SLOT(onBeginRemoveElement(int,const MDiagram*)));
            connect(_diagram_controller, SIGNAL(endRemoveElement(int,const MDiagram*)), this, SLOT(onEndRemoveElement(int,const MDiagram*)));
        }
    }
}

void PropertiesView::setStereotypeController(StereotypeController *stereotype_controller)
{
    _stereotype_controller = stereotype_controller;
}

void PropertiesView::setStyleController(StyleController *style_controller)
{
    _style_controller = style_controller;
}

void PropertiesView::setSelectedModelElements(const QList<MElement *> &model_elements)
{
    QMT_CHECK(model_elements.size() > 0);

    if (_selected_model_elements != model_elements) {
        _selected_model_elements = model_elements;
        _selected_diagram_elements.clear();
        _selected_diagram = 0;
        _mview.reset(new MView(this));
        _mview->update(_selected_model_elements);
        _widget = _mview->getTopLevelWidget();
    }
}

void PropertiesView::setSelectedDiagramElements(const QList<DElement *> &diagram_elements, MDiagram *diagram)
{
    QMT_CHECK(diagram_elements.size() > 0);
    QMT_CHECK(diagram);

    if (_selected_diagram_elements != diagram_elements || _selected_diagram != diagram) {
        _selected_diagram_elements = diagram_elements;
        _selected_diagram = diagram;
        _selected_model_elements.clear();
        _mview.reset(new MView(this));
        _mview->update(_selected_diagram_elements, _selected_diagram);
        _widget = _mview->getTopLevelWidget();
    }
}

void PropertiesView::clearSelection()
{
    _selected_model_elements.clear();
    _selected_diagram_elements.clear();
    _selected_diagram = 0;
    _mview.reset();
    _widget = 0;
}

QWidget *PropertiesView::getWidget() const
{
    return _widget;
}

void PropertiesView::editSelectedElement()
{
    if (_selected_model_elements.size() == 1 || (_selected_diagram_elements.size() == 1 && _selected_diagram)) {
        _mview->edit();
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
    MObject *mobject = _model_controller->getObject(row, parent);
    if (mobject && _selected_model_elements.contains(mobject)) {
        _mview->update(_selected_model_elements);
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
    MObject *mobject = _model_controller->getObject(row, parent);
    if (mobject && _selected_model_elements.contains(mobject)) {
        clearSelection();
    }
}

void PropertiesView::onEndRemoveObject(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onBeginMoveObject(int former_row, const MObject *former_owner)
{
    Q_UNUSED(former_row);
    Q_UNUSED(former_owner);
}

void PropertiesView::onEndMoveObject(int row, const MObject *owner)
{
    MObject *mobject = _model_controller->getObject(row, owner);
    if (mobject && _selected_model_elements.contains(mobject)) {
        _mview->update(_selected_model_elements);
    }
}

void PropertiesView::onBeginUpdateRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onEndUpdateRelation(int row, const MObject *parent)
{
    MRelation *mrelation = parent->getRelations().at(row);
    if (mrelation && _selected_model_elements.contains(mrelation)) {
        _mview->update(_selected_model_elements);
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
    MRelation *mrelation = parent->getRelations().at(row);
    if (mrelation && _selected_model_elements.contains(mrelation)) {
        clearSelection();
    }
}

void PropertiesView::onEndRemoveRelation(int row, const MObject *parent)
{
    Q_UNUSED(row);
    Q_UNUSED(parent);
}

void PropertiesView::onBeginMoveRelation(int former_row, const MObject *former_owner)
{
    Q_UNUSED(former_row);
    Q_UNUSED(former_owner);
}

void PropertiesView::onEndMoveRelation(int row, const MObject *owner)
{
    MRelation *mrelation = owner->getRelations().at(row);
    if (mrelation && _selected_model_elements.contains(mrelation)) {
        _mview->update(_selected_model_elements);
    }
}

void PropertiesView::onRelationEndChanged(MRelation *relation, MObject *end_object)
{
    Q_UNUSED(end_object);
    if (relation && _selected_model_elements.contains(relation)) {
        _mview->update(_selected_model_elements);
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
    if (diagram == _selected_diagram && _selected_diagram_elements.size() > 0) {
        _mview->update(_selected_diagram_elements, _selected_diagram);
    }
}

void PropertiesView::onBeginUpdateElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
}

void PropertiesView::onEndUpdateElement(int row, const MDiagram *diagram)
{
    if (diagram == _selected_diagram) {
        DElement *delement = diagram->getDiagramElements().at(row);
        if (_selected_diagram_elements.contains(delement)) {
            _mview->update(_selected_diagram_elements, _selected_diagram);
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
    if (diagram == _selected_diagram) {
        DElement *delement = diagram->getDiagramElements().at(row);
        if (_selected_diagram_elements.contains(delement)) {
            clearSelection();
        }
    }
}

void PropertiesView::onEndRemoveElement(int row, const MDiagram *diagram)
{
    Q_UNUSED(row);
    Q_UNUSED(diagram);
}

void PropertiesView::beginUpdate(MElement *model_element)
{
    QMT_CHECK(model_element);

    if (MObject *object = dynamic_cast<MObject *>(model_element)) {
        _model_controller->startUpdateObject(object);
    } else if (MRelation *relation = dynamic_cast<MRelation *>(model_element)) {
        _model_controller->startUpdateRelation(relation);
    } else {
        QMT_CHECK(false);
    }
}

void PropertiesView::endUpdate(MElement *model_element, bool cancelled)
{
    QMT_CHECK(model_element);

    if (MObject *object = dynamic_cast<MObject *>(model_element)) {
        _model_controller->finishUpdateObject(object, cancelled);
    } else if (MRelation *relation = dynamic_cast<MRelation *>(model_element)) {
        _model_controller->finishUpdateRelation(relation, cancelled);
    } else {
        QMT_CHECK(false);
    }
}

void PropertiesView::beginUpdate(DElement *diagram_element)
{
    QMT_CHECK(diagram_element);
    QMT_CHECK(_selected_diagram != 0);
    QMT_CHECK(_diagram_controller->findElement(diagram_element->getUid(), _selected_diagram) == diagram_element);

    _diagram_controller->startUpdateElement(diagram_element, _selected_diagram, DiagramController::UPDATE_MINOR);
}

void PropertiesView::endUpdate(DElement *diagram_element, bool cancelled)
{
    QMT_CHECK(diagram_element);
    QMT_CHECK(_selected_diagram != 0);
    QMT_CHECK(_diagram_controller->findElement(diagram_element->getUid(), _selected_diagram) == diagram_element);

    _diagram_controller->finishUpdateElement(diagram_element, _selected_diagram, cancelled);
}

}
