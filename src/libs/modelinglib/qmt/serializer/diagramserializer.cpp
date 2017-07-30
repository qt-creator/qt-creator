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
#include "qmt/diagram/dconnection.h"

#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dboundary.h"
#include "qmt/diagram/dswimlane.h"

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
            || attr("uid", element, &DElement::uid, &DElement::setUid)
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
            || attr("object", object, &DObject::modelUid, &DObject::setModelUid)
            || attr("stereotypes", object, &DObject::stereotypes, &DObject::setStereotypes)
            || attr("context", object, &DObject::context, &DObject::setContext)
            || attr("name", object, &DObject::name, &DObject::setName)
            || attr("pos", object, &DObject::pos, &DObject::setPos)
            || attr("rect", object, &DObject::rect, &DObject::setRect)
            || attr("auto-sized", object, &DObject::isAutoSized, &DObject::setAutoSized)
            || attr("visual-role", object, &visualRole, &setVisualRole)
            || attr("visual-role2", object, &DObject::visualSecondaryRole, &DObject::setVisualSecondaryRole)
            || attr("visual-emphasized", object, &DObject::isVisualEmphasized, &DObject::setVisualEmphasized)
            || attr("stereotype-display", object, &DObject::stereotypeDisplay, &DObject::setStereotypeDisplay)
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
            || attr("namespace", klass, &DClass::umlNamespace, &DClass::setUmlNamespace)
            || attr("template", klass, &DClass::templateParameters, &DClass::setTemplateParameters)
            || attr("template-display", klass, &DClass::templateDisplay, &DClass::setTemplateDisplay)
            || attr("show-all-members", klass, &DClass::showAllMembers, &DClass::setShowAllMembers)
            || attr("visible-members", klass, &DClass::visibleMembers, &DClass::setVisibleMembers)
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
            || attr("plain-shape", component, &DComponent::isPlainShape, &DComponent::setPlainShape)
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
            || attr("variety", item, &DItem::variety, &DItem::setVariety)
            || attr("shape-editable", item, &DItem::isShapeEditable, &DItem::setShapeEditable)
            || attr("shape", item, &DItem::shape, &DItem::setShape)
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
            || attr("object", relation, &DRelation::modelUid, &DRelation::setModelUid)
            || attr("stereotypes", relation, &DRelation::stereotypes, &DRelation::setStereotypes)
            || attr("a", relation, &DRelation::endAUid, &DRelation::setEndAUid)
            || attr("b", relation, &DRelation::endBUid, &DRelation::setEndBUid)
            || attr("name", relation, &DRelation::name, &DRelation::setName)
            || attr("points", relation, &DRelation::intermediatePoints, &DRelation::setIntermediatePoints)
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
            || attr("pos", point, &DRelation::IntermediatePoint::pos, &DRelation::IntermediatePoint::setPos)
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
            || attr("direction", dependency, &DDependency::direction, &DDependency::setDirection)
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
            || attr("name", associationEnd, &DAssociationEnd::name, &DAssociationEnd::setName)
            || attr("cradinality", associationEnd, &DAssociationEnd::cardinality, &DAssociationEnd::setCardinatlity)
            || attr("navigable", associationEnd, &DAssociationEnd::isNavigable, &DAssociationEnd::setNavigable)
            || attr("kind", associationEnd, &DAssociationEnd::kind, &DAssociationEnd::setKind)
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
            || attr("class", association, &DAssociation::assoicationClassUid, &DAssociation::setAssociationClassUid)
            || attr("a", association, &DAssociation::endA, &DAssociation::setEndA)
            || attr("b", association, &DAssociation::endB, &DAssociation::setEndB)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DAssociation)

// DConnection

QARK_REGISTER_TYPE_NAME(DConnectionEnd, "DConnectionEnd")
QARK_ACCESS_SERIALIZE(DConnectionEnd)

template<class Archive>
inline void Access<Archive, DConnectionEnd>::serialize(Archive &archive, DConnectionEnd &connectionEnd)
{
    archive || tag(connectionEnd)
            || attr("name", connectionEnd, &DConnectionEnd::name, &DConnectionEnd::setName)
            || attr("cradinality", connectionEnd, &DConnectionEnd::cardinality, &DConnectionEnd::setCardinatlity)
            || attr("navigable", connectionEnd, &DConnectionEnd::isNavigable, &DConnectionEnd::setNavigable)
            || end;
}

QARK_REGISTER_TYPE_NAME(DConnection, "DConnection")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DConnection, DElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DConnection, DRelation)
QARK_ACCESS_SERIALIZE(DConnection)

template<class Archive>
inline void Access<Archive, DConnection>::serialize(Archive &archive, DConnection &connection)
{
    archive || tag(connection)
            || base<DRelation>(connection)
            || attr("custom-relation", connection, &DConnection::customRelationId, &DConnection::setCustomRelationId)
            || attr("a", connection, &DConnection::endA, &DConnection::setEndA)
            || attr("b", connection, &DConnection::endB, &DConnection::setEndB)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DConnection)

// DAnnotation

QARK_REGISTER_TYPE_NAME(DAnnotation, "DAnnotation")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DAnnotation, DElement)
QARK_ACCESS_SERIALIZE(DAnnotation)

template<class Archive>
inline void Access<Archive, DAnnotation>::serialize(Archive &archive, DAnnotation &annotation)
{
    archive || tag(annotation)
            || base<DElement>(annotation)
            || attr("text", annotation, &DAnnotation::text, &DAnnotation::setText)
            || attr("pos", annotation, &DAnnotation::pos, &DAnnotation::setPos)
            || attr("rect", annotation, &DAnnotation::rect, &DAnnotation::setRect)
            || attr("auto-sized", annotation, &DAnnotation::isAutoSized, &DAnnotation::setAutoSized)
            || attr("visual-role", annotation, &DAnnotation::visualRole, &DAnnotation::setVisualRole)
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
            || attr("text", boundary, &DBoundary::text, &DBoundary::setText)
            || attr("pos", boundary, &DBoundary::pos, &DBoundary::setPos)
            || attr("rect", boundary, &DBoundary::rect, &DBoundary::setRect)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DBoundary)

// DSwimlane

QARK_REGISTER_TYPE_NAME(DSwimlane, "DSwimlane")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, DSwimlane, DElement)
QARK_ACCESS_SERIALIZE(DSwimlane)

template<class Archive>
inline void Access<Archive, DSwimlane>::serialize(Archive &archive, DSwimlane &swimlane)
{
    archive || tag(swimlane)
            || base<DElement>(swimlane)
            || attr("text", swimlane, &DSwimlane::text, &DSwimlane::setText)
            || attr("horizontal", swimlane, &DSwimlane::isHorizontal, &DSwimlane::setHorizontal)
            || attr("pos", swimlane, &DSwimlane::pos, &DSwimlane::setPos)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, DSwimlane)

} // namespace qark
