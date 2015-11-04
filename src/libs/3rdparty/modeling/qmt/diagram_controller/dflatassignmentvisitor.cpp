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

#include "dflatassignmentvisitor.h"

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

// TODO may flat assignment visitor use operator=() ?

DFlatAssignmentVisitor::DFlatAssignmentVisitor(DElement *target)
    : m_target(target)
{
    QMT_CHECK(target);
}

void DFlatAssignmentVisitor::visitDElement(const DElement *element)
{
    Q_UNUSED(element);
}

void DFlatAssignmentVisitor::visitDObject(const DObject *object)
{
    visitDElement(object);
    DObject *target = dynamic_cast<DObject *>(m_target);
    QMT_CHECK(target);
    target->setStereotypes(object->stereotypes());
    target->setName(object->name());
    target->setPos(object->pos());
    target->setRect(object->rect());
    target->setAutoSize(object->hasAutoSize());
    target->setDepth(object->depth());
    target->setVisualPrimaryRole(object->visualPrimaryRole());
    target->setVisualSecondaryRole(object->visualSecondaryRole());
    target->setVisualEmphasized(object->isVisualEmphasized());
    target->setStereotypeDisplay(object->stereotypeDisplay());
}

void DFlatAssignmentVisitor::visitDPackage(const DPackage *package)
{
    visitDObject(package);
}

void DFlatAssignmentVisitor::visitDClass(const DClass *klass)
{
    visitDObject(klass);
    DClass *target = dynamic_cast<DClass *>(m_target);
    QMT_CHECK(target);
    target->setNameSpace(klass->nameSpace());
    target->setTemplateParameters(klass->templateParameters());
    target->setTemplateDisplay(klass->templateDisplay());
    target->setMembers(klass->members());
    target->setShowAllMembers(klass->showAllMembers());
    target->setVisibleMembers(klass->visibleMembers());
}

void DFlatAssignmentVisitor::visitDComponent(const DComponent *component)
{
    visitDObject(component);
    DComponent *target = dynamic_cast<DComponent *>(m_target);
    QMT_CHECK(target);
    target->setPlainShape(component->plainShape());
}

void DFlatAssignmentVisitor::visitDDiagram(const DDiagram *diagram)
{
    visitDObject(diagram);
}

void DFlatAssignmentVisitor::visitDItem(const DItem *item)
{
    visitDObject(item);
    DItem *target = dynamic_cast<DItem *>(m_target);
    QMT_CHECK(target);
    target->setVariety(target->variety());
    target->setShapeEditable(target->isShapeEditable());
    target->setShape(target->shape());
}

void DFlatAssignmentVisitor::visitDRelation(const DRelation *relation)
{
    visitDElement(relation);
    DRelation *target = dynamic_cast<DRelation *>(m_target);
    QMT_CHECK(target);
    target->setStereotypes(relation->stereotypes());
    target->setIntermediatePoints(relation->intermediatePoints());
}

void DFlatAssignmentVisitor::visitDInheritance(const DInheritance *inheritance)
{
    visitDRelation(inheritance);
}

void DFlatAssignmentVisitor::visitDDependency(const DDependency *dependency)
{
    visitDRelation(dependency);
    DDependency *target = dynamic_cast<DDependency *>(m_target);
    QMT_CHECK(target);
    target->setDirection(dependency->direction());
}

void DFlatAssignmentVisitor::visitDAssociation(const DAssociation *association)
{
    visitDRelation(association);
    DAssociation *target = dynamic_cast<DAssociation *>(m_target);
    QMT_CHECK(target);
    target->setEndA(association->endA());
    target->setEndB(association->endB());
}

void DFlatAssignmentVisitor::visitDAnnotation(const DAnnotation *annotation)
{
    visitDElement(annotation);
    DAnnotation *target = dynamic_cast<DAnnotation *>(m_target);
    target->setText(annotation->text());
    target->setPos(annotation->pos());
    target->setRect(annotation->rect());
    target->setAutoSize(annotation->hasAutoSize());
    target->setVisualRole(annotation->visualRole());
}

void DFlatAssignmentVisitor::visitDBoundary(const DBoundary *boundary)
{
    visitDElement(boundary);
    DBoundary *target = dynamic_cast<DBoundary *>(m_target);
    target->setText(boundary->text());
    target->setPos(boundary->pos());
    target->setRect(boundary->rect());
}

}
