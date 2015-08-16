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

MChildrenVisitor::MChildrenVisitor()
{
}

void MChildrenVisitor::visitMElement(MElement *element)
{
    Q_UNUSED(element);
}

void MChildrenVisitor::visitMObject(MObject *object)
{
    foreach (const Handle<MObject> &handle, object->getChildren()) {
        MObject *child = handle.getTarget();
        if (child) {
            child->accept(this);
        }
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

}
