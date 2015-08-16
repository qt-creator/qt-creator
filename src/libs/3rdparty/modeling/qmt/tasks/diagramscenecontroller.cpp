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

#include "diagramscenecontroller.h"

#include "qmt/controller/namecontroller.h"
#include "qmt/controller/undocontroller.h"
#include "qmt/diagram_controller/dfactory.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dselection.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dboundary.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram_ui/diagram_mime_types.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model_controller/mselection.h"
#include "qmt/model/massociation.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/minheritance.h"
#include "qmt/model/mitem.h"
#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/msourceexpansion.h"
#include "qmt/tasks/alignonrastervisitor.h"
#include "qmt/tasks/isceneinspector.h"
#include "qmt/tasks/voidelementtasks.h"

#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QQueue>
#include <QPair>


namespace qmt {

namespace {
static VoidElementTasks dummy_element_tasks;
}

DiagramSceneController::DiagramSceneController(QObject *parent)
    : QObject(parent),
      _model_controller(0),
      _diagram_controller(0),
      _element_tasks(&dummy_element_tasks),
      _scene_inspector(0)
{
}

DiagramSceneController::~DiagramSceneController()
{
}

void DiagramSceneController::setModelController(ModelController *model_controller)
{
    if (_model_controller == model_controller) {
        return;
    }
    if (_model_controller) {
        disconnect(_model_controller, 0, this, 0);
        _model_controller = 0;
    }
    if (model_controller) {
        _model_controller = model_controller;
    }
}

void DiagramSceneController::setDiagramController(DiagramController *diagram_controller)
{
    if (_diagram_controller == diagram_controller) {
        return;
    }
    if (_diagram_controller) {
        disconnect(_diagram_controller, 0, this, 0);
        _diagram_controller = 0;
    }
    if (diagram_controller) {
        _diagram_controller = diagram_controller;
    }
}

void DiagramSceneController::setElementTasks(IElementTasks *element_tasks)
{
    _element_tasks = element_tasks;
}

void DiagramSceneController::setSceneInspector(ISceneInspector *scene_inspector)
{
    _scene_inspector = scene_inspector;
}

void DiagramSceneController::deleteFromDiagram(const DSelection &dselection, MDiagram *diagram)
{
    if (!dselection.isEmpty()) {
        MSelection mselection;
        DSelection remaining_dselection;
        foreach (const DSelection::Index &index, dselection.getIndices()) {
            DElement *delement = _diagram_controller->findElement(index.getElementKey(), diagram);
            QMT_CHECK(delement);
            if (delement->getModelUid().isValid()) {
                MElement *melement = _model_controller->findElement(delement->getModelUid());
                QMT_CHECK(melement);
                if (melement->getOwner()) {
                    mselection.append(melement->getUid(), melement->getOwner()->getUid());
                }
            } else {
                remaining_dselection.append(index);
            }
        }
        if (!remaining_dselection.isEmpty()) {
            _diagram_controller->deleteElements(remaining_dselection, diagram);
        }
        if (!mselection.isEmpty()) {
            _model_controller->deleteElements(mselection);
        }
    }
}

void DiagramSceneController::createDependency(DObject *end_a_object, DObject *end_b_object, const QList<QPointF> &intermediate_points, MDiagram *diagram)
{
    _diagram_controller->getUndoController()->beginMergeSequence(tr("Create Dependency"));

    MObject *end_a_model_object = _model_controller->findObject<MObject>(end_a_object->getModelUid());
    QMT_CHECK(end_a_model_object);
    MObject *end_b_model_object = _model_controller->findObject<MObject>(end_b_object->getModelUid());
    QMT_CHECK(end_b_model_object);

    if (end_a_model_object == end_b_model_object) {
        return;
    }

    MDependency *model_dependency = new MDependency();
    model_dependency->setEndA(end_a_model_object->getUid());
    model_dependency->setEndB(end_b_model_object->getUid());
    model_dependency->setDirection(MDependency::A_TO_B);
    _model_controller->addRelation(end_a_model_object, model_dependency);

    DRelation *relation = addRelation(model_dependency, intermediate_points, diagram);

    _diagram_controller->getUndoController()->endMergeSequence();

    if (relation) {
        emit newElementCreated(relation, diagram);
    }
}

void DiagramSceneController::createInheritance(DClass *derived_class, DClass *base_class, const QList<QPointF> &intermediate_points, MDiagram *diagram)
{
    _diagram_controller->getUndoController()->beginMergeSequence(tr("Create Inheritance"));

    MClass *derived_model_class = _model_controller->findObject<MClass>(derived_class->getModelUid());
    QMT_CHECK(derived_model_class);
    MClass *base_model_class = _model_controller->findObject<MClass>(base_class->getModelUid());
    QMT_CHECK(base_model_class);

    if (derived_model_class == base_model_class) {
        return;
    }

    MInheritance *model_inheritance = new MInheritance();
    model_inheritance->setDerived(derived_model_class->getUid());
    model_inheritance->setBase(base_model_class->getUid());
    _model_controller->addRelation(derived_model_class, model_inheritance);

    DRelation *relation = addRelation(model_inheritance, intermediate_points, diagram);

    _diagram_controller->getUndoController()->endMergeSequence();

    if (relation) {
        emit newElementCreated(relation, diagram);
    }
}

void DiagramSceneController::createAssociation(DClass *end_a_class, DClass *end_b_class, const QList<QPointF> &intermediate_points, MDiagram *diagram)
{
    _diagram_controller->getUndoController()->beginMergeSequence(tr("Create Association"));

    MClass *end_a_model_object = _model_controller->findObject<MClass>(end_a_class->getModelUid());
    QMT_CHECK(end_a_model_object);
    MClass *end_b_model_object = _model_controller->findObject<MClass>(end_b_class->getModelUid());
    QMT_CHECK(end_b_model_object);

    // TODO allow self assignment with just one intermediate point and a nice round arrow
    if (end_a_model_object == end_b_model_object && intermediate_points.count() < 2) {
        return;
    }

    MAssociation *model_association = new MAssociation();
    model_association->setEndA(end_a_model_object->getUid());
    MAssociationEnd end_a = model_association->getA();
    end_a.setNavigable(true);
    model_association->setA(end_a);
    model_association->setEndB(end_b_model_object->getUid());
    _model_controller->addRelation(end_a_model_object, model_association);

    DRelation *relation = addRelation(model_association, intermediate_points, diagram);

    _diagram_controller->getUndoController()->endMergeSequence();

    if (relation) {
        emit newElementCreated(relation, diagram);
    }
}

bool DiagramSceneController::isAddingAllowed(const Uid &model_element_key, MDiagram *diagram)
{
    MElement *model_element = _model_controller->findElement(model_element_key);
    QMT_CHECK(model_element);
    if (_diagram_controller->hasDelegate(model_element, diagram)) {
        return false;
    }
    if (MRelation *model_relation = dynamic_cast<MRelation *>(model_element)) {
        MObject *end_a_model_object = _model_controller->findObject(model_relation->getEndA());
        QMT_CHECK(end_a_model_object);
        DObject *end_a_diagram_object = _diagram_controller->findDelegate<DObject>(end_a_model_object, diagram);
        if (!end_a_diagram_object) {
            return false;
        }

        MObject *end_b_model_object = _model_controller->findObject(model_relation->getEndB());
        QMT_CHECK(end_b_model_object);
        DObject *end_b_diagram_object = _diagram_controller->findDelegate<DObject>(end_b_model_object, diagram);
        if (!end_b_diagram_object) {
            return false;
        }
    }
    return true;
}

void DiagramSceneController::addExistingModelElement(const Uid &model_element_key, const QPointF &pos, MDiagram *diagram)
{
    DElement *element = addModelElement(model_element_key, pos, diagram);
    if (element) {
        emit elementAdded(element, diagram);
    }
}

void DiagramSceneController::dropNewElement(const QString &new_element_id, const QString &name, const QString &stereotype, DElement *top_most_element_at_pos, const QPointF &pos, MDiagram *diagram)
{
    if (new_element_id == QLatin1String(ELEMENT_TYPE_ANNOTATION)) {
        DAnnotation *annotation = new DAnnotation();
        annotation->setText(QStringLiteral(""));
        annotation->setPos(pos - QPointF(10.0, 10.0));
        _diagram_controller->addElement(annotation, diagram);
        alignOnRaster(annotation, diagram);
        emit newElementCreated(annotation, diagram);
    } else if (new_element_id == QLatin1String(ELEMENT_TYPE_BOUNDARY)) {
        DBoundary *boundary = new DBoundary();
        boundary->setPos(pos);
        _diagram_controller->addElement(boundary, diagram);
        alignOnRaster(boundary, diagram);
        emit newElementCreated(boundary, diagram);
    } else {
        MPackage *parent_package = findSuitableParentPackage(top_most_element_at_pos, diagram);
        MObject *new_object = 0;
        QString new_name;
        if (new_element_id == QLatin1String(ELEMENT_TYPE_PACKAGE)) {
            MPackage *package = new MPackage();
            new_name = tr("New Package");
            if (!stereotype.isEmpty()) {
                package->setStereotypes(QStringList() << stereotype);
            }
            new_object = package;
        } else if (new_element_id == QLatin1String(ELEMENT_TYPE_COMPONENT)) {
            MComponent *component = new MComponent();
            new_name = tr("New Component");
            if (!stereotype.isEmpty()) {
                component->setStereotypes(QStringList() << stereotype);
            }
            new_object = component;
        } else if (new_element_id == QLatin1String(ELEMENT_TYPE_CLASS)) {
            MClass *klass = new MClass();
            new_name = tr("New Class");
            if (!stereotype.isEmpty()) {
                klass->setStereotypes(QStringList() << stereotype);
            }
            new_object = klass;
        } else if (new_element_id == QLatin1String(ELEMENT_TYPE_ITEM)) {
            MItem *item = new MItem();
            new_name = tr("New Item");
            if (!stereotype.isEmpty()) {
                item->setVariety(stereotype);
                item->setVarietyEditable(false);
            }
            new_object = item;
        }
        if (new_object) {
            if (!name.isEmpty()) {
                new_name = tr("New %1").arg(name);
            }
            new_object->setName(new_name);
            dropNewModelElement(new_object, parent_package, pos, diagram);
        }
    }
}

void DiagramSceneController::dropNewModelElement(MObject *model_object, MPackage *parent_package, const QPointF &pos, MDiagram *diagram)
{
    _model_controller->getUndoController()->beginMergeSequence(tr("Drop Element"));
    _model_controller->addObject(parent_package, model_object);
    DElement *element = addObject(model_object, pos, diagram);
    _model_controller->getUndoController()->endMergeSequence();
    if (element) {
        emit newElementCreated(element, diagram);
    }
}

MPackage *DiagramSceneController::findSuitableParentPackage(DElement *topmost_diagram_element, MDiagram *diagram)
{
    MPackage *parent_package = 0;
    if (DPackage *diagram_package = dynamic_cast<DPackage *>(topmost_diagram_element)) {
        parent_package = _model_controller->findObject<MPackage>(diagram_package->getModelUid());
    } else if (DObject *diagram_object = dynamic_cast<DObject *>(topmost_diagram_element)) {
        MObject *model_object = _model_controller->findObject(diagram_object->getModelUid());
        if (model_object) {
            parent_package = dynamic_cast<MPackage *>(model_object->getOwner());
        }
    }
    if (parent_package == 0 && diagram != 0) {
        parent_package = dynamic_cast<MPackage *>(diagram->getOwner());
    }
    if (parent_package == 0) {
        parent_package = _model_controller->getRootPackage();
    }
    return parent_package;
}

MDiagram *DiagramSceneController::findDiagramBySearchId(MPackage *package, const QString &diagram_name)
{
    QString diagram_search_id = NameController::calcElementNameSearchId(diagram_name);
    foreach (const Handle<MObject> &handle, package->getChildren()) {
        if (handle.hasTarget()) {
            if (MDiagram *diagram = dynamic_cast<MDiagram *>(handle.getTarget())) {
                if (NameController::calcElementNameSearchId(diagram->getName()) == diagram_search_id) {
                    return diagram;
                }
            }
        }
    }
    return 0;
}

namespace {

static QPointF alignObjectLeft(DObject *object, DObject *other_object)
{
    qreal left = object->getPos().x() + object->getRect().left();
    QPointF pos = other_object->getPos();
    qreal other_left = pos.x() + other_object->getRect().left();
    qreal delta = other_left - left;
    pos.setX(pos.x() - delta);
    return pos;
}

static QPointF alignObjectRight(DObject *object, DObject *other_object)
{
    qreal right = object->getPos().x() + object->getRect().right();
    QPointF pos = other_object->getPos();
    qreal other_right = pos.x() + other_object->getRect().right();
    qreal delta = other_right - right;
    pos.setX(pos.x() - delta);
    return pos;
}

static QPointF alignObjectHCenter(DObject *object, DObject *other_object)
{
    qreal center = object->getPos().x();
    QPointF pos = other_object->getPos();
    qreal other_center = pos.x();
    qreal delta = other_center - center;
    pos.setX(pos.x() - delta);
    return pos;
}

static QPointF alignObjectTop(DObject *object, DObject *other_object)
{
    qreal top = object->getPos().y() + object->getRect().top();
    QPointF pos = other_object->getPos();
    qreal other_top = pos.y() + other_object->getRect().top();
    qreal delta = other_top - top;
    pos.setY(pos.y() - delta);
    return pos;
}

static QPointF alignObjectBottom(DObject *object, DObject *other_object)
{
    qreal bottom = object->getPos().y() + object->getRect().bottom();
    QPointF pos = other_object->getPos();
    qreal other_bottom = pos.y() + other_object->getRect().bottom();
    qreal delta = other_bottom - bottom;
    pos.setY(pos.y() - delta);
    return pos;
}

static QPointF alignObjectVCenter(DObject *object, DObject *other_object)
{
    qreal center = object->getPos().y();
    QPointF pos = other_object->getPos();
    qreal other_center = pos.y();
    qreal delta = other_center - center;
    pos.setY(pos.y() - delta);
    return pos;
}

static QRectF alignObjectWidth(DObject *object, const QSizeF &size)
{
    QRectF rect = object->getRect();
    rect.setX(-size.width() / 2.0);
    rect.setWidth(size.width());
    return rect;
}

static QRectF alignObjectHeight(DObject *object, const QSizeF &size)
{
    QRectF rect = object->getRect();
    rect.setY(-size.height() / 2.0);
    rect.setHeight(size.height());
    return rect;
}

static QRectF alignObjectSize(DObject *object, const QSizeF &size)
{
    Q_UNUSED(object);

    QRectF rect;
    rect.setX(-size.width() / 2.0);
    rect.setY(-size.height() / 2.0);
    rect.setWidth(size.width());
    rect.setHeight(size.height());
    return rect;
}

}

void DiagramSceneController::alignLeft(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectLeft, diagram);
}

void DiagramSceneController::alignRight(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectRight, diagram);
}

void DiagramSceneController::alignHCenter(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectHCenter, diagram);
}

void DiagramSceneController::alignTop(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectTop, diagram);
}

void DiagramSceneController::alignBottom(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectBottom, diagram);
}

void DiagramSceneController::alignVCenter(DObject *object, const DSelection &selection, MDiagram *diagram)
{
    alignPosition(object, selection, alignObjectVCenter, diagram);
}

void DiagramSceneController::alignWidth(DObject *object, const DSelection &selection, const QSizeF &minimum_size, MDiagram *diagram)
{
    alignSize(object, selection, minimum_size, alignObjectWidth, diagram);
}

void DiagramSceneController::alignHeight(DObject *object, const DSelection &selection, const QSizeF &minimum_size, MDiagram *diagram)
{
    alignSize(object, selection, minimum_size, alignObjectHeight, diagram);
}

void DiagramSceneController::alignSize(DObject *object, const DSelection &selection, const QSizeF &minimum_size, MDiagram *diagram)
{
    alignSize(object, selection, minimum_size, alignObjectSize, diagram);
}

void DiagramSceneController::alignPosition(DObject *object, const DSelection &selection, QPointF (*aligner)(DObject *, DObject *), MDiagram *diagram)
{
    foreach (const DSelection::Index &index, selection.getIndices()) {
        DElement *element = _diagram_controller->findElement(index.getElementKey(), diagram);
        if (DObject *selected_object = dynamic_cast<DObject *>(element)) {
            if (selected_object != object) {
                QPointF new_pos = aligner(object, selected_object);
                if (new_pos != selected_object->getPos()) {
                    _diagram_controller->startUpdateElement(selected_object, diagram, DiagramController::UPDATE_GEOMETRY);
                    selected_object->setPos(new_pos);
                    _diagram_controller->finishUpdateElement(selected_object, diagram, false);
                }
            }
        }
    }
}

void DiagramSceneController::alignSize(DObject *object, const DSelection &selection, const QSizeF &minimum_size, QRectF (*aligner)(DObject *, const QSizeF &), MDiagram *diagram)
{
    QSizeF size;
    if (object->getRect().width() < minimum_size.width()) {
        size.setWidth(minimum_size.width());
    } else {
        size.setWidth(object->getRect().width());
    }
    if (object->getRect().height() < minimum_size.height()) {
        size.setHeight(minimum_size.height());
    } else {
        size.setHeight(object->getRect().height());
    }
    foreach (const DSelection::Index &index, selection.getIndices()) {
        DElement *element = _diagram_controller->findElement(index.getElementKey(), diagram);
        if (DObject *selected_object = dynamic_cast<DObject *>(element)) {
            QRectF new_rect = aligner(selected_object, size);
            if (new_rect != selected_object->getRect()) {
                _diagram_controller->startUpdateElement(selected_object, diagram, DiagramController::UPDATE_GEOMETRY);
                selected_object->setAutoSize(false);
                selected_object->setRect(new_rect);
                _diagram_controller->finishUpdateElement(selected_object, diagram, false);
            }
        }
    }
}

void DiagramSceneController::alignOnRaster(DElement *element, MDiagram *diagram)
{
    AlignOnRasterVisitor visitor;
    visitor.setDiagramController(_diagram_controller);
    visitor.setSceneInspector(_scene_inspector);
    visitor.setDiagram(diagram);
    element->accept(&visitor);
}

DElement *DiagramSceneController::addModelElement(const Uid &model_element_key, const QPointF &pos, MDiagram *diagram)
{
    DElement *element = 0;
    if (MObject *model_object = _model_controller->findObject(model_element_key)) {
        element = addObject(model_object, pos, diagram);
    } else if (MRelation *model_relation = _model_controller->findRelation(model_element_key)) {
        element = addRelation(model_relation, QList<QPointF>(), diagram);
    } else {
        QMT_CHECK(false);
    }
    return element;
}

DObject *DiagramSceneController::addObject(MObject *model_object, const QPointF &pos, MDiagram *diagram)
{
    QMT_CHECK(model_object);

    if (_diagram_controller->hasDelegate(model_object, diagram)) {
        return 0;
    }

    _diagram_controller->getUndoController()->beginMergeSequence(tr("Add Element"));

    DFactory factory;
    model_object->accept(&factory);
    DObject *diagram_object = dynamic_cast<DObject *>(factory.getProduct());
    QMT_CHECK(diagram_object);
    diagram_object->setPos(pos);
    _diagram_controller->addElement(diagram_object, diagram);
    alignOnRaster(diagram_object, diagram);

    // add all relations between any other element on diagram and new element
    foreach (DElement *delement, diagram->getDiagramElements()) {
        if (delement != diagram_object) {
            DObject *dobject = dynamic_cast<DObject *>(delement);
            if (dobject) {
                MObject *mobject = _model_controller->findObject(dobject->getModelUid());
                if (mobject) {
                    foreach (const Handle<MRelation> &handle, mobject->getRelations()) {
                        if (handle.hasTarget()
                                && ((handle.getTarget()->getEndA() == model_object->getUid()
                                     && handle.getTarget()->getEndB() == mobject->getUid())
                                    || (handle.getTarget()->getEndA() == mobject->getUid()
                                        && handle.getTarget()->getEndB() == model_object->getUid()))) {
                            addRelation(handle.getTarget(), QList<QPointF>(), diagram);
                        }
                    }
                    foreach (const Handle<MRelation> &handle, model_object->getRelations()) {
                        if (handle.hasTarget()
                                && ((handle.getTarget()->getEndA() == model_object->getUid()
                                     && handle.getTarget()->getEndB() == mobject->getUid())
                                    || (handle.getTarget()->getEndA() == mobject->getUid()
                                        && handle.getTarget()->getEndB() == model_object->getUid()))) {
                            addRelation(handle.getTarget(), QList<QPointF>(), diagram);
                        }
                    }
                }
            }
        }
    }

    // add all self relations
    foreach (const Handle<MRelation> &handle, model_object->getRelations()) {
        if (handle.hasTarget ()
                && handle.getTarget()->getEndA() == model_object->getUid()
                && handle.getTarget()->getEndB() == model_object->getUid()) {
            addRelation(handle.getTarget(), QList<QPointF>(), diagram);
        }
    }

    _diagram_controller->getUndoController()->endMergeSequence();

    return diagram_object;
}

DRelation *DiagramSceneController::addRelation(MRelation *model_relation, const QList<QPointF> &intermediate_points, MDiagram *diagram)
{
    QMT_CHECK(model_relation);

    if (_diagram_controller->hasDelegate(model_relation, diagram)) {
        return 0;
    }

    DFactory factory;
    model_relation->accept(&factory);
    DRelation *diagram_relation = dynamic_cast<DRelation *>(factory.getProduct());
    QMT_CHECK(diagram_relation);

    MObject *end_a_model_object = _model_controller->findObject(model_relation->getEndA());
    QMT_CHECK(end_a_model_object);
    DObject *end_a_diagram_object = _diagram_controller->findDelegate<DObject>(end_a_model_object, diagram);
    QMT_CHECK(end_a_diagram_object);
    diagram_relation->setEndA(end_a_diagram_object->getUid());

    MObject *end_b_model_object = _model_controller->findObject(model_relation->getEndB());
    QMT_CHECK(end_b_model_object);
    DObject *end_b_diagram_object = _diagram_controller->findDelegate<DObject>(end_b_model_object, diagram);
    QMT_CHECK(end_b_diagram_object);
    diagram_relation->setEndB(end_b_diagram_object->getUid());

    QList<DRelation::IntermediatePoint> relation_points;
    if (end_a_diagram_object->getUid() == end_b_diagram_object->getUid() && intermediate_points.isEmpty()) {
        // create some intermediate points for self-relation
        QRectF rect = end_a_diagram_object->getRect().translated(end_a_diagram_object->getPos());
        static const qreal EDGE_RADIUS = 30.0;
        qreal w = rect.width() * 0.25;
        if (w > EDGE_RADIUS) {
            w = EDGE_RADIUS;
        }
        qreal h = rect.height() * 0.25;
        if (h > EDGE_RADIUS) {
            h = EDGE_RADIUS;
        }
        QPointF i1(rect.x() - EDGE_RADIUS, rect.bottom() - h);
        QPointF i2(rect.x() - EDGE_RADIUS, rect.bottom() + EDGE_RADIUS);
        QPointF i3(rect.x() + w, rect.bottom() + EDGE_RADIUS);
        relation_points.append(DRelation::IntermediatePoint(i1));
        relation_points.append(DRelation::IntermediatePoint(i2));
        relation_points.append(DRelation::IntermediatePoint(i3));
    } else {
        foreach (const QPointF &intermediate_point, intermediate_points) {
            relation_points.append(DRelation::IntermediatePoint(intermediate_point));
        }
    }
    diagram_relation->setIntermediatePoints(relation_points);

    _diagram_controller->addElement(diagram_relation, diagram);
    alignOnRaster(diagram_relation, diagram);

    return diagram_relation;
}

}
