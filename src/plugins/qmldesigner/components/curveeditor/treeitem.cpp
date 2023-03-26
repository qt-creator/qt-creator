// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "treeitem.h"

#include <QIcon>
#include <QVariant>

namespace QmlDesigner {

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
        for (size_t i = 0, total = m_parent->m_children.size(); i < total; ++i) {
            if (m_parent->m_children[i] == this)
                return static_cast<int>(i);
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

TreeItem *TreeItem::root() const
{
    TreeItem *p = parent();
    while (p) {
        if (!p->parent())
            return p;
        p = p->parent();
    }
    return p;
}

TreeItem *TreeItem::parent() const
{
    return m_parent;
}

TreeItem *TreeItem::child(int row) const
{
    if (row < 0 || row >= rowCount())
        return nullptr;

    return m_children.at(static_cast<size_t>(row));
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

TreeItem *TreeItem::find(const QString &id) const
{
    for (auto *child : m_children) {
        if (child->name() == id)
            return child;

        if (auto *childsChild = child->find(id))
            return childsChild;
    }

    return nullptr;
}

std::vector<TreeItem *> TreeItem::children() const
{
    return m_children;
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

bool TreeItem::operator==(unsigned int id) const
{
    return m_id == id;
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

NodeTreeItem::NodeTreeItem(const QString &name,
                           [[maybe_unused]] const QIcon &icon,
                           const std::vector<QString> &parentIds)
    : TreeItem(name)
    , m_icon(icon)
    , m_parentIds(parentIds)
{
}

NodeTreeItem *NodeTreeItem::asNodeItem()
{
    return this;
}

bool NodeTreeItem::implicitlyLocked() const
{
    TreeItem *r = root();
    if (!r)
        return false;

    for (auto &&id : m_parentIds) {
        if (TreeItem *item = r->find(id))
            if (item->locked())
                return true;
    }

    return false;
}

bool NodeTreeItem::implicitlyPinned() const
{
    TreeItem *r = root();
    if (!r)
        return false;

    for (auto &&id : m_parentIds) {
        if (TreeItem *item = r->find(id))
            if (item->pinned())
                return true;
    }
    return false;
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

PropertyTreeItem::PropertyTreeItem(const QString &name, const AnimationCurve &curve)
    : TreeItem(name)
    , m_type(curve.valueType())
    , m_component(Component::Generic)
    , m_curve(curve)
{}

bool PropertyTreeItem::implicitlyLocked() const
{
    if (auto *parentNode = parentNodeTreeItem())
        return parentNode->locked() || parentNode->implicitlyLocked();

    return false;
}

bool PropertyTreeItem::implicitlyPinned() const
{
    if (auto *parentNode = parentNodeTreeItem())
        return parentNode->pinned() || parentNode->implicitlyPinned();

    return false;
}

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

PropertyTreeItem::ValueType PropertyTreeItem::valueType() const
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

std::string toString(PropertyTreeItem::ValueType type)
{
    switch (type) {
    case PropertyTreeItem::ValueType::Bool:
        return "Bool";
    case PropertyTreeItem::ValueType::Integer:
        return "Integer";
    case PropertyTreeItem::ValueType::Double:
        return "Double";
    default:
        return "Undefined";
    }
}

} // End namespace QmlDesigner.
