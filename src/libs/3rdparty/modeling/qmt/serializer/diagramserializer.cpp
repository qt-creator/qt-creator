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

#include "diagramserializer.h"

#include "infrastructureserializer.h"
#include "modelserializer.h"

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

#include "qark/qxmloutarchive.h"
#include "qark/qxmlinarchive.h"
#include "qark/serialize.h"

using namespace qmt;


namespace qark {


// DElement

QARK_REGISTER_TYPE_NAME(DElement, "DElement")
QARK_ACCESS_SERIALIZE(DElement)

template<class Archive>
inline void Access<Archive, DElement>::serialize(Archive &archive, DElement &element)
{
    archive || tag(element)
            || attr(QStringLiteral("uid"), element, &DElement::getUid, &DElement::setUid)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DElement)


// DObject

// functions for backward compatibility to old visual role
static DObject::VisualPrimaryRole getVisualRole(const DObject &object)
{
    DObject::VisualPrimaryRole visualRole = object.getVisualPrimaryRole();
    if (visualRole == DObject::DEPRECATED_PRIMARY_ROLE_DARKER
            || visualRole == DObject::DEPRECATED_PRIMARY_ROLE_LIGHTER
            || visualRole == DObject::DEPRECATED_PRIMARY_ROLE_OUTLINE
            || visualRole == DObject::DEPRECATED_PRIMARY_ROLE_SOFTEN) {
        QMT_CHECK(false);
        visualRole = DObject::PRIMARY_ROLE_NORMAL;
    }
    return visualRole;
}

static void setVisualRole(DObject &object, DObject::VisualPrimaryRole visualRole)
{
    if (visualRole == DObject::DEPRECATED_PRIMARY_ROLE_DARKER) {
        object.setVisualPrimaryRole(DObject::PRIMARY_ROLE_NORMAL);
        object.setVisualSecondaryRole(DObject::SECONDARY_ROLE_DARKER);
    } else if (visualRole == DObject::DEPRECATED_PRIMARY_ROLE_LIGHTER) {
        object.setVisualPrimaryRole(DObject::PRIMARY_ROLE_NORMAL);
        object.setVisualSecondaryRole(DObject::SECONDARY_ROLE_LIGHTER);
    } else if (visualRole == DObject::DEPRECATED_PRIMARY_ROLE_OUTLINE) {
        object.setVisualPrimaryRole(DObject::PRIMARY_ROLE_NORMAL);
        object.setVisualSecondaryRole(DObject::SECONDARY_ROLE_OUTLINE);
    } else if (visualRole == DObject::DEPRECATED_PRIMARY_ROLE_SOFTEN) {
        object.setVisualPrimaryRole(DObject::PRIMARY_ROLE_NORMAL);
        object.setVisualSecondaryRole(DObject::SECONDARY_ROLE_SOFTEN);
    } else {
        object.setVisualPrimaryRole(visualRole);
    }
}

QARK_REGISTER_TYPE_NAME(DObject, "DObject")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DObject, DElement)
QARK_ACCESS_SERIALIZE(DObject)

template<class Archive>
inline void Access<Archive, DObject>::serialize(Archive &archive, DObject &object)
{
    archive || tag(object)
            || base<DElement>(object)
            || attr(QStringLiteral("object"), object, &DObject::getModelUid, &DObject::setModelUid)
            || attr(QStringLiteral("stereotypes"), object, &DObject::getStereotypes, &DObject::setStereotypes)
            || attr(QStringLiteral("context"), object, &DObject::getContext, &DObject::setContext)
            || attr(QStringLiteral("name"), object, &DObject::getName, &DObject::setName)
            || attr(QStringLiteral("pos"), object, &DObject::getPos, &DObject::setPos)
            || attr(QStringLiteral("rect"), object, &DObject::getRect, &DObject::setRect)
            || attr(QStringLiteral("auto-sized"), object, &DObject::hasAutoSize, &DObject::setAutoSize)
            || attr(QStringLiteral("visual-role"), object, &getVisualRole, &setVisualRole)
            || attr(QStringLiteral("visual-role2"), object, &DObject::getVisualSecondaryRole, &DObject::setVisualSecondaryRole)
            || attr(QStringLiteral("visual-emphasized"), object, &DObject::isVisualEmphasized, &DObject::setVisualEmphasized)
            || attr(QStringLiteral("stereotype-display"), object, &DObject::getStereotypeDisplay, &DObject::setStereotypeDisplay)
            // depth is not persistent
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DObject)


// DPackage

QARK_REGISTER_TYPE_NAME(DPackage, "DPackage")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DPackage, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DPackage, DObject)
QARK_ACCESS_SERIALIZE(DPackage)

template<class Archive>
inline void Access<Archive, DPackage>::serialize(Archive &archive, DPackage &package)
{
    archive || tag(package)
            || base<DObject>(package)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DPackage)


// DClass

QARK_REGISTER_TYPE_NAME(DClass, "DClass")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DClass, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DClass, DObject)
QARK_ACCESS_SERIALIZE(DClass)

template<class Archive>
inline void Access<Archive, DClass>::serialize(Archive &archive, DClass &klass)
{
    archive || tag(klass)
            || base<DObject>(klass)
            || attr(QStringLiteral("namespace"), klass, &DClass::getNamespace, &DClass::setNamespace)
            || attr(QStringLiteral("template"), klass, &DClass::getTemplateParameters, &DClass::setTemplateParameters)
            || attr(QStringLiteral("template-display"), klass, &DClass::getTemplateDisplay, &DClass::setTemplateDisplay)
            || attr(QStringLiteral("show-all-members"), klass, &DClass::getShowAllMembers, &DClass::setShowAllMembers)
            || attr(QStringLiteral("visible-members"), klass, &DClass::getVisibleMembers, &DClass::setVisibleMembers)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DClass)


// DComponent

QARK_REGISTER_TYPE_NAME(DComponent, "DComponent")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DComponent, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DComponent, DObject)
QARK_ACCESS_SERIALIZE(DComponent)

template<class Archive>
inline void Access<Archive, DComponent>::serialize(Archive &archive, DComponent &component)
{
    archive || tag(component)
            || base<DObject>(component)
            || attr(QStringLiteral("plain-shape"), component, &DComponent::getPlainShape, &DComponent::setPlainShape)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DComponent)


// DDiagram

QARK_REGISTER_TYPE_NAME(DDiagram, "DDiagram")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DDiagram, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DDiagram, DObject)
QARK_ACCESS_SERIALIZE(DDiagram)

template<class Archive>
inline void Access<Archive, DDiagram>::serialize(Archive &archive, DDiagram &diagram)
{
    archive || tag(diagram)
            || base<DObject>(diagram)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DDiagram)


// DItem

QARK_REGISTER_TYPE_NAME(DItem, "DItem")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DItem, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DItem, DObject)
QARK_ACCESS_SERIALIZE(DItem)

template<class Archive>
inline void Access<Archive, DItem>::serialize(Archive &archive, DItem &item)
{
    archive || tag(item)
            || base<DObject>(item)
            || attr(QStringLiteral("variety"), item, &DItem::getVariety, &DItem::setVariety)
            || attr(QStringLiteral("shape-editable"), item, &DItem::isShapeEditable, &DItem::setShapeEditable)
            || attr(QStringLiteral("shape"), item, &DItem::getShape, &DItem::setShape)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DItem)


// DRelation

QARK_REGISTER_TYPE_NAME(DRelation, "DRelation")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DRelation, DElement)
QARK_ACCESS_SERIALIZE(DRelation)

template<class Archive>
inline void Access<Archive, DRelation>::serialize(Archive &archive, DRelation &relation)
{
    archive || tag(relation)
            || base<DElement>(relation)
            || attr(QStringLiteral("object"), relation, &DRelation::getModelUid, &DRelation::setModelUid)
            || attr(QStringLiteral("stereotypes"), relation, &DRelation::getStereotypes, &DRelation::setStereotypes)
            || attr(QStringLiteral("a"), relation, &DRelation::getEndA, &DRelation::setEndA)
            || attr(QStringLiteral("b"), relation, &DRelation::getEndB, &DRelation::setEndB)
            || attr(QStringLiteral("name"), relation, &DRelation::getName, &DRelation::setName)
            || attr(QStringLiteral("points"), relation, &DRelation::getIntermediatePoints, &DRelation::setIntermediatePoints)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DRelation)


// DRelation::IntermediatePoint

QARK_REGISTER_TYPE_NAME(DRelation::IntermediatePoint, "DRelation--IntermediatePoint")
QARK_ACCESS_SERIALIZE(DRelation::IntermediatePoint)

template<class Archive>
inline void Access<Archive, DRelation::IntermediatePoint>::serialize(Archive &archive, DRelation::IntermediatePoint &point)
{
    archive || tag(point)
            || attr(QStringLiteral("pos"), point, &DRelation::IntermediatePoint::getPos, &DRelation::IntermediatePoint::setPos)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DRelation::IntermediatePoint)


// DInheritance

QARK_REGISTER_TYPE_NAME(DInheritance, "DInheritance")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DInheritance, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DInheritance, DRelation)
QARK_ACCESS_SERIALIZE(DInheritance)

template<class Archive>
inline void Access<Archive, DInheritance>::serialize(Archive &archive, DInheritance &inheritance)
{
    archive || tag(inheritance)
            || base<DRelation>(inheritance)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DInheritance)


// DDependency

QARK_REGISTER_TYPE_NAME(DDependency, "DDependency")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DDependency, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DDependency, DRelation)
QARK_ACCESS_SERIALIZE(DDependency)

template<class Archive>
inline void Access<Archive, DDependency>::serialize(Archive &archive, DDependency &dependency)
{
    archive || tag(dependency)
            || base<DRelation>(dependency)
            || attr(QStringLiteral("direction"), dependency, &DDependency::getDirection, &DDependency::setDirection)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DDependency)


// DAssociation

QARK_REGISTER_TYPE_NAME(DAssociationEnd, "DAssociationEnd")
QARK_ACCESS_SERIALIZE(DAssociationEnd)

template<class Archive>
inline void Access<Archive, DAssociationEnd>::serialize(Archive &archive, DAssociationEnd &associationEnd)
{
    archive || tag(associationEnd)
            || attr(QStringLiteral("name"), associationEnd, &DAssociationEnd::getName, &DAssociationEnd::setName)
            || attr(QStringLiteral("cradinality"), associationEnd, &DAssociationEnd::getCardinality, &DAssociationEnd::setCardinatlity)
            || attr(QStringLiteral("navigable"), associationEnd, &DAssociationEnd::isNavigable, &DAssociationEnd::setNavigable)
            || attr(QStringLiteral("kind"), associationEnd, &DAssociationEnd::getKind, &DAssociationEnd::setKind)
            || end;
}

QARK_REGISTER_TYPE_NAME(DAssociation, "DAssociation")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DAssociation, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DAssociation, DRelation)
QARK_ACCESS_SERIALIZE(DAssociation)

template<class Archive>
inline void Access<Archive, DAssociation>::serialize(Archive &archive, DAssociation &association)
{
    archive || tag(association)
            || base<DRelation>(association)
            || attr(QStringLiteral("class"), association, &DAssociation::getAssoicationClassUid, &DAssociation::setAssociationClassUid)
            || attr(QStringLiteral("a"), association, &DAssociation::getA, &DAssociation::setA)
            || attr(QStringLiteral("b"), association, &DAssociation::getB, &DAssociation::setB)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DAssociation)


// DAnnotation

QARK_REGISTER_TYPE_NAME(DAnnotation, "DAnnotation")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DAnnotation, DElement)
QARK_ACCESS_SERIALIZE(DAnnotation)

template<class Archive>
inline void Access<Archive, DAnnotation>::serialize(Archive &archive, DAnnotation &annotation)
{
    archive || tag(annotation)
            || base<DElement>(annotation)
            || attr(QStringLiteral("text"), annotation, &DAnnotation::getText, &DAnnotation::setText)
            || attr(QStringLiteral("pos"), annotation, &DAnnotation::getPos, &DAnnotation::setPos)
            || attr(QStringLiteral("rect"), annotation, &DAnnotation::getRect, &DAnnotation::setRect)
            || attr(QStringLiteral("auto-sized"), annotation, &DAnnotation::hasAutoSize, &DAnnotation::setAutoSize)
            || attr(QStringLiteral("visual-role"), annotation, &DAnnotation::getVisualRole, &DAnnotation::setVisualRole)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DAnnotation)


// DBoundary

QARK_REGISTER_TYPE_NAME(DBoundary, "DBoundary")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DBoundary, DElement)
QARK_ACCESS_SERIALIZE(DBoundary)

template<class Archive>
inline void Access<Archive, DBoundary>::serialize(Archive &archive, DBoundary &boundary)
{
    archive || tag(boundary)
            || base<DElement>(boundary)
            || attr(QStringLiteral("text"), boundary, &DBoundary::getText, &DBoundary::setText)
            || attr(QStringLiteral("pos"), boundary, &DBoundary::getPos, &DBoundary::setPos)
            || attr(QStringLiteral("rect"), boundary, &DBoundary::getRect, &DBoundary::setRect)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DBoundary)

}
