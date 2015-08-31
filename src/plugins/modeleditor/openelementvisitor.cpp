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

#include "openelementvisitor.h"

#include "elementtasks.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram/ddiagram.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram/ddependency.h"
#include "qmt/diagram/dinheritance.h"
#include "qmt/diagram/dassociation.h"

#include "qmt/model/melement.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mitem.h"

#include "qmt/model_controller/modelcontroller.h"

namespace ModelEditor {
namespace Internal {

void OpenDiagramElementVisitor::setModelController(qmt::ModelController *modelController)
{
    m_modelController = modelController;
}

void OpenDiagramElementVisitor::setElementTasks(ElementTasks *elementTasks)
{
    m_elementTasks = elementTasks;
}

void OpenDiagramElementVisitor::visitDElement(const qmt::DElement *element)
{
    qmt::MElement *modelElement = m_modelController->findElement(element->getModelUid());
    if (modelElement) {
        OpenModelElementVisitor visitor;
        visitor.setElementTasks(m_elementTasks);
        modelElement->accept(&visitor);
    }
}

void OpenDiagramElementVisitor::visitDObject(const qmt::DObject *object)
{
    visitDElement(object);
}

void OpenDiagramElementVisitor::visitDPackage(const qmt::DPackage *package)
{
    visitDObject(package);
}

void OpenDiagramElementVisitor::visitDClass(const qmt::DClass *klass)
{
    visitDObject(klass);
}

void OpenDiagramElementVisitor::visitDComponent(const qmt::DComponent *component)
{
    visitDObject(component);
}

void OpenDiagramElementVisitor::visitDDiagram(const qmt::DDiagram *diagram)
{
    visitDObject(diagram);
}

void OpenDiagramElementVisitor::visitDItem(const qmt::DItem *item)
{
    visitDObject(item);
}

void OpenDiagramElementVisitor::visitDRelation(const qmt::DRelation *relation)
{
    visitDElement(relation);
}

void OpenDiagramElementVisitor::visitDInheritance(const qmt::DInheritance *inheritance)
{
    visitDRelation(inheritance);
}

void OpenDiagramElementVisitor::visitDDependency(const qmt::DDependency *dependency)
{
    visitDRelation(dependency);
}

void OpenDiagramElementVisitor::visitDAssociation(const qmt::DAssociation *association)
{
    visitDRelation(association);
}

void OpenDiagramElementVisitor::visitDAnnotation(const qmt::DAnnotation *annotation)
{
    Q_UNUSED(annotation);
}

void OpenDiagramElementVisitor::visitDBoundary(const qmt::DBoundary *boundary)
{
    Q_UNUSED(boundary);
}

void OpenModelElementVisitor::setElementTasks(ElementTasks *elementTasks)
{
    m_elementTasks = elementTasks;
}

void OpenModelElementVisitor::visitMElement(const qmt::MElement *element)
{
    Q_UNUSED(element);
}

void OpenModelElementVisitor::visitMObject(const qmt::MObject *object)
{
    Q_UNUSED(object);
}

void OpenModelElementVisitor::visitMPackage(const qmt::MPackage *package)
{
    if (m_elementTasks->hasDiagram(package))
        m_elementTasks->openDiagram(package);
    else if (m_elementTasks->mayCreateDiagram(package))
        m_elementTasks->createAndOpenDiagram(package);
}

void OpenModelElementVisitor::visitMClass(const qmt::MClass *klass)
{
    m_elementTasks->openClassDefinition(klass);
}

void OpenModelElementVisitor::visitMComponent(const qmt::MComponent *component)
{
    m_elementTasks->openSourceFile(component);
}

void OpenModelElementVisitor::visitMDiagram(const qmt::MDiagram *diagram)
{
    m_elementTasks->openDiagram(diagram);
}

void OpenModelElementVisitor::visitMCanvasDiagram(const qmt::MCanvasDiagram *diagram)
{
    visitMDiagram(diagram);
}

void OpenModelElementVisitor::visitMItem(const qmt::MItem *item)
{
    visitMObject(item);
}

void OpenModelElementVisitor::visitMRelation(const qmt::MRelation *relation)
{
    Q_UNUSED(relation);
}

void OpenModelElementVisitor::visitMDependency(const qmt::MDependency *dependency)
{
    Q_UNUSED(dependency);
}

void OpenModelElementVisitor::visitMInheritance(const qmt::MInheritance *inheritance)
{
    Q_UNUSED(inheritance);
}

void OpenModelElementVisitor::visitMAssociation(const qmt::MAssociation *association)
{
    Q_UNUSED(association);
}

} // namespace Internal
} // namespace ModelEditor
