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

#include "modelserializer.h"

#include "infrastructureserializer.h"

#include "qmt/model/melement.h"
#include "qmt/model/msourceexpansion.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mitem.h"
#include "qmt/model/mrelation.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/massociation.h"
#include "qmt/model/minheritance.h"
#include "qmt/model/mconnection.h"

#include "qmt/diagram/delement.h"

#include "qark/qxmloutarchive.h"
#include "qark/qxmlinarchive.h"
#include "qark/serialize.h"
#include "qark/access.h"

// this using directive avoids some writing effort and especially avoids
// namespace qualifiers in typenames within xml file (which breaks xml syntax)
using namespace qmt;

namespace qark {

// MExpansion
QARK_REGISTER_TYPE_NAME(MExpansion, "MExpansion")
QARK_ACCESS_SERIALIZE(MExpansion)

template<class Archive>
inline void Access<Archive, MExpansion>::serialize(Archive &archive, MExpansion &expansion)
{
    archive || tag(expansion)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MExpansion)

// MSourceExpansion

QARK_REGISTER_TYPE_NAME(MSourceExpansion, "MSourceExpansion")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MSourceExpansion, MExpansion)
QARK_ACCESS_SERIALIZE(MSourceExpansion)

template<class Archive>
inline void Access<Archive, MSourceExpansion>::serialize(Archive &archive, MSourceExpansion &sourceExpansion)
{
    archive || tag(sourceExpansion)
            || base<MExpansion>(sourceExpansion)
            || attr("source", sourceExpansion, &MSourceExpansion::sourceId, &MSourceExpansion::setSourceId)
            || attr("transient", sourceExpansion, &MSourceExpansion::isTransient, &MSourceExpansion::setTransient)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MSourceExpansion)

// MElement

QARK_REGISTER_TYPE_NAME(MElement, "MElement")
QARK_ACCESS_SERIALIZE(MElement)

template<class Archive>
inline void Access<Archive, MElement>::serialize(Archive &archive, MElement &element)
{
    archive || tag(element)
            || attr("uid", element, &MElement::uid, &MElement::setUid)
            || attr("flags", element, &MElement::flags, &MElement::setFlags)
            || attr("expansion", element, &MElement::expansion, &MElement::setExpansion)
            || attr("stereotypes", element, &MElement::stereotypes, &MElement::setStereotypes)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MElement)

// MObject

QARK_REGISTER_TYPE_NAME(MObject, "MObject")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MObject, MElement)
QARK_ACCESS_SERIALIZE(MObject)

template<class Archive>
inline void Access<Archive, MObject>::serialize(Archive &archive, MObject &object)
{
    archive || tag(object)
            || base<MElement>(object)
            || attr("name", object, &MObject::name, &MObject::setName)
            || attr("children", object, &MObject::children, &MObject::setChildren)
            || attr("relations", object, &MObject::relations, &MObject::setRelations)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MObject)

// MPackage

QARK_REGISTER_TYPE_NAME(MPackage, "MPackage")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MPackage, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MPackage, MObject)
QARK_ACCESS_SERIALIZE(MPackage)

template<class Archive>
inline void Access<Archive, MPackage>::serialize(Archive &archive, MPackage &package)
{
    archive || tag(package)
            || base<MObject>(package)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MPackage)

// MClass

QARK_REGISTER_TYPE_NAME(MClass, "MClass")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MClass, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MClass, MObject)
QARK_ACCESS_SERIALIZE(MClass)

template<class Archive>
inline void Access<Archive, MClass>::serialize(Archive &archive, MClass &klass)
{
    archive || tag(klass)
            || base<MObject>(klass)
            || attr("namespace", klass, &MClass::umlNamespace, &MClass::setUmlNamespace)
            || attr("template", klass, &MClass::templateParameters, &MClass::setTemplateParameters)
            || attr("members", klass, &MClass::members, &MClass::setMembers)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MClass)

// MClassMember

QARK_REGISTER_TYPE_NAME(MClassMember, "MClassMember")
QARK_ACCESS_SERIALIZE(MClassMember)

template<class Archive>
inline void Access<Archive, MClassMember>::serialize(Archive &archive, MClassMember &member)
{
    archive || tag(member)
            || attr("uid", member, &MClassMember::uid, &MClassMember::setUid)
            || attr("stereotypes", member, &MClassMember::stereotypes, &MClassMember::setStereotypes)
            || attr("group", member, &MClassMember::group, &MClassMember::setGroup)
            || attr("visibility", member, &MClassMember::visibility, &MClassMember::setVisibility)
            || attr("type", member, &MClassMember::memberType, &MClassMember::setMemberType)
            || attr("properties", member, &MClassMember::properties, &MClassMember::setProperties)
            || attr("declaration", member, &MClassMember::declaration, &MClassMember::setDeclaration)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MClassMember)

// MComponent

QARK_REGISTER_TYPE_NAME(MComponent, "MComponent")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MComponent, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MComponent, MObject)
QARK_ACCESS_SERIALIZE(MComponent)

template<class Archive>
inline void Access<Archive, MComponent>::serialize(Archive &archive, MComponent &component)
{
    archive || tag(component)
            || base<MObject>(component)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MComponent)

// MDiagram

QARK_REGISTER_TYPE_NAME(MDiagram, "MDiagram")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MDiagram, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MDiagram, MObject)
QARK_ACCESS_SERIALIZE(MDiagram)

template<class Archive>
inline void Access<Archive, MDiagram>::serialize(Archive &archive, MDiagram &diagram)
{
    archive || tag(diagram)
            || base<MObject>(diagram)
            || attr("elements", diagram, &MDiagram::diagramElements, &MDiagram::setDiagramElements)
            || attr("last-modified", diagram, &MDiagram::lastModified, &MDiagram::setLastModified)
            || attr("toolbarid", diagram, &MDiagram::toolbarId, &MDiagram::setToolbarId)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MDiagram)

// MCanvasDiagram

QARK_REGISTER_TYPE_NAME(MCanvasDiagram, "MCanvasDiagram")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MCanvasDiagram, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MCanvasDiagram, MObject)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MCanvasDiagram, MDiagram)
QARK_ACCESS_SERIALIZE(MCanvasDiagram)

template<class Archive>
inline void Access<Archive, MCanvasDiagram>::serialize(Archive &archive, MCanvasDiagram &diagram)
{
    archive || tag(diagram)
            || base<MDiagram>(diagram)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MCanvasDiagram)

// MItem

QARK_REGISTER_TYPE_NAME(MItem, "MItem")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MItem, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MItem, MObject)
QARK_ACCESS_SERIALIZE(MItem)

template<class Archive>
inline void Access<Archive, MItem>::serialize(Archive &archive, MItem &item)
{
    archive || tag(item)
            || base<MObject>(item)
            || attr("variety-editable", item, &MItem::isVarietyEditable, &MItem::setVarietyEditable)
            || attr("variety", item, &MItem::variety, &MItem::setVariety)
            || attr("shape-editable", item, &MItem::isShapeEditable, &MItem::setShapeEditable)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MItem)

// MRelation

QARK_REGISTER_TYPE_NAME(MRelation, "MRelation")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MRelation, MElement)
QARK_ACCESS_SERIALIZE(MRelation)

template<class Archive>
inline void Access<Archive, MRelation>::serialize(Archive &archive, MRelation &relation)
{
    archive || tag(relation)
            || base<MElement>(relation)
            || attr("name", relation, &MRelation::name, &MRelation::setName)
            || attr("a", relation, &MRelation::endAUid, &MRelation::setEndAUid)
            || attr("b", relation, &MRelation::endBUid, &MRelation::setEndBUid)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MRelation)

// MInheritance

QARK_REGISTER_TYPE_NAME(MInheritance, "MInheritance")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MInheritance, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MInheritance, MRelation)
QARK_ACCESS_SERIALIZE(MInheritance)

template<class Archive>
inline void Access<Archive, MInheritance>::serialize(Archive &archive, MInheritance &inheritance)
{
    archive || tag(inheritance)
            || base<MRelation>(inheritance)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MInheritance)

// MDependency

QARK_REGISTER_TYPE_NAME(MDependency, "MDependency")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MDependency, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MDependency, MRelation)
QARK_ACCESS_SERIALIZE(MDependency)

template<class Archive>
inline void Access<Archive, MDependency>::serialize(Archive &archive, MDependency &dependency)
{
    archive || tag(dependency)
            || base<MRelation>(dependency)
            || attr("direction", dependency, &MDependency::direction, &MDependency::setDirection)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MDependency)

// MAssociation

QARK_REGISTER_TYPE_NAME(MAssociationEnd, "MAssociationEnd")
QARK_ACCESS_SERIALIZE(MAssociationEnd)

template<class Archive>
inline void Access<Archive, MAssociationEnd>::serialize(Archive &archive, MAssociationEnd &associationEnd)
{
    archive || tag(associationEnd)
            || attr("name", associationEnd, &MAssociationEnd::name, &MAssociationEnd::setName)
            || attr("cardinality", associationEnd, &MAssociationEnd::cardinality, &MAssociationEnd::setCardinality)
            || attr("navigable", associationEnd, &MAssociationEnd::isNavigable, &MAssociationEnd::setNavigable)
            || attr("kind", associationEnd, &MAssociationEnd::kind, &MAssociationEnd::setKind)
            || end;
}

QARK_REGISTER_TYPE_NAME(MAssociation, "MAssociation")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MAssociation, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MAssociation, MRelation)
QARK_ACCESS_SERIALIZE(MAssociation)

template<class Archive>
inline void Access<Archive, MAssociation>::serialize(Archive &archive, MAssociation &association)
{
    archive || tag(association)
            || base<MRelation>(association)
            || attr("class", association, &MAssociation::assoicationClassUid, &MAssociation::setAssociationClassUid)
            || attr("a", association, &MAssociation::endA, &MAssociation::setEndA)
            || attr("b", association, &MAssociation::endB, &MAssociation::setEndB)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MAssociation)

// MConnection

QARK_REGISTER_TYPE_NAME(MConnectionEnd, "MConnectionEnd")
QARK_ACCESS_SERIALIZE(MConnectionEnd)

template<class Archive>
inline void Access<Archive, MConnectionEnd>::serialize(Archive &archive, MConnectionEnd &connectionEnd)
{
    archive || tag(connectionEnd)
            || attr("name", connectionEnd, &MConnectionEnd::name, &MConnectionEnd::setName)
            || attr("cardinality", connectionEnd, &MConnectionEnd::cardinality, &MConnectionEnd::setCardinality)
            || attr("navigable", connectionEnd, &MConnectionEnd::isNavigable, &MConnectionEnd::setNavigable)
            || end;
}

QARK_REGISTER_TYPE_NAME(MConnection, "MConnection")
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MConnection, MElement)
QARK_REGISTER_DERIVED_CLASS(QXmlInArchive, QXmlOutArchive, MConnection, MRelation)
QARK_ACCESS_SERIALIZE(MConnection)

template<class Archive>
inline void Access<Archive, MConnection>::serialize(Archive &archive, MConnection &connection)
{
    archive || tag(connection)
            || base<MRelation>(connection)
            || attr("custom-relation", connection, &MConnection::customRelationId, &MConnection::setCustomRelationId)
            || attr("a", connection, &MConnection::endA, &MConnection::setEndA)
            || attr("b", connection, &MConnection::endB, &MConnection::setEndB)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MConnection)

} // namespace qark
