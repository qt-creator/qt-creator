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
    foreach (const Handle<MObject> &handle, children) {
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
    foreach (const Handle<MRelation> &handle, relations) {
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
