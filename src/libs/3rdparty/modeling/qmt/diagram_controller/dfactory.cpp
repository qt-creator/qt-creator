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
    : _product(0)
{
}

void DFactory::visitMElement(const MElement *element)
{
    Q_UNUSED(element);
    QMT_CHECK(_product);
}

void DFactory::visitMObject(const MObject *object)
{
    DObject *diagram_object = dynamic_cast<DObject *>(_product);
    QMT_CHECK(diagram_object);
    diagram_object->setModelUid(object->getUid());
    visitMElement(object);
}

void DFactory::visitMPackage(const MPackage *package)
{
    QMT_CHECK(!_product);
    DPackage *diagram_package = new DPackage();
    _product = diagram_package;
    visitMObject(package);
}

void DFactory::visitMClass(const MClass *klass)
{
    QMT_CHECK(!_product);
    DClass *diagram_klass = new DClass();
    _product = diagram_klass;
    visitMObject(klass);
}

void DFactory::visitMComponent(const MComponent *component)
{
    QMT_CHECK(!_product);
    DComponent *diagram_component = new DComponent();
    _product = diagram_component;
    visitMObject(component);
}

void DFactory::visitMDiagram(const MDiagram *diagram)
{
    QMT_CHECK(!_product);
    DDiagram *diagram_diagram = new DDiagram();
    _product = diagram_diagram;
    visitMObject(diagram);
}

void DFactory::visitMCanvasDiagram(const MCanvasDiagram *diagram)
{
    QMT_CHECK(!_product);
    visitMDiagram(diagram);
}

void DFactory::visitMItem(const MItem *item)
{
    QMT_CHECK(!_product);
    DItem *diagram_item = new DItem();
    _product = diagram_item;
    visitMObject(item);
}

void DFactory::visitMRelation(const MRelation *relation)
{
    DRelation *diagram_relation = dynamic_cast<DRelation *>(_product);
    QMT_CHECK(diagram_relation);
    diagram_relation->setModelUid(relation->getUid());
    visitMElement(relation);
}

void DFactory::visitMDependency(const MDependency *dependency)
{
    QMT_CHECK(!_product);
    DDependency *diagram_dependency = new DDependency();
    _product = diagram_dependency;
    visitMRelation(dependency);
}

void DFactory::visitMInheritance(const MInheritance *inheritance)
{
    QMT_CHECK(!_product);
    DInheritance *diagram_inheritance = new DInheritance();
    _product = diagram_inheritance;
    visitMRelation(inheritance);
}

void DFactory::visitMAssociation(const MAssociation *association)
{
    QMT_CHECK(!_product);
    DAssociation *diagram_association = new DAssociation();
    _product = diagram_association;
    visitMRelation(association);
}

}
