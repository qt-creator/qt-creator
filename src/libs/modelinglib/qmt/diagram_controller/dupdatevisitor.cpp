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

#include "dupdatevisitor.h"

#include "qmt/diagram/delement.h"
#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram/ddependency.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram/dconnection.h"

#include "qmt/model/melement.h"
#include "qmt/model/mobject.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mitem.h"
#include "qmt/model/mrelation.h"
#include "qmt/model/massociation.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/minheritance.h"

namespace qmt {

DUpdateVisitor::DUpdateVisitor(DElement *target, const MDiagram *diagram, bool checkNeedsUpdate)
    : m_target(target),
      m_diagram(diagram),
      m_checkNeedsUpdate(checkNeedsUpdate),
      m_isUpdateNeeded(!checkNeedsUpdate)
{
}

void DUpdateVisitor::setCheckNeedsUpdate(bool checkNeedsUpdate)
{
    m_checkNeedsUpdate = checkNeedsUpdate;
    m_isUpdateNeeded = !checkNeedsUpdate;
}

void DUpdateVisitor::visitMElement(const MElement *element)
{
    Q_UNUSED(element);

    QMT_CHECK(m_target);
}

void DUpdateVisitor::visitMObject(const MObject *object)
{
    auto dobject = dynamic_cast<DObject *>(m_target);
    QMT_ASSERT(dobject, return);
    if (isUpdating(object->stereotypes() != dobject->stereotypes()))
        dobject->setStereotypes(object->stereotypes());
    const MObject *objectOwner = object->owner();
    const MObject *diagramOwner = m_diagram->owner();
    if (objectOwner && diagramOwner && objectOwner->uid() != diagramOwner->uid()) {
        if (isUpdating(objectOwner->name() != dobject->context()))
            dobject->setContext(objectOwner->name());
    } else {
        if (isUpdating(!dobject->context().isEmpty()))
            dobject->setContext(QString());
    }
    if (isUpdating(object->name() != dobject->name()))
        dobject->setName(object->name());
    // TODO unlikely that this is called for all objects if hierarchy is modified
    // PERFORM remove loop
    int depth = 1;
    const MObject *owner = object->owner();
    while (owner) {
        owner = owner->owner();
        depth += 1;
    }
    if (isUpdating(depth != dobject->depth()))
        dobject->setDepth(depth);
    visitMElement(object);
}

void DUpdateVisitor::visitMPackage(const MPackage *package)
{
    visitMObject(package);
}

void DUpdateVisitor::visitMClass(const MClass *klass)
{
    auto dclass = dynamic_cast<DClass *>(m_target);
    QMT_ASSERT(dclass, return);
    if (isUpdating(klass->umlNamespace() != dclass->umlNamespace()))
        dclass->setUmlNamespace(klass->umlNamespace());
    if (isUpdating(klass->templateParameters() != dclass->templateParameters()))
        dclass->setTemplateParameters(klass->templateParameters());
    if (isUpdating(klass->members() != dclass->members()))
        dclass->setMembers(klass->members());
    visitMObject(klass);
}

void DUpdateVisitor::visitMComponent(const MComponent *component)
{
    visitMObject(component);
}

void DUpdateVisitor::visitMDiagram(const MDiagram *diagram)
{
    visitMObject(diagram);
}

void DUpdateVisitor::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    visitMDiagram(diagram);
}

void DUpdateVisitor::visitMItem(const MItem *item)
{
    auto ditem = dynamic_cast<DItem *>(m_target);
    QMT_ASSERT(ditem, return);
    if (isUpdating(item->isShapeEditable() != ditem->isShapeEditable()))
        ditem->setShapeEditable(item->isShapeEditable());
    if (isUpdating(item->variety() != ditem->variety()))
        ditem->setVariety(item->variety());
    visitMObject(item);
}

void DUpdateVisitor::visitMRelation(const MRelation *relation)
{
    auto drelation = dynamic_cast<DRelation *>(m_target);
    QMT_ASSERT(drelation, return);
    if (isUpdating(relation->stereotypes() != drelation->stereotypes()))
        drelation->setStereotypes(relation->stereotypes());
    if (isUpdating(relation->name() != drelation->name()))
        drelation->setName(relation->name());
    // TODO improve performance of MDiagram::findDiagramElement
    DObject *endAObject = dynamic_cast<DObject *>(m_diagram->findDiagramElement(drelation->endAUid()));
    if (!endAObject || relation->endAUid() != endAObject->modelUid()) {
        (void) isUpdating(true);
        endAObject = nullptr;
        // TODO use DiagramController::findDelegate (and improve performance of that method)
        foreach (DElement *diagramElement, m_diagram->diagramElements()) {
            if (diagramElement->modelUid().isValid() && diagramElement->modelUid() == relation->endAUid()) {
                endAObject = dynamic_cast<DObject *>(diagramElement);
                break;
            }
        }
        if (endAObject)
            drelation->setEndAUid(endAObject->uid());
        else
            drelation->setEndAUid(Uid::invalidUid());
    }
    DObject *endBObject = dynamic_cast<DObject *>(m_diagram->findDiagramElement(drelation->endBUid()));
    if (!endBObject || relation->endBUid() != endBObject->modelUid()) {
        (void) isUpdating(true);
        endBObject = nullptr;
        // TODO use DiagramController::findDelegate
        foreach (DElement *diagramElement, m_diagram->diagramElements()) {
            if (diagramElement->modelUid().isValid() && diagramElement->modelUid() == relation->endBUid()) {
                endBObject = dynamic_cast<DObject *>(diagramElement);
                break;
            }
        }
        if (endBObject)
            drelation->setEndBUid(endBObject->uid());
        else
            drelation->setEndBUid(Uid::invalidUid());
    }
    visitMElement(relation);
}

void DUpdateVisitor::visitMDependency(const MDependency *dependency)
{
    auto ddependency = dynamic_cast<DDependency *>(m_target);
    QMT_ASSERT(ddependency, return);
    if (isUpdating(dependency->direction() != ddependency->direction()))
        ddependency->setDirection(dependency->direction());
    visitMRelation(dependency);
}

void DUpdateVisitor::visitMInheritance(const MInheritance *inheritance)
{
    visitMRelation(inheritance);
}

void DUpdateVisitor::visitMAssociation(const MAssociation *association)
{
    auto dassociation = dynamic_cast<DAssociation *>(m_target);
    QMT_ASSERT(dassociation, return);
    DAssociationEnd endA;
    endA.setName(association->endA().name());
    endA.setCardinatlity(association->endA().cardinality());
    endA.setNavigable(association->endA().isNavigable());
    endA.setKind(association->endA().kind());
    if (isUpdating(endA != dassociation->endA()))
        dassociation->setEndA(endA);
    DAssociationEnd endB;
    endB.setName(association->endB().name());
    endB.setCardinatlity(association->endB().cardinality());
    endB.setNavigable(association->endB().isNavigable());
    endB.setKind(association->endB().kind());
    if (isUpdating(endB != dassociation->endB()))
        dassociation->setEndB(endB);
    visitMRelation(association);
}

void DUpdateVisitor::visitMConnection(const MConnection *connection)
{
    auto dconnection = dynamic_cast<DConnection *>(m_target);
    QMT_ASSERT(dconnection, return);
    if (isUpdating(connection->customRelationId() != dconnection->customRelationId()))
        dconnection->setCustomRelationId(connection->customRelationId());
    DConnectionEnd endA;
    endA.setName(connection->endA().name());
    endA.setCardinatlity(connection->endA().cardinality());
    endA.setNavigable(connection->endA().isNavigable());
    if (isUpdating(endA != dconnection->endA()))
        dconnection->setEndA(endA);
    DConnectionEnd endB;
    endB.setName(connection->endB().name());
    endB.setCardinatlity(connection->endB().cardinality());
    endB.setNavigable(connection->endB().isNavigable());
    if (isUpdating(endB != dconnection->endB()))
        dconnection->setEndB(endB);
    visitMRelation(connection);
}

bool DUpdateVisitor::isUpdating(bool valueChanged)
{
    if (m_checkNeedsUpdate) {
        if (valueChanged)
            m_isUpdateNeeded = true;
        return false;
    }
    return valueChanged;
}

} // namespace qmt
