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

#include "dupdatevisitor.h"

#include "qmt/diagram/delement.h"
#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram/ddependency.h"
#include "qmt/diagram/dassociation.h"

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
      m_updateNeeded(!checkNeedsUpdate)
{
}

void DUpdateVisitor::setCheckNeedsUpdate(bool checkNeedsUpdate)
{
    m_checkNeedsUpdate = checkNeedsUpdate;
    m_updateNeeded = !checkNeedsUpdate;
}

void DUpdateVisitor::visitMElement(const MElement *element)
{
    Q_UNUSED(element);

    QMT_CHECK(m_target);
}

void DUpdateVisitor::visitMObject(const MObject *object)
{
    DObject *dobject = dynamic_cast<DObject *>(m_target);
    QMT_CHECK(dobject);
    if (isUpdating(object->getStereotypes() != dobject->getStereotypes())) {
        dobject->setStereotypes(object->getStereotypes());
    }
    const MObject *objectOwner = object->getOwner();
    const MObject *diagramOwner = m_diagram->getOwner();
    if (objectOwner && diagramOwner && objectOwner->getUid() != diagramOwner->getUid()) {
        if (isUpdating(objectOwner->getName() != dobject->getContext())) {
            dobject->setContext(objectOwner->getName());
        }
    } else {
        if (isUpdating(!dobject->getContext().isEmpty())) {
            dobject->setContext(QString());
        }
    }
    if (isUpdating(object->getName() != dobject->getName())) {
        dobject->setName(object->getName());
    }
    // TODO unlikely that this is called for all objects if hierarchy is modified
    // PERFORM remove loop
    int depth = 1;
    const MObject *owner = object->getOwner();
    while (owner) {
        owner = owner->getOwner();
        depth += 1;
    }
    if (isUpdating(depth != dobject->getDepth())) {
        dobject->setDepth(depth);
    }
    visitMElement(object);
}

void DUpdateVisitor::visitMPackage(const MPackage *package)
{
    visitMObject(package);
}

void DUpdateVisitor::visitMClass(const MClass *klass)
{
    DClass *dclass = dynamic_cast<DClass *>(m_target);
    QMT_CHECK(dclass);
    if (isUpdating(klass->getNamespace() != dclass->getNamespace())) {
        dclass->setNamespace(klass->getNamespace());
    }
    if (isUpdating(klass->getTemplateParameters() != dclass->getTemplateParameters())) {
        dclass->setTemplateParameters(klass->getTemplateParameters());
    }
    if (isUpdating(klass->getMembers() != dclass->getMembers())) {
        dclass->setMembers(klass->getMembers());
    }
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
    DItem *ditem = dynamic_cast<DItem *>(m_target);
    QMT_CHECK(ditem);
    if (isUpdating(item->isShapeEditable() != ditem->isShapeEditable())) {
        ditem->setShapeEditable(item->isShapeEditable());
    }
    if (isUpdating(item->getVariety() != ditem->getVariety())) {
        ditem->setVariety(item->getVariety());
    }
    visitMObject(item);
}

void DUpdateVisitor::visitMRelation(const MRelation *relation)
{
    DRelation *drelation = dynamic_cast<DRelation *>(m_target);
    QMT_CHECK(drelation);
    if (isUpdating(relation->getStereotypes() != drelation->getStereotypes())) {
        drelation->setStereotypes(relation->getStereotypes());
    }
    if (isUpdating(relation->getName() != drelation->getName())) {
        drelation->setName(relation->getName());
    }
    visitMElement(relation);
}

void DUpdateVisitor::visitMDependency(const MDependency *dependency)
{
    DDependency *ddependency = dynamic_cast<DDependency *>(m_target);
    QMT_CHECK(ddependency);
    if (isUpdating(dependency->getDirection() != ddependency->getDirection())) {
        ddependency->setDirection(dependency->getDirection());
    }
    visitMRelation(dependency);
}

void DUpdateVisitor::visitMInheritance(const MInheritance *inheritance)
{
    visitMRelation(inheritance);
}

void DUpdateVisitor::visitMAssociation(const MAssociation *association)
{
    DAssociation *dassociation = dynamic_cast<DAssociation *>(m_target);
    QMT_CHECK(dassociation);
    DAssociationEnd endA;
    endA.setName(association->getA().getName());
    endA.setCardinatlity(association->getA().getCardinality());
    endA.setNavigable(association->getA().isNavigable());
    endA.setKind(association->getA().getKind());
    if (isUpdating(endA != dassociation->getA())) {
        dassociation->setA(endA);
    }
    DAssociationEnd endB;
    endB.setName(association->getB().getName());
    endB.setCardinatlity(association->getB().getCardinality());
    endB.setNavigable(association->getB().isNavigable());
    endB.setKind(association->getB().getKind());
    if (isUpdating(endB != dassociation->getB())) {
        dassociation->setB(endB);
    }
    visitMRelation(association);
}

bool DUpdateVisitor::isUpdating(bool valueChanged)
{
    if (m_checkNeedsUpdate) {
        if (valueChanged) {
            m_updateNeeded = true;
        }
        return false;
    }
    return valueChanged;
}

}
