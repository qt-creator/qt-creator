// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mchildrenvisitor.h"

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

void MChildrenVisitor::visitMElement(MElement *element)
{
    Q_UNUSED(element)
}

void MChildrenVisitor::visitMObject(MObject *object)
{
    for (const Handle<MObject> &handle : object->children()) {
        MObject *child = handle.target();
        if (child)
            child->accept(this);
    }
    visitMElement(object);
}

void MChildrenVisitor::visitMPackage(MPackage *package)
{
    visitMObject(package);
}

void MChildrenVisitor::visitMClass(MClass *klass)
{
    visitMObject(klass);
}

void MChildrenVisitor::visitMComponent(MComponent *component)
{
    visitMObject(component);
}

void MChildrenVisitor::visitMDiagram(MDiagram *diagram)
{
    visitMObject(diagram);
}

void MChildrenVisitor::visitMCanvasDiagram(MCanvasDiagram *diagram)
{
    visitMDiagram(diagram);
}

void MChildrenVisitor::visitMItem(MItem *item)
{
    visitMObject(item);
}

void MChildrenVisitor::visitMRelation(MRelation *relation)
{
    visitMElement(relation);
}

void MChildrenVisitor::visitMDependency(MDependency *dependency)
{
    visitMRelation(dependency);
}

void MChildrenVisitor::visitMInheritance(MInheritance *inheritance)
{
    visitMRelation(inheritance);
}

void MChildrenVisitor::visitMAssociation(MAssociation *association)
{
    visitMRelation(association);
}

void MChildrenVisitor::visitMConnection(MConnection *connection)
{
    visitMRelation(connection);
}

} // namespace qmt
