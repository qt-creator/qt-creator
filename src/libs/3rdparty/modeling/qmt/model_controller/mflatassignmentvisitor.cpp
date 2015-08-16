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
    : _target(target)
{
    QMT_CHECK(_target);
}

void MFlatAssignmentVisitor::visitMElement(const MElement *element)
{
    _target->setStereotypes(element->getStereotypes());
}

void MFlatAssignmentVisitor::visitMObject(const MObject *object)
{
    visitMElement(object);
    MObject *target_object = dynamic_cast<MObject *>(_target);
    QMT_CHECK(target_object);
    target_object->setName(object->getName());
}

void MFlatAssignmentVisitor::visitMPackage(const MPackage *package)
{
    visitMObject(package);
}

void MFlatAssignmentVisitor::visitMClass(const MClass *klass)
{
    visitMObject(klass);
    MClass *target_class = dynamic_cast<MClass *>(_target);
    QMT_CHECK(target_class);
    target_class->setNamespace(klass->getNamespace());
    target_class->setTemplateParameters(klass->getTemplateParameters());
    target_class->setMembers(klass->getMembers());
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
    MItem *target_item = dynamic_cast<MItem *>(_target);
    QMT_CHECK(target_item);
    target_item->setVarietyEditable(item->isVarietyEditable());
    target_item->setVariety(item->getVariety());
    target_item->setShapeEditable(item->isShapeEditable());
}

void MFlatAssignmentVisitor::visitMRelation(const MRelation *relation)
{
    visitMElement(relation);
    MRelation *target_relation = dynamic_cast<MRelation *>(_target);
    QMT_CHECK(target_relation);
    target_relation->setName(relation->getName());
}

void MFlatAssignmentVisitor::visitMDependency(const MDependency *dependency)
{
    visitMRelation(dependency);
    MDependency *target_dependency = dynamic_cast<MDependency *>(_target);
    QMT_CHECK(target_dependency);
    target_dependency->setDirection(dependency->getDirection());
}

void MFlatAssignmentVisitor::visitMInheritance(const MInheritance *inheritance)
{
    visitMRelation(inheritance);
}

void MFlatAssignmentVisitor::visitMAssociation(const MAssociation *association)
{
    visitMRelation(association);
    MAssociation *target_association = dynamic_cast<MAssociation *>(_target);
    QMT_CHECK(target_association);
    target_association->setA(association->getA());
    target_association->setB(association->getB());
}

}
