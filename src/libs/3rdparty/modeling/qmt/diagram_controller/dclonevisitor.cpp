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

#include "dclonevisitor.h"

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

DCloneVisitor::DCloneVisitor()
    : m_cloned(0)
{
}

void DCloneVisitor::visitDElement(const DElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(m_cloned);
}

void DCloneVisitor::visitDObject(const DObject *object)
{
    QMT_CHECK(m_cloned);
    visitDElement(object);
}

void DCloneVisitor::visitDPackage(const DPackage *package)
{
    if (!m_cloned)
        m_cloned = new DPackage(*package);
    visitDObject(package);
}

void DCloneVisitor::visitDClass(const DClass *klass)
{
    if (!m_cloned)
        m_cloned = new DClass(*klass);
    visitDObject(klass);
}

void DCloneVisitor::visitDComponent(const DComponent *component)
{
    if (!m_cloned)
        m_cloned = new DComponent(*component);
    visitDObject(component);
}

void DCloneVisitor::visitDDiagram(const DDiagram *diagram)
{
    if (!m_cloned)
        m_cloned = new DDiagram(*diagram);
    visitDObject(diagram);
}

void DCloneVisitor::visitDItem(const DItem *item)
{
    if (!m_cloned)
        m_cloned = new DItem(*item);
    visitDObject(item);

}

void DCloneVisitor::visitDRelation(const DRelation *relation)
{
    QMT_CHECK(m_cloned);
    visitDElement(relation);
}

void DCloneVisitor::visitDInheritance(const DInheritance *inheritance)
{
    if (!m_cloned)
        m_cloned = new DInheritance(*inheritance);
    visitDRelation(inheritance);
}

void DCloneVisitor::visitDDependency(const DDependency *dependency)
{
    if (!m_cloned)
        m_cloned = new DDependency(*dependency);
    visitDRelation(dependency);
}

void DCloneVisitor::visitDAssociation(const DAssociation *association)
{
    if (!m_cloned)
        m_cloned = new DAssociation(*association);
    visitDRelation(association);
}

void DCloneVisitor::visitDAnnotation(const DAnnotation *annotation)
{
    if (!m_cloned)
        m_cloned = new DAnnotation(*annotation);
    visitDElement(annotation);
}

void DCloneVisitor::visitDBoundary(const DBoundary *boundary)
{
    if (!m_cloned)
        m_cloned = new DBoundary(*boundary);
    visitDElement(boundary);
}

DCloneDeepVisitor::DCloneDeepVisitor()
    : m_cloned(0)
{
}

void DCloneDeepVisitor::visitDElement(const DElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(m_cloned);
}

void DCloneDeepVisitor::visitDObject(const DObject *object)
{
    QMT_CHECK(m_cloned);
    visitDElement(object);
}

void DCloneDeepVisitor::visitDPackage(const DPackage *package)
{
    if (!m_cloned)
        m_cloned = new DPackage(*package);
    visitDObject(package);
}

void DCloneDeepVisitor::visitDClass(const DClass *klass)
{
    if (!m_cloned)
        m_cloned = new DClass(*klass);
    visitDObject(klass);
}

void DCloneDeepVisitor::visitDComponent(const DComponent *component)
{
    if (!m_cloned)
        m_cloned = new DComponent(*component);
    visitDObject(component);
}

void DCloneDeepVisitor::visitDDiagram(const DDiagram *diagram)
{
    if (!m_cloned)
        m_cloned = new DDiagram(*diagram);
    visitDObject(diagram);
}

void DCloneDeepVisitor::visitDItem(const DItem *item)
{
    if (!m_cloned)
        m_cloned = new DItem(*item);
    visitDObject(item);
}

void DCloneDeepVisitor::visitDRelation(const DRelation *relation)
{
    QMT_CHECK(m_cloned);
    visitDElement(relation);
}

void DCloneDeepVisitor::visitDInheritance(const DInheritance *inheritance)
{
    if (!m_cloned)
        m_cloned = new DInheritance(*inheritance);
    visitDRelation(inheritance);
}

void DCloneDeepVisitor::visitDDependency(const DDependency *dependency)
{
    if (!m_cloned)
        m_cloned = new DDependency(*dependency);
    visitDRelation(dependency);
}

void DCloneDeepVisitor::visitDAssociation(const DAssociation *association)
{
    if (!m_cloned)
        m_cloned = new DAssociation(*association);
    visitDRelation(association);
}

void DCloneDeepVisitor::visitDAnnotation(const DAnnotation *annotation)
{
    if (!m_cloned)
        m_cloned = new DAnnotation(*annotation);
    visitDElement(annotation);
}

void DCloneDeepVisitor::visitDBoundary(const DBoundary *boundary)
{
    if (!m_cloned)
        m_cloned = new DBoundary(*boundary);
    visitDElement(boundary);
}

} // namespace qmt
