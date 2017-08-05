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

#include "mflatassignmentvisitor.h"

#include "qmt/model/mpackage.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mitem.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/minheritance.h"
#include "qmt/model/massociation.h"
#include "qmt/model/mconnection.h"

namespace qmt {

// TODO may flat assignment visitor use operator=() ?

MFlatAssignmentVisitor::MFlatAssignmentVisitor(MElement *target)
    : m_target(target)
{
    QMT_CHECK(m_target);
}

void MFlatAssignmentVisitor::visitMElement(const MElement *element)
{
    m_target->setStereotypes(element->stereotypes());
}

void MFlatAssignmentVisitor::visitMObject(const MObject *object)
{
    visitMElement(object);
    auto targetObject = dynamic_cast<MObject *>(m_target);
    QMT_ASSERT(targetObject, return);
    targetObject->setName(object->name());
}

void MFlatAssignmentVisitor::visitMPackage(const MPackage *package)
{
    visitMObject(package);
}

void MFlatAssignmentVisitor::visitMClass(const MClass *klass)
{
    visitMObject(klass);
    auto targetClass = dynamic_cast<MClass *>(m_target);
    QMT_ASSERT(targetClass, return);
    targetClass->setUmlNamespace(klass->umlNamespace());
    targetClass->setTemplateParameters(klass->templateParameters());
    targetClass->setMembers(klass->members());
}

void MFlatAssignmentVisitor::visitMComponent(const MComponent *component)
{
    visitMObject(component);
}

void MFlatAssignmentVisitor::visitMDiagram(const MDiagram *diagram)
{
    visitMObject(diagram);
    auto targetDiagram = dynamic_cast<MDiagram *>(m_target);
    QMT_ASSERT(targetDiagram, return);
    targetDiagram->setToolbarId(diagram->toolbarId());
}

void MFlatAssignmentVisitor::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    visitMDiagram(diagram);
}

void MFlatAssignmentVisitor::visitMItem(const MItem *item)
{
    visitMObject(item);
    auto targetItem = dynamic_cast<MItem *>(m_target);
    QMT_ASSERT(targetItem, return);
    targetItem->setVarietyEditable(item->isVarietyEditable());
    targetItem->setVariety(item->variety());
    targetItem->setShapeEditable(item->isShapeEditable());
}

void MFlatAssignmentVisitor::visitMRelation(const MRelation *relation)
{
    visitMElement(relation);
    auto targetRelation = dynamic_cast<MRelation *>(m_target);
    QMT_ASSERT(targetRelation, return);
    targetRelation->setName(relation->name());
    targetRelation->setEndAUid(relation->endAUid());
    targetRelation->setEndBUid(relation->endBUid());
}

void MFlatAssignmentVisitor::visitMDependency(const MDependency *dependency)
{
    visitMRelation(dependency);
    auto targetDependency = dynamic_cast<MDependency *>(m_target);
    QMT_ASSERT(targetDependency, return);
    targetDependency->setDirection(dependency->direction());
}

void MFlatAssignmentVisitor::visitMInheritance(const MInheritance *inheritance)
{
    visitMRelation(inheritance);
}

void MFlatAssignmentVisitor::visitMAssociation(const MAssociation *association)
{
    visitMRelation(association);
    auto targetAssociation = dynamic_cast<MAssociation *>(m_target);
    QMT_ASSERT(targetAssociation, return);
    targetAssociation->setEndA(association->endA());
    targetAssociation->setEndB(association->endB());
    // TODO assign association class UID?
}

void MFlatAssignmentVisitor::visitMConnection(const MConnection *connection)
{
    visitMRelation(connection);
    auto targetConnection = dynamic_cast<MConnection *>(m_target);
    QMT_ASSERT(targetConnection, return);
    targetConnection->setCustomRelationId(connection->customRelationId());
    targetConnection->setEndA(connection->endA());
    targetConnection->setEndB(connection->endB());
}

} // namespace qmt
