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

#include "diagramscenemodelitemvisitors.h"

#include "items/packageitem.h"
#include "items/classitem.h"
#include "items/componentitem.h"
#include "items/diagramitem.h"
#include "items/itemitem.h"
#include "items/relationitem.h"
#include "items/associationitem.h"
#include "items/annotationitem.h"
#include "items/boundaryitem.h"

#include "qmt/diagram/delement.h"
#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram/ddiagram.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram/dinheritance.h"
#include "qmt/diagram/ddependency.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dboundary.h"
#include "qmt/infrastructure/qmtassert.h"


namespace qmt {

DiagramSceneModel::CreationVisitor::CreationVisitor(DiagramSceneModel *diagram_scene_model)
    : _diagram_scene_model(diagram_scene_model),
      _graphics_item(0)
{
}

void DiagramSceneModel::CreationVisitor::visitDElement(DElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(false);
}

void DiagramSceneModel::CreationVisitor::visitDObject(DObject *object)
{
    Q_UNUSED(object);
    QMT_CHECK(false);
}

void DiagramSceneModel::CreationVisitor::visitDPackage(DPackage *package)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new PackageItem(package, _diagram_scene_model);
}

void DiagramSceneModel::CreationVisitor::visitDClass(DClass *klass)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new ClassItem(klass, _diagram_scene_model);
}

void DiagramSceneModel::CreationVisitor::visitDComponent(DComponent *component)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new ComponentItem(component, _diagram_scene_model);
}

void DiagramSceneModel::CreationVisitor::visitDDiagram(DDiagram *diagram)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new DiagramItem(diagram, _diagram_scene_model);
}

void DiagramSceneModel::CreationVisitor::visitDItem(DItem *item)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new ItemItem(item, _diagram_scene_model);
}

void DiagramSceneModel::CreationVisitor::visitDRelation(DRelation *relation)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new RelationItem(relation, _diagram_scene_model);
}

void DiagramSceneModel::CreationVisitor::visitDInheritance(DInheritance *inheritance)
{
    visitDRelation(inheritance);
}

void DiagramSceneModel::CreationVisitor::visitDDependency(DDependency *dependency)
{
    visitDRelation(dependency);
}

void DiagramSceneModel::CreationVisitor::visitDAssociation(DAssociation *association)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new AssociationItem(association, _diagram_scene_model);
}

void DiagramSceneModel::CreationVisitor::visitDAnnotation(DAnnotation *annotation)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new AnnotationItem(annotation, _diagram_scene_model);
}

void DiagramSceneModel::CreationVisitor::visitDBoundary(DBoundary *boundary)
{
    QMT_CHECK(!_graphics_item);
    _graphics_item = new BoundaryItem(boundary, _diagram_scene_model);
}



DiagramSceneModel::UpdateVisitor::UpdateVisitor(QGraphicsItem *item, DiagramSceneModel *diagram_scene_model, DElement *related_element)
    : _graphics_item(item),
      _diagram_scene_model(diagram_scene_model),
      _related_element(related_element)
{
}

void DiagramSceneModel::UpdateVisitor::visitDElement(DElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(false);
}

void DiagramSceneModel::UpdateVisitor::visitDObject(DObject *object)
{
    if (_related_element == 0) {
        // update all related relations
        foreach (QGraphicsItem *item, _diagram_scene_model->_graphics_items) {
            DElement *element = _diagram_scene_model->_item_to_element_map.value(item);
            QMT_CHECK(element);
            if (dynamic_cast<DRelation *>(element) != 0) {
                UpdateVisitor visitor(item, _diagram_scene_model, object);
                element->accept(&visitor);
            }
        }
    }
}

void DiagramSceneModel::UpdateVisitor::visitDPackage(DPackage *package)
{
    QMT_CHECK(_graphics_item);

    if (_related_element == 0) {
        PackageItem *package_item = qgraphicsitem_cast<PackageItem *>(_graphics_item);
        QMT_CHECK(package_item);
        QMT_CHECK(package_item->getObject() == package);
        package_item->update();
    }

    visitDObject(package);
}

void DiagramSceneModel::UpdateVisitor::visitDClass(DClass *klass)
{
    QMT_CHECK(_graphics_item);

    if (_related_element == 0) {
        ClassItem *class_item = qgraphicsitem_cast<ClassItem *>(_graphics_item);
        QMT_CHECK(class_item);
        QMT_CHECK(class_item->getObject() == klass);
        class_item->update();
    }

    visitDObject(klass);
}

void DiagramSceneModel::UpdateVisitor::visitDComponent(DComponent *component)
{
    QMT_CHECK(_graphics_item);

    if (_related_element == 0) {
        ComponentItem *component_item = qgraphicsitem_cast<ComponentItem *>(_graphics_item);
        QMT_CHECK(component_item);
        QMT_CHECK(component_item->getObject() == component);
        component_item->update();
    }

    visitDObject(component);
}

void DiagramSceneModel::UpdateVisitor::visitDDiagram(DDiagram *diagram)
{
    QMT_CHECK(_graphics_item);

    if (_related_element == 0) {
        DiagramItem *document_item = qgraphicsitem_cast<DiagramItem *>(_graphics_item);
        QMT_CHECK(document_item);
        QMT_CHECK(document_item->getObject() == diagram);
        document_item->update();
    }

    visitDObject(diagram);
}

void DiagramSceneModel::UpdateVisitor::visitDItem(DItem *item)
{
    QMT_CHECK(_graphics_item);

    if (_related_element == 0) {
        ItemItem *item_item = qgraphicsitem_cast<ItemItem *>(_graphics_item);
        QMT_CHECK(item_item);
        QMT_CHECK(item_item->getObject() == item);
        item_item->update();
    }

    visitDObject(item);
}

void DiagramSceneModel::UpdateVisitor::visitDRelation(DRelation *relation)
{
    QMT_CHECK(_graphics_item);

    if (_related_element == 0 || _related_element->getUid() == relation->getEndA() || _related_element->getUid() == relation->getEndB()) {
        RelationItem *relation_item = qgraphicsitem_cast<RelationItem *>(_graphics_item);
        QMT_CHECK(relation_item);
        QMT_CHECK(relation_item->getRelation() == relation);
        relation_item->update();
    }
}

void DiagramSceneModel::UpdateVisitor::visitDInheritance(DInheritance *inheritance)
{
    visitDRelation(inheritance);
}

void DiagramSceneModel::UpdateVisitor::visitDDependency(DDependency *dependency)
{
    visitDRelation(dependency);
}

void DiagramSceneModel::UpdateVisitor::visitDAssociation(DAssociation *association)
{
    visitDRelation(association);
}

void DiagramSceneModel::UpdateVisitor::visitDAnnotation(DAnnotation *annotation)
{
    Q_UNUSED(annotation); // avoid warning in release mode
    QMT_CHECK(_graphics_item);

    AnnotationItem *annotation_item = qgraphicsitem_cast<AnnotationItem *>(_graphics_item);
    QMT_CHECK(annotation_item);
    QMT_CHECK(annotation_item->getAnnotation() == annotation);
    annotation_item->update();
}

void DiagramSceneModel::UpdateVisitor::visitDBoundary(DBoundary *boundary)
{
    Q_UNUSED(boundary); // avoid warning in release mode
    QMT_CHECK(_graphics_item);

    BoundaryItem *boundary_item = qgraphicsitem_cast<BoundaryItem *>(_graphics_item);
    QMT_CHECK(boundary_item);
    QMT_CHECK(boundary_item->getBoundary() == boundary);
    boundary_item->update();
}

}
