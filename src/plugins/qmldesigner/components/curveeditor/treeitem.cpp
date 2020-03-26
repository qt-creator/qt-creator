/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "treeitem.h"

#include <QIcon>
#include <QVariant>

namespace DesignTools {

TreeItem::TreeItem(const QString &name)
    : m_name(name)
    , m_id(0)
    , m_locked(false)
    , m_pinned(false)
    , m_parent(nullptr)
    , m_children()
{}

TreeItem::~TreeItem()
{
    m_parent = nullptr;

    qDeleteAll(m_children);
    m_children.clear();
}

QIcon TreeItem::icon() const
{
    return QIcon();
}

NodeTreeItem *TreeItem::asNodeItem()
{
    return nullptr;
}

PropertyTreeItem *TreeItem::asPropertyItem()
{
    return nullptr;
}

unsigned int TreeItem::id() const
{
    return m_id;
}

QString TreeItem::name() const
{
    return m_name;
}

TreeItem::Path TreeItem::path() const
{
    Path fullName;
    fullName.push_back(name());

    TreeItem *parent = this->parent();
    while (parent) {
        if (parent->name() == "Root")
            break;

        fullName.push_back(parent->name());
        parent = parent->parent();
    }

    std::reverse(fullName.begin(), fullName.end());

    return fullName;
}

bool TreeItem::hasChildren() const
{
    return !m_children.empty();
}

bool TreeItem::locked() const
{
    return m_locked;
}

bool TreeItem::pinned() const
{
    return m_pinned;
}

bool TreeItem::compare(const std::vector<QString> &path) const
{
    auto thisPath = this->path();
    if (thisPath.size() != path.size())
        return false;

    for (size_t i = 0; i < thisPath.size(); ++i) {
        if (thisPath[i] != path[i])
            return false;
    }

    return true;
}

int TreeItem::row() const
{
    if (m_parent) {
        for (int i = 0, total = int(m_parent->m_children.size()); i < total; ++i) {
            if (m_parent->m_children[i] == this)
                return i;
        }
    }

    return 0;
}

int TreeItem::column() const
{
    return 0;
}

int TreeItem::rowCount() const
{
    return int(m_children.size());
}

int TreeItem::columnCount() const
{
    return 3;
}

TreeItem *TreeItem::parent() const
{
    return m_parent;
}

TreeItem *TreeItem::child(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_children.size()))
        return nullptr;

    return m_children.at(row);
}

TreeItem *TreeItem::find(unsigned int id) const
{
    for (auto *child : m_children) {
        if (child->id() == id)
            return child;

        if (auto *childsChild = child->find(id))
            return childsChild;
    }

    return nullptr;
}

QVariant TreeItem::data(int column) const
{
    switch (column) {
    case 0:
        return QVariant(m_name);
    case 1:
        return QVariant(m_locked);
    case 2:
        return QVariant(m_pinned);
    case 3:
        return QVariant(m_id);
    default:
        return QVariant();
    }
}

QVariant TreeItem::headerData(int column) const
{
    switch (column) {
    case 0:
        return QString("Name");
    case 1:
        return QString("L");
    case 2:
        return QString("P");
    case 3:
        return QString("Id");
    default:
        return QVariant();
    }
}

void TreeItem::setId(unsigned int &id)
{
    m_id = id;

    for (auto *child : m_children)
        child->setId(++id);
}

void TreeItem::addChild(TreeItem *child)
{
    child->m_parent = this;
    m_children.push_back(child);
}

void TreeItem::setLocked(bool locked)
{
    m_locked = locked;
}

void TreeItem::setPinned(bool pinned)
{
    m_pinned = pinned;
}

NodeTreeItem::NodeTreeItem(const QString &name, const QIcon &icon)
    : TreeItem(name)
    , m_icon(icon)
{
    Q_UNUSED(icon)
}

NodeTreeItem *NodeTreeItem::asNodeItem()
{
    return this;
}

QIcon NodeTreeItem::icon() const
{
    return m_icon;
}

std::vector<PropertyTreeItem *> NodeTreeItem::properties() const
{
    std::vector<PropertyTreeItem *> out;
    for (auto *child : m_children) {
        if (auto *pti = child->asPropertyItem())
            out.push_back(pti);
    }

    return out;
}

std::string toString(ValueType type)
{
    switch (type) {
    case ValueType::Bool:
        return "Bool";
    case ValueType::Integer:
        return "Integer";
    case ValueType::Double:
        return "Double";
    default:
        return "Undefined";
    }
}

PropertyTreeItem::PropertyTreeItem(const QString &name,
                                   const AnimationCurve &curve,
                                   const ValueType &type)
    : TreeItem(name)
    , m_type(type)
    , m_component(Component::Generic)
    , m_curve(curve)
{}

PropertyTreeItem *PropertyTreeItem::asPropertyItem()
{
    return this;
}

const NodeTreeItem *PropertyTreeItem::parentNodeTreeItem() const
{
    TreeItem *p = parent();
    while (p) {
        if (NodeTreeItem *ni = p->asNodeItem())
            return ni;
        p = p->parent();
    }
    return nullptr;
}

ValueType PropertyTreeItem::valueType() const
{
    return m_type;
}

PropertyTreeItem::Component PropertyTreeItem::component() const
{
    return m_component;
}

AnimationCurve PropertyTreeItem::curve() const
{
    return m_curve;
}

bool PropertyTreeItem::hasUnified() const
{
    return m_curve.hasUnified();
}

QString PropertyTreeItem::unifyString() const
{
    return m_curve.unifyString();
}

void PropertyTreeItem::setCurve(const AnimationCurve &curve)
{
    m_curve = curve;
}

void PropertyTreeItem::setComponent(const Component &comp)
{
    m_component = comp;
}

} // End namespace DesignTools.
