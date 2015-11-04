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

#include "mclonevisitor.h"

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
#include "qmt/model/mdependency.h"
#include "qmt/model/minheritance.h"

#include "qmt/diagram/delement.h"
#include "qmt/diagram_controller/dclonevisitor.h"


namespace qmt {

MCloneVisitor::MCloneVisitor()
    : m_cloned(0)
{
}

void MCloneVisitor::visitMElement(const MElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(m_cloned);
}

void MCloneVisitor::visitMObject(const MObject *object)
{
    QMT_CHECK(m_cloned);
    visitMElement(object);
}

void MCloneVisitor::visitMPackage(const MPackage *package)
{
    if (!m_cloned) {
        m_cloned = new MPackage(*package);
    }
    visitMObject(package);
}

void MCloneVisitor::visitMClass(const MClass *klass)
{
    if (!m_cloned) {
        m_cloned = new MClass(*klass);
    }
    visitMObject(klass);
}

void MCloneVisitor::visitMComponent(const MComponent *component)
{
    if (!m_cloned) {
        m_cloned = new MComponent(*component);
    }
    visitMObject(component);
}

void MCloneVisitor::visitMDiagram(const MDiagram *diagram)
{
    QMT_CHECK(m_cloned);
    MDiagram *cloned = dynamic_cast<MDiagram *>(m_cloned);
    QMT_CHECK(cloned);
    foreach (const DElement *element, diagram->getDiagramElements()) {
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        DElement *clonedElement = visitor.getCloned();
        cloned->addDiagramElement(clonedElement);
    }
    visitMObject(diagram);
}

void MCloneVisitor::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    if (!m_cloned) {
        m_cloned = new MCanvasDiagram(*diagram);
    }
    visitMDiagram(diagram);
}

void MCloneVisitor::visitMItem(const MItem *item)
{
    if (!m_cloned) {
        m_cloned = new MItem(*item);
    }
    visitMObject(item);
}

void MCloneVisitor::visitMRelation(const MRelation *relation)
{
    QMT_CHECK(m_cloned);
    visitMElement(relation);
}

void MCloneVisitor::visitMDependency(const MDependency *dependency)
{
    if (!m_cloned) {
        m_cloned = new MDependency(*dependency);
    }
    visitMRelation(dependency);
}

void MCloneVisitor::visitMInheritance(const MInheritance *inheritance)
{
    if (!m_cloned) {
        m_cloned = new MInheritance(*inheritance);
    }
    visitMRelation(inheritance);
}

void MCloneVisitor::visitMAssociation(const MAssociation *association)
{
    if (!m_cloned) {
        m_cloned = new MAssociation(*association);
    }
    visitMRelation(association);
}


MCloneDeepVisitor::MCloneDeepVisitor()
    : m_cloned(0)
{
}

void MCloneDeepVisitor::visitMElement(const MElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(m_cloned);
}

void MCloneDeepVisitor::visitMObject(const MObject *object)
{
    QMT_CHECK(m_cloned);
    visitMElement(object);
    MObject *cloned = dynamic_cast<MObject *>(m_cloned);
    QMT_CHECK(cloned);
    foreach (const Handle<MObject> &handle, object->getChildren()) {
        if (handle.hasTarget()) {
            MCloneDeepVisitor visitor;
            handle.getTarget()->accept(&visitor);
            MObject *clonedChild = dynamic_cast<MObject *>(visitor.getCloned());
            QMT_CHECK(clonedChild);
            cloned->addChild(clonedChild);
        } else {
            cloned->addChild(handle.getUid());
        }
    }
    foreach (const Handle<MRelation> &handle, object->getRelations()) {
        if (handle.hasTarget()) {
            MCloneDeepVisitor visitor;
            handle.getTarget()->accept(&visitor);
            MRelation *clonedRelation = dynamic_cast<MRelation *>(visitor.getCloned());
            QMT_CHECK(clonedRelation);
            cloned->addRelation(clonedRelation);
        } else {
            cloned->addRelation(handle.getUid());
        }
    }
}

void MCloneDeepVisitor::visitMPackage(const MPackage *package)
{
    if (!m_cloned) {
        m_cloned = new MPackage(*package);
    }
    visitMObject(package);
}

void MCloneDeepVisitor::visitMClass(const MClass *klass)
{
    if (!m_cloned) {
        m_cloned = new MClass(*klass);
    }
    visitMObject(klass);
}

void MCloneDeepVisitor::visitMComponent(const MComponent *component)
{
    if (!m_cloned) {
        m_cloned = new MComponent(*component);
    }
    visitMObject(component);
}

void MCloneDeepVisitor::visitMDiagram(const MDiagram *diagram)
{
    QMT_CHECK(m_cloned);
    MDiagram *cloned = dynamic_cast<MDiagram *>(m_cloned);
    QMT_CHECK(cloned);
    foreach (const DElement *element, diagram->getDiagramElements()) {
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        DElement *clonedElement = visitor.getCloned();
        cloned->addDiagramElement(clonedElement);
    }
    visitMObject(diagram);
}

void MCloneDeepVisitor::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    if (!m_cloned) {
        m_cloned = new MCanvasDiagram(*diagram);
    }
    visitMDiagram(diagram);
}

void MCloneDeepVisitor::visitMItem(const MItem *item)
{
    if (!m_cloned) {
        m_cloned = new MItem(*item);
    }
    visitMObject(item);
}

void MCloneDeepVisitor::visitMRelation(const MRelation *relation)
{
    QMT_CHECK(m_cloned);
    visitMElement(relation);
    MRelation *cloned = dynamic_cast<MRelation *>(m_cloned);
    QMT_CHECK(cloned);
    cloned->setEndA(relation->getEndA());
    cloned->setEndB(relation->getEndB());
}

void MCloneDeepVisitor::visitMDependency(const MDependency *dependency)
{
    if (!m_cloned) {
        m_cloned = new MDependency(*dependency);
    }
    visitMRelation(dependency);
}

void MCloneDeepVisitor::visitMInheritance(const MInheritance *inheritance)
{
    if (!m_cloned) {
        m_cloned = new MInheritance(*inheritance);
    }
    visitMRelation(inheritance);
}

void MCloneDeepVisitor::visitMAssociation(const MAssociation *association)
{
    if (!m_cloned) {
        m_cloned = new MAssociation(*association);
    }
    visitMRelation(association);
}

}
