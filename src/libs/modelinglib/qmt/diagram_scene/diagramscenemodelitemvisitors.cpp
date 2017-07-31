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

#include "diagramscenemodelitemvisitors.h"

#include "items/packageitem.h"
#include "items/classitem.h"
#include "items/componentitem.h"
#include "items/diagramitem.h"
#include "items/itemitem.h"
#include "items/relationitem.h"
#include "items/associationitem.h"
#include "items/connectionitem.h"
#include "items/annotationitem.h"
#include "items/boundaryitem.h"
#include "items/swimlaneitem.h"

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
#include "qmt/diagram/dconnection.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dboundary.h"
#include "qmt/diagram/dswimlane.h"
#include "qmt/infrastructure/qmtassert.h"

namespace qmt {

DiagramSceneModel::CreationVisitor::CreationVisitor(DiagramSceneModel *diagramSceneModel)
    : m_diagramSceneModel(diagramSceneModel)
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
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new PackageItem(package, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDClass(DClass *klass)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new ClassItem(klass, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDComponent(DComponent *component)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new ComponentItem(component, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDDiagram(DDiagram *diagram)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new DiagramItem(diagram, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDItem(DItem *item)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new ItemItem(item, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDRelation(DRelation *relation)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new RelationItem(relation, m_diagramSceneModel);
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
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new AssociationItem(association, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDConnection(DConnection *connection)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new ConnectionItem(connection, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDAnnotation(DAnnotation *annotation)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new AnnotationItem(annotation, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDBoundary(DBoundary *boundary)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new BoundaryItem(boundary, m_diagramSceneModel);
}

void DiagramSceneModel::CreationVisitor::visitDSwimlane(DSwimlane *swimlane)
{
    QMT_CHECK(!m_graphicsItem);
    m_graphicsItem = new SwimlaneItem(swimlane, m_diagramSceneModel);
}

DiagramSceneModel::UpdateVisitor::UpdateVisitor(QGraphicsItem *item, DiagramSceneModel *diagramSceneModel,
                                                DElement *relatedElement)
    : m_graphicsItem(item),
      m_diagramSceneModel(diagramSceneModel),
      m_relatedElement(relatedElement)
{
}

void DiagramSceneModel::UpdateVisitor::visitDElement(DElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(false);
}

void DiagramSceneModel::UpdateVisitor::visitDObject(DObject *object)
{
    if (!m_relatedElement) {
        // update all related relations
        foreach (QGraphicsItem *item, m_diagramSceneModel->m_graphicsItems) {
            DElement *element = m_diagramSceneModel->m_itemToElementMap.value(item);
            QMT_CHECK(element);
            if (dynamic_cast<DRelation *>(element)) {
                UpdateVisitor visitor(item, m_diagramSceneModel, object);
                element->accept(&visitor);
            }
        }
    }
}

void DiagramSceneModel::UpdateVisitor::visitDPackage(DPackage *package)
{
    QMT_ASSERT(m_graphicsItem, return);

    if (!m_relatedElement) {
        PackageItem *packageItem = qgraphicsitem_cast<PackageItem *>(m_graphicsItem);
        QMT_ASSERT(packageItem, return);
        QMT_CHECK(packageItem->object() == package);
        packageItem->update();
    }

    visitDObject(package);
}

void DiagramSceneModel::UpdateVisitor::visitDClass(DClass *klass)
{
    QMT_ASSERT(m_graphicsItem, return);

    if (!m_relatedElement) {
        ClassItem *classItem = qgraphicsitem_cast<ClassItem *>(m_graphicsItem);
        QMT_ASSERT(classItem, return);
        QMT_CHECK(classItem->object() == klass);
        classItem->update();
    }

    visitDObject(klass);
}

void DiagramSceneModel::UpdateVisitor::visitDComponent(DComponent *component)
{
    QMT_ASSERT(m_graphicsItem, return);

    if (!m_relatedElement) {
        ComponentItem *componentItem = qgraphicsitem_cast<ComponentItem *>(m_graphicsItem);
        QMT_ASSERT(componentItem, return);
        QMT_CHECK(componentItem->object() == component);
        componentItem->update();
    }

    visitDObject(component);
}

void DiagramSceneModel::UpdateVisitor::visitDDiagram(DDiagram *diagram)
{
    QMT_ASSERT(m_graphicsItem, return);

    if (!m_relatedElement) {
        DiagramItem *documentItem = qgraphicsitem_cast<DiagramItem *>(m_graphicsItem);
        QMT_ASSERT(documentItem, return);
        QMT_CHECK(documentItem->object() == diagram);
        documentItem->update();
    }

    visitDObject(diagram);
}

void DiagramSceneModel::UpdateVisitor::visitDItem(DItem *item)
{
    QMT_ASSERT(m_graphicsItem, return);

    if (!m_relatedElement) {
        ItemItem *itemItem = qgraphicsitem_cast<ItemItem *>(m_graphicsItem);
        QMT_ASSERT(itemItem, return);
        QMT_CHECK(itemItem->object() == item);
        itemItem->update();
    }

    visitDObject(item);
}

void DiagramSceneModel::UpdateVisitor::visitDRelation(DRelation *relation)
{
    QMT_ASSERT(m_graphicsItem, return);

    if (!m_relatedElement
            || m_relatedElement->uid() == relation->endAUid()
            || m_relatedElement->uid() == relation->endBUid()) {
        RelationItem *relationItem = qgraphicsitem_cast<RelationItem *>(m_graphicsItem);
        QMT_ASSERT(relationItem, return);
        QMT_CHECK(relationItem->relation() == relation);
        relationItem->update();
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

void DiagramSceneModel::UpdateVisitor::visitDConnection(DConnection *connection)
{
    visitDRelation(connection);
}

void DiagramSceneModel::UpdateVisitor::visitDAnnotation(DAnnotation *annotation)
{
    Q_UNUSED(annotation); // avoid warning in release mode
    QMT_ASSERT(m_graphicsItem, return);

    AnnotationItem *annotationItem = qgraphicsitem_cast<AnnotationItem *>(m_graphicsItem);
    QMT_ASSERT(annotationItem, return);
    QMT_CHECK(annotationItem->annotation() == annotation);
    annotationItem->update();
}

void DiagramSceneModel::UpdateVisitor::visitDBoundary(DBoundary *boundary)
{
    Q_UNUSED(boundary); // avoid warning in release mode
    QMT_ASSERT(m_graphicsItem, return);

    BoundaryItem *boundaryItem = qgraphicsitem_cast<BoundaryItem *>(m_graphicsItem);
    QMT_ASSERT(boundaryItem, return);
    QMT_CHECK(boundaryItem->boundary() == boundary);
    boundaryItem->update();
}

void DiagramSceneModel::UpdateVisitor::visitDSwimlane(DSwimlane *swimlane)
{
    Q_UNUSED(swimlane); // avoid warning in release mode
    QMT_ASSERT(m_graphicsItem, return);

    SwimlaneItem *swimlaneItem = qgraphicsitem_cast<SwimlaneItem *>(m_graphicsItem);
    QMT_ASSERT(swimlaneItem, return);
    QMT_CHECK(swimlaneItem->swimlane() == swimlane);
    swimlaneItem->update();
}

} // namespace qmt
