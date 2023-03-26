// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mobject.h"

#include "mrelation.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MObject::MObject()
    : MElement(),
      m_children(true),
      m_relations(true)
{
}

MObject::MObject(const MObject &rhs)
    : MElement(rhs),
      m_name(rhs.m_name),
      m_children(true),
      m_relations(true)
{
}

MObject::~MObject()
{
}

MObject &MObject::operator =(const MObject &rhs)
{
    if (this != &rhs) {
        MElement::operator=(rhs);
        m_name = rhs.m_name;
        // no deep copy; list of children remains unchanged
    }
    return *this;
}

void MObject::setName(const QString &name)
{
    m_name = name;
}

void MObject::setChildren(const Handles<MObject> &children)
{
    m_children = children;
    for (const Handle<MObject> &handle : children) {
        if (handle.hasTarget())
            handle.target()->setOwner(this);
    }
}

void MObject::addChild(const Uid &uid)
{
    m_children.add(uid);
}

void MObject::addChild(MObject *child)
{
    QMT_ASSERT(child, return);
    QMT_ASSERT(!child->owner(), return);
    m_children.add(child);
    child->setOwner(this);
}

void MObject::insertChild(int beforeIndex, const Uid &uid)
{
    m_children.insert(beforeIndex, uid);
}

void MObject::insertChild(int beforeIndex, MObject *child)
{
    QMT_ASSERT(child, return);
    QMT_ASSERT(!child->owner(), return);
    m_children.insert(beforeIndex, child);
    child->setOwner(this);
}

void MObject::removeChild(const Uid &uid)
{
    QMT_ASSERT(m_children.contains(uid), return);
    MObject *child = m_children.find(uid);
    if (child)
        child->setOwner(nullptr);
    m_children.remove(uid);
}

void MObject::removeChild(MObject *child)
{
    QMT_ASSERT(child, return);
    QMT_ASSERT(m_children.contains(child), return);
    child->setOwner(nullptr);
    m_children.remove(child);
}

void MObject::decontrolChild(const Uid &uid)
{
    QMT_ASSERT(m_children.contains(uid), return);
    MObject *child = m_children.find(uid);
    if (child)
        child->setOwner(nullptr);
    m_children.take(uid);
}

void MObject::decontrolChild(MObject *child)
{
    QMT_ASSERT(child, return);
    QMT_ASSERT(m_children.contains(child), return);
    child->setOwner(nullptr);
    m_children.take(child);
}

void MObject::setRelations(const Handles<MRelation> &relations)
{
    m_relations = relations;
    for (const Handle<MRelation> &handle : relations) {
        if (handle.hasTarget())
            handle.target()->setOwner(this);
    }
}

void MObject::addRelation(const Uid &uid)
{
    m_relations.add(uid);
}

void MObject::addRelation(MRelation *relation)
{
    QMT_ASSERT(relation, return);
    QMT_ASSERT(!relation->owner(), return);
    relation->setOwner(this);
    m_relations.add(relation);
}

void MObject::insertRelation(int beforeIndex, MRelation *relation)
{
    QMT_ASSERT(relation, return);
    QMT_ASSERT(!relation->owner(), return);
    relation->setOwner(this);
    m_relations.insert(beforeIndex, relation);
}

void MObject::removeRelation(MRelation *relation)
{
    QMT_ASSERT(relation, return);
    relation->setOwner(nullptr);
    m_relations.remove(relation);
}

void MObject::decontrolRelation(MRelation *relation)
{
    QMT_ASSERT(relation, return);
    relation->setOwner(nullptr);
    m_relations.take(relation);
}

void MObject::accept(MVisitor *visitor)
{
    visitor->visitMObject(this);
}

void MObject::accept(MConstVisitor *visitor) const
{
    visitor->visitMObject(this);
}

} // namespace qmt
