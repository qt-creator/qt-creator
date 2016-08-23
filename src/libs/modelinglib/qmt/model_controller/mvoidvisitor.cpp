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

#include "mvoidvisitor.h"

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
#include "qmt/model/mconnection.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/minheritance.h"

#include <QtGlobal>

namespace qmt {

void MVoidVisitor::visitMElement(MElement *element)
{
    Q_UNUSED(element);
}

void MVoidVisitor::visitMObject(MObject *object)
{
    visitMElement(object);
}

void MVoidVisitor::visitMPackage(MPackage *package)
{
    visitMObject(package);
}

void MVoidVisitor::visitMClass(MClass *klass)
{
    visitMObject(klass);
}

void MVoidVisitor::visitMComponent(MComponent *component)
{
    visitMObject(component);
}

void MVoidVisitor::visitMDiagram(MDiagram *diagram)
{
    visitMObject(diagram);
}

void MVoidVisitor::visitMCanvasDiagram(MCanvasDiagram *diagram)
{
    visitMDiagram(diagram);
}

void MVoidVisitor::visitMItem(MItem *item)
{
    visitMObject(item);
}

void MVoidVisitor::visitMRelation(MRelation *relation)
{
    visitMElement(relation);
}

void MVoidVisitor::visitMDependency(MDependency *dependency)
{
    visitMRelation(dependency);
}

void MVoidVisitor::visitMInheritance(MInheritance *inheritance)
{
    visitMRelation(inheritance);
}

void MVoidVisitor::visitMAssociation(MAssociation *association)
{
    visitMRelation(association);
}

void MVoidVisitor::visitMConnection(MConnection *connection)
{
    visitMRelation(connection);
}

void MVoidConstVisitor::visitMElement(const MElement *element)
{
    Q_UNUSED(element);
}

void MVoidConstVisitor::visitMObject(const MObject *object)
{
    visitMElement(object);
}

void MVoidConstVisitor::visitMPackage(const MPackage *package)
{
    visitMObject(package);
}

void MVoidConstVisitor::visitMClass(const MClass *klass)
{
    visitMObject(klass);
}

void MVoidConstVisitor::visitMComponent(const MComponent *component)
{
    visitMObject(component);
}

void MVoidConstVisitor::visitMDiagram(const MDiagram *diagram)
{
    visitMObject(diagram);
}

void MVoidConstVisitor::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    visitMDiagram(diagram);
}

void MVoidConstVisitor::visitMItem(const MItem *item)
{
    visitMObject(item);
}

void MVoidConstVisitor::visitMRelation(const MRelation *relation)
{
    visitMElement(relation);
}

void MVoidConstVisitor::visitMDependency(const MDependency *dependency)
{
    visitMRelation(dependency);
}

void MVoidConstVisitor::visitMInheritance(const MInheritance *inheritance)
{
    visitMRelation(inheritance);
}

void MVoidConstVisitor::visitMAssociation(const MAssociation *association)
{
    visitMRelation(association);
}

void MVoidConstVisitor::visitMConnection(const MConnection *connection)
{
    visitMRelation(connection);
}

} // namespace qmt
