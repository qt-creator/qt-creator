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

#include "dfactory.h"

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

namespace qmt {

DFactory::DFactory()
    : m_product(0)
{
}

void DFactory::visitMElement(const MElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(m_product);
}

void DFactory::visitMObject(const MObject *object)
{
    auto diagramObject = dynamic_cast<DObject *>(m_product);
    QMT_CHECK(diagramObject);
    diagramObject->setModelUid(object->uid());
    visitMElement(object);
}

void DFactory::visitMPackage(const MPackage *package)
{
    QMT_CHECK(!m_product);
    auto diagramPackage = new DPackage();
    m_product = diagramPackage;
    visitMObject(package);
}

void DFactory::visitMClass(const MClass *klass)
{
    QMT_CHECK(!m_product);
    auto diagramKlass = new DClass();
    m_product = diagramKlass;
    visitMObject(klass);
}

void DFactory::visitMComponent(const MComponent *component)
{
    QMT_CHECK(!m_product);
    auto diagramComponent = new DComponent();
    m_product = diagramComponent;
    visitMObject(component);
}

void DFactory::visitMDiagram(const MDiagram *diagram)
{
    QMT_CHECK(!m_product);
    auto diagramDiagram = new DDiagram();
    m_product = diagramDiagram;
    visitMObject(diagram);
}

void DFactory::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    QMT_CHECK(!m_product);
    visitMDiagram(diagram);
}

void DFactory::visitMItem(const MItem *item)
{
    QMT_CHECK(!m_product);
    auto diagramItem = new DItem();
    m_product = diagramItem;
    visitMObject(item);
}

void DFactory::visitMRelation(const MRelation *relation)
{
    auto diagramRelation = dynamic_cast<DRelation *>(m_product);
    QMT_CHECK(diagramRelation);
    diagramRelation->setModelUid(relation->uid());
    visitMElement(relation);
}

void DFactory::visitMDependency(const MDependency *dependency)
{
    QMT_CHECK(!m_product);
    auto diagramDependency = new DDependency();
    m_product = diagramDependency;
    visitMRelation(dependency);
}

void DFactory::visitMInheritance(const MInheritance *inheritance)
{
    QMT_CHECK(!m_product);
    auto diagramInheritance = new DInheritance();
    m_product = diagramInheritance;
    visitMRelation(inheritance);
}

void DFactory::visitMAssociation(const MAssociation *association)
{
    QMT_CHECK(!m_product);
    auto diagramAssociation = new DAssociation();
    m_product = diagramAssociation;
    visitMRelation(association);
}

} // namespace qmt
