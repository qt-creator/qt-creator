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
    MObject *targetObject = dynamic_cast<MObject *>(m_target);
    QMT_CHECK(targetObject);
    targetObject->setName(object->name());
}

void MFlatAssignmentVisitor::visitMPackage(const MPackage *package)
{
    visitMObject(package);
}

void MFlatAssignmentVisitor::visitMClass(const MClass *klass)
{
    visitMObject(klass);
    MClass *targetClass = dynamic_cast<MClass *>(m_target);
    QMT_CHECK(targetClass);
    targetClass->setNameSpace(klass->nameSpace());
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
}

void MFlatAssignmentVisitor::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    visitMDiagram(diagram);
}

void MFlatAssignmentVisitor::visitMItem(const MItem *item)
{
    visitMObject(item);
    MItem *targetItem = dynamic_cast<MItem *>(m_target);
    QMT_CHECK(targetItem);
    targetItem->setVarietyEditable(item->isVarietyEditable());
    targetItem->setVariety(item->variety());
    targetItem->setShapeEditable(item->isShapeEditable());
}

void MFlatAssignmentVisitor::visitMRelation(const MRelation *relation)
{
    visitMElement(relation);
    MRelation *targetRelation = dynamic_cast<MRelation *>(m_target);
    QMT_CHECK(targetRelation);
    targetRelation->setName(relation->name());
}

void MFlatAssignmentVisitor::visitMDependency(const MDependency *dependency)
{
    visitMRelation(dependency);
    MDependency *targetDependency = dynamic_cast<MDependency *>(m_target);
    QMT_CHECK(targetDependency);
    targetDependency->setDirection(dependency->direction());
}

void MFlatAssignmentVisitor::visitMInheritance(const MInheritance *inheritance)
{
    visitMRelation(inheritance);
}

void MFlatAssignmentVisitor::visitMAssociation(const MAssociation *association)
{
    visitMRelation(association);
    MAssociation *targetAssociation = dynamic_cast<MAssociation *>(m_target);
    QMT_CHECK(targetAssociation);
    targetAssociation->setEndA(association->endA());
    targetAssociation->setEndB(association->endB());
}

}
