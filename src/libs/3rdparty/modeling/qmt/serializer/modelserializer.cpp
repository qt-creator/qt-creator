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
inline void Access<Archive, MSourceExpansion>::serialize(Archive &archive, MSourceExpansion &source_expansion)
{
    archive || tag(source_expansion)
            || base<MExpansion>(source_expansion)
            || attr(QStringLiteral("source"), source_expansion, &MSourceExpansion::getSourceId, &MSourceExpansion::setSourceId)
            || attr(QStringLiteral("transient"), source_expansion, &MSourceExpansion::isTransient, &MSourceExpansion::setTransient)
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
            || attr(QStringLiteral("uid"), element, &MElement::getUid, &MElement::setUid)
            || attr(QStringLiteral("flags"), element, &MElement::getFlags, &MElement::setFlags)
            || attr(QStringLiteral("expansion"), element, &MElement::getExpansion, &MElement::setExpansion)
            || attr(QStringLiteral("stereotypes"), element, &MElement::getStereotypes, &MElement::setStereotypes)
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
            || attr(QStringLiteral("name"), object, &MObject::getName, &MObject::setName)
            || attr(QStringLiteral("children"), object, &MObject::getChildren, &MObject::setChildren)
            || attr(QStringLiteral("relations"), object, &MObject::getRelations, &MObject::setRelations)
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
            || attr(QStringLiteral("namespace"), klass, &MClass::getNamespace, &MClass::setNamespace)
            || attr(QStringLiteral("template"), klass, &MClass::getTemplateParameters, &MClass::setTemplateParameters)
            || attr(QStringLiteral("members"), klass, &MClass::getMembers, &MClass::setMembers)
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
            || attr(QStringLiteral("uid"), member, &MClassMember::getUid, &MClassMember::setUid)
            || attr(QStringLiteral("stereotypes"), member, &MClassMember::getStereotypes, &MClassMember::setStereotypes)
            || attr(QStringLiteral("group"), member, &MClassMember::getGroup, &MClassMember::setGroup)
            || attr(QStringLiteral("visibility"), member, &MClassMember::getVisibility, &MClassMember::setVisibility)
            || attr(QStringLiteral("type"), member, &MClassMember::getMemberType, &MClassMember::setMemberType)
            || attr(QStringLiteral("properties"), member, &MClassMember::getProperties, &MClassMember::setProperties)
            || attr(QStringLiteral("declaration"), member, &MClassMember::getDeclaration, &MClassMember::setDeclaration)
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
            || attr(QStringLiteral("elements"), diagram, &MDiagram::getDiagramElements, &MDiagram::setDiagramElements)
            || attr(QStringLiteral("last-modified"), diagram, &MDiagram::getLastModified, &MDiagram::setLastModified)
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
            || attr(QStringLiteral("variety-editable"), item, &MItem::isVarietyEditable, &MItem::setVarietyEditable)
            || attr(QStringLiteral("variety"), item, &MItem::getVariety, &MItem::setVariety)
            || attr(QStringLiteral("shape-editable"), item, &MItem::isShapeEditable, &MItem::setShapeEditable)
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
            || attr(QStringLiteral("name"), relation, &MRelation::getName, &MRelation::setName)
            || attr(QStringLiteral("a"), relation, &MRelation::getEndA, &MRelation::setEndA)
            || attr(QStringLiteral("b"), relation, &MRelation::getEndB, &MRelation::setEndB)
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
            || attr(QStringLiteral("direction"), dependency, &MDependency::getDirection, &MDependency::setDirection)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MDependency)


// MAssociation

QARK_REGISTER_TYPE_NAME(MAssociationEnd, "MAssociationEnd")
QARK_ACCESS_SERIALIZE(MAssociationEnd)

template<class Archive>
inline void Access<Archive, MAssociationEnd>::serialize(Archive &archive, MAssociationEnd &association_end)
{
    archive || tag(association_end)
            || attr(QStringLiteral("name"), association_end, &MAssociationEnd::getName, &MAssociationEnd::setName)
            || attr(QStringLiteral("cardinality"), association_end, &MAssociationEnd::getCardinality, &MAssociationEnd::setCardinality)
            || attr(QStringLiteral("navigable"), association_end, &MAssociationEnd::isNavigable, &MAssociationEnd::setNavigable)
            || attr(QStringLiteral("kind"), association_end, &MAssociationEnd::getKind, &MAssociationEnd::setKind)
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
            || attr(QStringLiteral("class"), association, &MAssociation::getAssoicationClassUid, &MAssociation::setAssociationClassUid)
            || attr(QStringLiteral("a"), association, &MAssociation::getA, &MAssociation::setA)
            || attr(QStringLiteral("b"), association, &MAssociation::getB, &MAssociation::setB)
            || end;
}

QARK_ACCESS_SPECIALIZE(QXmlInArchive, QXmlOutArchive, MAssociation)

}
