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
            || attr(QStringLiteral("uid"), element, &DElement::uid, &DElement::setUid)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DElement)

// DObject

// functions for backward compatibility to old visual role
static DObject::VisualPrimaryRole visualRole(const DObject &object)
{
    DObject::VisualPrimaryRole visualRole = object.visualPrimaryRole();
    if (visualRole == DObject::DeprecatedPrimaryRoleDarker
            || visualRole == DObject::DeprecatedPrimaryRoleLighter
            || visualRole == DObject::DeprecatedPrimaryRoleOutline
            || visualRole == DObject::DeprecatedPrimaryRoleSoften) {
        QMT_CHECK(false);
        visualRole = DObject::PrimaryRoleNormal;
    }
    return visualRole;
}

static void setVisualRole(DObject &object, DObject::VisualPrimaryRole visualRole)
{
    if (visualRole == DObject::DeprecatedPrimaryRoleDarker) {
        object.setVisualPrimaryRole(DObject::PrimaryRoleNormal);
        object.setVisualSecondaryRole(DObject::SecondaryRoleDarker);
    } else if (visualRole == DObject::DeprecatedPrimaryRoleLighter) {
        object.setVisualPrimaryRole(DObject::PrimaryRoleNormal);
        object.setVisualSecondaryRole(DObject::SecondaryRoleLighter);
    } else if (visualRole == DObject::DeprecatedPrimaryRoleOutline) {
        object.setVisualPrimaryRole(DObject::PrimaryRoleNormal);
        object.setVisualSecondaryRole(DObject::SecondaryRoleOutline);
    } else if (visualRole == DObject::DeprecatedPrimaryRoleSoften) {
        object.setVisualPrimaryRole(DObject::PrimaryRoleNormal);
        object.setVisualSecondaryRole(DObject::SecondaryRoleSoften);
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
            || attr(QStringLiteral("object"), object, &DObject::modelUid, &DObject::setModelUid)
            || attr(QStringLiteral("stereotypes"), object, &DObject::stereotypes, &DObject::setStereotypes)
            || attr(QStringLiteral("context"), object, &DObject::context, &DObject::setContext)
            || attr(QStringLiteral("name"), object, &DObject::name, &DObject::setName)
            || attr(QStringLiteral("pos"), object, &DObject::pos, &DObject::setPos)
            || attr(QStringLiteral("rect"), object, &DObject::rect, &DObject::setRect)
            || attr(QStringLiteral("auto-sized"), object, &DObject::isAutoSized, &DObject::setAutoSized)
            || attr(QStringLiteral("visual-role"), object, &visualRole, &setVisualRole)
            || attr(QStringLiteral("visual-role2"), object, &DObject::visualSecondaryRole, &DObject::setVisualSecondaryRole)
            || attr(QStringLiteral("visual-emphasized"), object, &DObject::isVisualEmphasized, &DObject::setVisualEmphasized)
            || attr(QStringLiteral("stereotype-display"), object, &DObject::stereotypeDisplay, &DObject::setStereotypeDisplay)
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
            || attr(QStringLiteral("namespace"), klass, &DClass::umlNamespace, &DClass::setUmlNamespace)
            || attr(QStringLiteral("template"), klass, &DClass::templateParameters, &DClass::setTemplateParameters)
            || attr(QStringLiteral("template-display"), klass, &DClass::templateDisplay, &DClass::setTemplateDisplay)
            || attr(QStringLiteral("show-all-members"), klass, &DClass::showAllMembers, &DClass::setShowAllMembers)
            || attr(QStringLiteral("visible-members"), klass, &DClass::visibleMembers, &DClass::setVisibleMembers)
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
            || attr(QStringLiteral("plain-shape"), component, &DComponent::isPlainShape, &DComponent::setPlainShape)
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
            || attr(QStringLiteral("variety"), item, &DItem::variety, &DItem::setVariety)
            || attr(QStringLiteral("shape-editable"), item, &DItem::isShapeEditable, &DItem::setShapeEditable)
            || attr(QStringLiteral("shape"), item, &DItem::shape, &DItem::setShape)
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
            || attr(QStringLiteral("object"), relation, &DRelation::modelUid, &DRelation::setModelUid)
            || attr(QStringLiteral("stereotypes"), relation, &DRelation::stereotypes, &DRelation::setStereotypes)
            || attr(QStringLiteral("a"), relation, &DRelation::endAUid, &DRelation::setEndAUid)
            || attr(QStringLiteral("b"), relation, &DRelation::endBUid, &DRelation::setEndBUid)
            || attr(QStringLiteral("name"), relation, &DRelation::name, &DRelation::setName)
            || attr(QStringLiteral("points"), relation, &DRelation::intermediatePoints, &DRelation::setIntermediatePoints)
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
            || attr(QStringLiteral("pos"), point, &DRelation::IntermediatePoint::pos, &DRelation::IntermediatePoint::setPos)
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
            || attr(QStringLiteral("direction"), dependency, &DDependency::direction, &DDependency::setDirection)
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
            || attr(QStringLiteral("name"), associationEnd, &DAssociationEnd::name, &DAssociationEnd::setName)
            || attr(QStringLiteral("cradinality"), associationEnd, &DAssociationEnd::cardinality, &DAssociationEnd::setCardinatlity)
            || attr(QStringLiteral("navigable"), associationEnd, &DAssociationEnd::isNavigable, &DAssociationEnd::setNavigable)
            || attr(QStringLiteral("kind"), associationEnd, &DAssociationEnd::kind, &DAssociationEnd::setKind)
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
            || attr(QStringLiteral("class"), association, &DAssociation::assoicationClassUid, &DAssociation::setAssociationClassUid)
            || attr(QStringLiteral("a"), association, &DAssociation::endA, &DAssociation::setEndA)
            || attr(QStringLiteral("b"), association, &DAssociation::endB, &DAssociation::setEndB)
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
            || attr(QStringLiteral("text"), annotation, &DAnnotation::text, &DAnnotation::setText)
            || attr(QStringLiteral("pos"), annotation, &DAnnotation::pos, &DAnnotation::setPos)
            || attr(QStringLiteral("rect"), annotation, &DAnnotation::rect, &DAnnotation::setRect)
            || attr(QStringLiteral("auto-sized"), annotation, &DAnnotation::isAutoSized, &DAnnotation::setAutoSized)
            || attr(QStringLiteral("visual-role"), annotation, &DAnnotation::visualRole, &DAnnotation::setVisualRole)
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
            || attr(QStringLiteral("text"), boundary, &DBoundary::text, &DBoundary::setText)
            || attr(QStringLiteral("pos"), boundary, &DBoundary::pos, &DBoundary::setPos)
            || attr(QStringLiteral("rect"), boundary, &DBoundary::rect, &DBoundary::setRect)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DBoundary)

} // namespace qark
