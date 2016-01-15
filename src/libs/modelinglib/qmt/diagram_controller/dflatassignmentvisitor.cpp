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
    auto target = dynamic_cast<DObject *>(m_target);
    QMT_CHECK(target);
    target->setStereotypes(object->stereotypes());
    target->setName(object->name());
    target->setPos(object->pos());
    target->setRect(object->rect());
    target->setAutoSized(object->isAutoSized());
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
    auto target = dynamic_cast<DClass *>(m_target);
    QMT_CHECK(target);
    target->setUmlNamespace(klass->umlNamespace());
    target->setTemplateParameters(klass->templateParameters());
    target->setTemplateDisplay(klass->templateDisplay());
    target->setMembers(klass->members());
    target->setShowAllMembers(klass->showAllMembers());
    target->setVisibleMembers(klass->visibleMembers());
}

void DFlatAssignmentVisitor::visitDComponent(const DComponent *component)
{
    visitDObject(component);
    auto target = dynamic_cast<DComponent *>(m_target);
    QMT_CHECK(target);
    target->setPlainShape(component->isPlainShape());
}

void DFlatAssignmentVisitor::visitDDiagram(const DDiagram *diagram)
{
    visitDObject(diagram);
}

void DFlatAssignmentVisitor::visitDItem(const DItem *item)
{
    visitDObject(item);
    auto target = dynamic_cast<DItem *>(m_target);
    QMT_CHECK(target);
    target->setVariety(target->variety());
    target->setShapeEditable(target->isShapeEditable());
    target->setShape(target->shape());
}

void DFlatAssignmentVisitor::visitDRelation(const DRelation *relation)
{
    visitDElement(relation);
    auto target = dynamic_cast<DRelation *>(m_target);
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
    auto target = dynamic_cast<DDependency *>(m_target);
    QMT_CHECK(target);
    target->setDirection(dependency->direction());
}

void DFlatAssignmentVisitor::visitDAssociation(const DAssociation *association)
{
    visitDRelation(association);
    auto target = dynamic_cast<DAssociation *>(m_target);
    QMT_CHECK(target);
    target->setEndA(association->endA());
    target->setEndB(association->endB());
}

void DFlatAssignmentVisitor::visitDAnnotation(const DAnnotation *annotation)
{
    visitDElement(annotation);
    auto target = dynamic_cast<DAnnotation *>(m_target);
    target->setText(annotation->text());
    target->setPos(annotation->pos());
    target->setRect(annotation->rect());
    target->setAutoSized(annotation->isAutoSized());
    target->setVisualRole(annotation->visualRole());
}

void DFlatAssignmentVisitor::visitDBoundary(const DBoundary *boundary)
{
    visitDElement(boundary);
    auto target = dynamic_cast<DBoundary *>(m_target);
    target->setText(boundary->text());
    target->setPos(boundary->pos());
    target->setRect(boundary->rect());
}

} // namespace qmt
