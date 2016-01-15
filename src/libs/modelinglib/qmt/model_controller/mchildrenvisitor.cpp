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

namespace qmt {

void MChildrenVisitor::visitMElement(MElement *element)
{
    Q_UNUSED(element);
}

void MChildrenVisitor::visitMObject(MObject *object)
{
    foreach (const Handle<MObject> &handle, object->children()) {
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

} // namespace qmt
