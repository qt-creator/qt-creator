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
    if (!m_cloned)
        m_cloned = new MPackage(*package);
    visitMObject(package);
}

void MCloneVisitor::visitMClass(const MClass *klass)
{
    if (!m_cloned)
        m_cloned = new MClass(*klass);
    visitMObject(klass);
}

void MCloneVisitor::visitMComponent(const MComponent *component)
{
    if (!m_cloned)
        m_cloned = new MComponent(*component);
    visitMObject(component);
}

void MCloneVisitor::visitMDiagram(const MDiagram *diagram)
{
    QMT_CHECK(m_cloned);
    auto cloned = dynamic_cast<MDiagram *>(m_cloned);
    QMT_CHECK(cloned);
    foreach (const DElement *element, diagram->diagramElements()) {
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        DElement *clonedElement = visitor.cloned();
        cloned->addDiagramElement(clonedElement);
    }
    visitMObject(diagram);
}

void MCloneVisitor::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    if (!m_cloned)
        m_cloned = new MCanvasDiagram(*diagram);
    visitMDiagram(diagram);
}

void MCloneVisitor::visitMItem(const MItem *item)
{
    if (!m_cloned)
        m_cloned = new MItem(*item);
    visitMObject(item);
}

void MCloneVisitor::visitMRelation(const MRelation *relation)
{
    QMT_CHECK(m_cloned);
    visitMElement(relation);
}

void MCloneVisitor::visitMDependency(const MDependency *dependency)
{
    if (!m_cloned)
        m_cloned = new MDependency(*dependency);
    visitMRelation(dependency);
}

void MCloneVisitor::visitMInheritance(const MInheritance *inheritance)
{
    if (!m_cloned)
        m_cloned = new MInheritance(*inheritance);
    visitMRelation(inheritance);
}

void MCloneVisitor::visitMAssociation(const MAssociation *association)
{
    if (!m_cloned)
        m_cloned = new MAssociation(*association);
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
    auto cloned = dynamic_cast<MObject *>(m_cloned);
    QMT_CHECK(cloned);
    foreach (const Handle<MObject> &handle, object->children()) {
        if (handle.hasTarget()) {
            MCloneDeepVisitor visitor;
            handle.target()->accept(&visitor);
            auto clonedChild = dynamic_cast<MObject *>(visitor.cloned());
            QMT_CHECK(clonedChild);
            cloned->addChild(clonedChild);
        } else {
            cloned->addChild(handle.uid());
        }
    }
    foreach (const Handle<MRelation> &handle, object->relations()) {
        if (handle.hasTarget()) {
            MCloneDeepVisitor visitor;
            handle.target()->accept(&visitor);
            auto clonedRelation = dynamic_cast<MRelation *>(visitor.cloned());
            QMT_CHECK(clonedRelation);
            cloned->addRelation(clonedRelation);
        } else {
            cloned->addRelation(handle.uid());
        }
    }
}

void MCloneDeepVisitor::visitMPackage(const MPackage *package)
{
    if (!m_cloned)
        m_cloned = new MPackage(*package);
    visitMObject(package);
}

void MCloneDeepVisitor::visitMClass(const MClass *klass)
{
    if (!m_cloned)
        m_cloned = new MClass(*klass);
    visitMObject(klass);
}

void MCloneDeepVisitor::visitMComponent(const MComponent *component)
{
    if (!m_cloned)
        m_cloned = new MComponent(*component);
    visitMObject(component);
}

void MCloneDeepVisitor::visitMDiagram(const MDiagram *diagram)
{
    QMT_CHECK(m_cloned);
    auto cloned = dynamic_cast<MDiagram *>(m_cloned);
    QMT_CHECK(cloned);
    foreach (const DElement *element, diagram->diagramElements()) {
        DCloneDeepVisitor visitor;
        element->accept(&visitor);
        DElement *clonedElement = visitor.cloned();
        cloned->addDiagramElement(clonedElement);
    }
    visitMObject(diagram);
}

void MCloneDeepVisitor::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    if (!m_cloned)
        m_cloned = new MCanvasDiagram(*diagram);
    visitMDiagram(diagram);
}

void MCloneDeepVisitor::visitMItem(const MItem *item)
{
    if (!m_cloned)
        m_cloned = new MItem(*item);
    visitMObject(item);
}

void MCloneDeepVisitor::visitMRelation(const MRelation *relation)
{
    QMT_CHECK(m_cloned);
    visitMElement(relation);
    auto cloned = dynamic_cast<MRelation *>(m_cloned);
    QMT_CHECK(cloned);
    cloned->setEndAUid(relation->endAUid());
    cloned->setEndBUid(relation->endBUid());
}

void MCloneDeepVisitor::visitMDependency(const MDependency *dependency)
{
    if (!m_cloned)
        m_cloned = new MDependency(*dependency);
    visitMRelation(dependency);
}

void MCloneDeepVisitor::visitMInheritance(const MInheritance *inheritance)
{
    if (!m_cloned)
        m_cloned = new MInheritance(*inheritance);
    visitMRelation(inheritance);
}

void MCloneDeepVisitor::visitMAssociation(const MAssociation *association)
{
    if (!m_cloned)
        m_cloned = new MAssociation(*association);
    visitMRelation(association);
}

} // namespace qmt
