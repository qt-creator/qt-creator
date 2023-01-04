// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dvoidvisitor.h"

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

namespace qmt {

DVoidVisitor::DVoidVisitor()
{
}

void DVoidVisitor::visitDElement(DElement *element)
{
    Q_UNUSED(element)
}

void DVoidVisitor::visitDObject(DObject *object)
{
    visitDElement(object);
}

void DVoidVisitor::visitDPackage(DPackage *package)
{
    visitDObject(package);
}

void DVoidVisitor::visitDClass(DClass *klass)
{
    visitDObject(klass);
}

void DVoidVisitor::visitDComponent(DComponent *component)
{
    visitDObject(component);
}

void DVoidVisitor::visitDDiagram(DDiagram *diagram)
{
    visitDObject(diagram);
}

void DVoidVisitor::visitDItem(DItem *item)
{
    visitDObject(item);
}

void DVoidVisitor::visitDRelation(DRelation *relation)
{
    visitDElement(relation);
}

void DVoidVisitor::visitDInheritance(DInheritance *inheritance)
{
    visitDRelation(inheritance);
}

void DVoidVisitor::visitDDependency(DDependency *dependency)
{
    visitDRelation(dependency);
}

void DVoidVisitor::visitDAssociation(DAssociation *association)
{
    visitDRelation(association);
}

void DVoidVisitor::visitDConnection(DConnection *connection)
{
    visitDRelation(connection);
}

void DVoidVisitor::visitDAnnotation(DAnnotation *annotation)
{
    visitDElement(annotation);
}

void DVoidVisitor::visitDBoundary(DBoundary *boundary)
{
    visitDElement(boundary);
}

void DVoidVisitor::visitDSwimlane(DSwimlane *swimlane)
{
    visitDElement(swimlane);
}

DConstVoidVisitor::DConstVoidVisitor()
{
}

void DConstVoidVisitor::visitDElement(const DElement *element)
{
    Q_UNUSED(element)
}

void DConstVoidVisitor::visitDObject(const DObject *object)
{
    visitDElement(object);
}

void DConstVoidVisitor::visitDPackage(const DPackage *package)
{
    visitDObject(package);
}

void DConstVoidVisitor::visitDClass(const DClass *klass)
{
    visitDObject(klass);
}

void DConstVoidVisitor::visitDComponent(const DComponent *component)
{
    visitDObject(component);
}

void DConstVoidVisitor::visitDDiagram(const DDiagram *diagram)
{
    visitDObject(diagram);
}

void DConstVoidVisitor::visitDItem(const DItem *item)
{
    visitDObject(item);
}

void DConstVoidVisitor::visitDRelation(const DRelation *relation)
{
    visitDElement(relation);
}

void DConstVoidVisitor::visitDInheritance(const DInheritance *inheritance)
{
    visitDRelation(inheritance);
}

void DConstVoidVisitor::visitDDependency(const DDependency *dependency)
{
    visitDRelation(dependency);
}

void DConstVoidVisitor::visitDAssociation(const DAssociation *association)
{
    visitDRelation(association);
}

void DConstVoidVisitor::visitDConnection(const DConnection *connection)
{
    visitDRelation(connection);
}

void DConstVoidVisitor::visitDAnnotation(const DAnnotation *annotation)
{
    visitDElement(annotation);
}

void DConstVoidVisitor::visitDBoundary(const DBoundary *boundary)
{
    visitDElement(boundary);
}

void DConstVoidVisitor::visitDSwimlane(const DSwimlane *swimlane)
{
    visitDElement(swimlane);
}

} // namespace qmt
