// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectsmaptreeitem.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>

using namespace Utils;

namespace Squish {
namespace Internal {

const QChar ObjectsMapTreeItem::COLON = ':';

/************************* ObjectsMapTreeItem ***********************************************/

ObjectsMapTreeItem::ObjectsMapTreeItem(const QString &name, Qt::ItemFlags flags)
    : m_propertiesModel(new PropertiesModel(this))
    , m_name(name)
    , m_flags(flags)
{}

ObjectsMapTreeItem::~ObjectsMapTreeItem()
{
    delete m_propertiesModel;
}

// these connections cannot be setup from inside the ctor as we have the need to be already
// child of a model, so this must be setup after the item has been added to a model
// (or as soon the model is available)
void ObjectsMapTreeItem::initPropertyModelConnections(ObjectsMapModel *objMapModel)
{
    QObject::connect(m_propertiesModel,
                     &PropertiesModel::propertyChanged,
                     objMapModel,
                     &ObjectsMapModel::propertyChanged);
    QObject::connect(m_propertiesModel,
                     &PropertiesModel::propertyRemoved,
                     objMapModel,
                     &ObjectsMapModel::propertyRemoved);
    QObject::connect(m_propertiesModel,
                     &PropertiesModel::propertyAdded,
                     objMapModel,
                     &ObjectsMapModel::propertyAdded);
}

QVariant ObjectsMapTreeItem::data(int column, int role) const
{
    if (column == 0 && role == Qt::DisplayRole)
        return m_name;

    return TreeItem::data(column, role);
}

bool ObjectsMapTreeItem::setData(int column, const QVariant &data, int role)
{
    if (column != 0 || role != Qt::EditRole)
        return false;

    m_name = data.toString();
    return true;
}

Qt::ItemFlags ObjectsMapTreeItem::flags(int /*column*/) const
{
    return m_flags;
}

void ObjectsMapTreeItem::setPropertiesContent(const QByteArray &content)
{
    if (parseProperties(content)) {
        m_propertiesContent.clear();
        return;
    }

    m_propertiesContent = content;
}

QByteArray ObjectsMapTreeItem::propertiesToByteArray() const
{
    if (!isValid())
        return propertiesContent();

    QByteArray result;
    const PropertyList properties = sorted(this->properties(),
         [](const Property &lhs, const Property &rhs) { return lhs.m_name < rhs.m_name; });

    result.append('{');
    for (const Property &property : properties)
        result.append(property.toString().toUtf8()).append(' ');
    if (result.at(result.size() - 1) == ' ')
        result.chop(1);
    result.append('}');
    return result;
}

QString ObjectsMapTreeItem::parentName() const
{
    QString result;
    if (auto propertyItem = m_propertiesModel->findItemAtLevel<1>([](TreeItem *item) {
            return static_cast<PropertyTreeItem *>(item)->property().isContainer();
        })) {
        result = propertyItem->data(2, Qt::DisplayRole).toString();
    }
    return result;
}

PropertyList ObjectsMapTreeItem::properties() const
{
    PropertyList result;
    m_propertiesModel->forItemsAtLevel<1>([&result](Utils::TreeItem *item) {
        result.append(static_cast<PropertyTreeItem *>(item)->property());
    });
    return result;
}

bool ObjectsMapTreeItem::parseProperties(const QByteArray &properties)
{
    enum ParseState { None, Name, Operator, Value };

    TreeItem *propertyRoot = m_propertiesModel->rootItem();
    QTC_ASSERT(propertyRoot, return false);
    // if we perform a re-parse, we might have already children
    propertyRoot->removeChildren();

    if (properties.isEmpty() || properties.at(0) != '{')
        return false;

    QString p = QString::fromUtf8(properties);
    ParseState state = None;
    QString name;
    QString value;
    QString oper;
    bool masquerading = false;
    for (QChar c : p) {
        if (masquerading) {
            value.append('\\').append(c);
            masquerading = false;
            continue;
        }

        switch (c.unicode()) {
        case '=':
            if (state == Value) {
                value.append(c);
            } else if ((state == Name) || (state == Operator && oper.size() < 2)) {
                state = Operator;
                oper.append(c);
            } else {
                propertyRoot->removeChildren();
                return false;
            }
            break;
        case '?':
        case '~':
            if (state == Name || (state == Operator && oper.isEmpty())) {
                state = Operator;
                oper.append(c);
            } else if (state == Value) {
                value.append(c);
            } else {
                propertyRoot->removeChildren();
                return false;
            }
            break;
        case '\'':
            if (state == Operator) {
                state = Value;
            } else if (state == Value) {
                state = None;
                Property prop;
                if (!prop.set(name, oper, value)) {
                    propertyRoot->removeChildren();
                    return false;
                }
                m_propertiesModel->addNewProperty(new PropertyTreeItem(prop));
                name.clear();
                oper.clear();
                value.clear();
            } else {
                propertyRoot->removeChildren();
                return false;
            }
            break;
        case '{':
            if (state == None) {
                state = Name;
            } else if (state == Value) {
                value.append(c);
            } else {
                propertyRoot->removeChildren();
                return false;
            }
            break;
        case '}':
            if (state == Value) {
                value.append(c);
            } else if (state == None) {
                return true;
            } else {
                propertyRoot->removeChildren();
                return false;
            }
            break;
        case '\\':
            if (state == Value) {
                masquerading = true;
            } else {
                propertyRoot->removeChildren();
                return false;
            }
            break;
        default:
            if (c.isSpace()) {
                if (state == Value) {
                    value.append(c);
                } else if (state == Name) {
                    state = Operator;
                } else if (state == Operator) {
                    if (!oper.endsWith('=')) {
                        propertyRoot->removeChildren();
                        return false;
                    }
                }
            } else {
                if (state == None) {
                    state = Name;
                    name.append(c);
                } else if (state == Name) {
                    name.append(c);
                } else if (state == Value) {
                    value.append(c);
                } else {
                    propertyRoot->removeChildren();
                    return false;
                }
            }
        }
    }
    if (masquerading || state != None)
        propertyRoot->removeChildren();
    return state == None;
}

/******************************* ObjectsMapModel ********************************************/

ObjectsMapModel::ObjectsMapModel(QObject *parent)
    : TreeModel<ObjectsMapTreeItem>(new ObjectsMapTreeItem(""), parent)
{
    connect(this, &ObjectsMapModel::propertyChanged, this, &ObjectsMapModel::onPropertyChanged);
    connect(this, &ObjectsMapModel::propertyRemoved, this, &ObjectsMapModel::onPropertyRemoved);
    connect(this, &ObjectsMapModel::nameChanged, this, &ObjectsMapModel::onNameChanged);
    connect(this, &ObjectsMapModel::propertyAdded, this, &ObjectsMapModel::modelChanged);
}

bool ObjectsMapModel::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    // only allow editing here
    if (role != Qt::EditRole)
        return false;

    const QString old = idx.data().toString();
    QString modified = data.toString();
    if (modified.isEmpty())
        return false;

    if (modified.at(0) != ObjectsMapTreeItem::COLON)
        modified.prepend(ObjectsMapTreeItem::COLON);

    bool result = TreeModel::setData(idx, modified, role);
    if (result) {
        emit nameChanged(old, modified);
        emit requestSelection(idx);
    }
    return result;
}

void ObjectsMapModel::addNewObject(ObjectsMapTreeItem *item)
{
    QTC_ASSERT(item, return );
    QTC_ASSERT(rootItem(), return );

    auto parentItem = rootItem();
    const QString parentName = item->parentName();
    if (!parentName.isEmpty()) {
        if (auto found = findItem(parentName))
            parentItem = found;
    }

    parentItem->appendChild(item);
    emit modelChanged();
}

ObjectsMapTreeItem *ObjectsMapModel::findItem(const QString &search) const
{
    return findNonRootItem(
        [search](ObjectsMapTreeItem *item) { return item->data(0, Qt::DisplayRole) == search; });
}

void ObjectsMapModel::removeSymbolicNameResetReferences(const QString &symbolicName,
                                                        const QString &newRef)
{
    ObjectsMapTreeItem *item = findItem(symbolicName);
    QTC_ASSERT(item, return );

    forAllItems([&symbolicName, &newRef](ObjectsMapTreeItem *item) {
        // ignore invisible root and invalid item
        if (!item->parent() || !item->isValid())
            return;

        PropertiesModel *propertiesModel = item->propertiesModel();
        propertiesModel->modifySpecialProperty(symbolicName, newRef);
        return;
    });

    delete takeItem(item);
    emit modelChanged();
}

void ObjectsMapModel::removeSymbolicNameInvalidateReferences(const QModelIndex &idx)
{
    TreeItem *item = itemForIndex(idx);
    QTC_ASSERT(item, return );

    item->forAllChildren([this](TreeItem *childItem) {
        ObjectsMapTreeItem *objMapItem = static_cast<ObjectsMapTreeItem *>(childItem);
        takeItem(objMapItem);
        addNewObject(objMapItem);
    });

    delete takeItem(item);
    emit modelChanged();
}

void ObjectsMapModel::removeSymbolicName(const QModelIndex &idx)
{
    TreeItem *item = itemForIndex(idx);
    QTC_ASSERT(item, return );
    delete takeItem(item);
    emit modelChanged();
}

QStringList ObjectsMapModel::allSymbolicNames() const
{
    TreeItem *root = rootItem();
    QTC_ASSERT(root, return {});

    QMap<QString, PropertyList> objects;
    forAllItems([&objects](ObjectsMapTreeItem *item) {
        if (item->parent())
            objects.insert(item->data(0, Qt::DisplayRole).toString(), item->properties());
    });
    return objects.keys();
}

void ObjectsMapModel::changeRootItem(ObjectsMapTreeItem *newRoot)
{
    setRootItem(newRoot);
}

void ObjectsMapModel::onNameChanged(const QString &old, const QString &modified)
{
    if (old != modified) {
        // walk over all ObjectsMapTreeItems
        QTC_ASSERT(rootItem(), return );
        forSelectedItems([&old, &modified](ObjectsMapTreeItem *item) {
            if (!item->parent())
                return true;
            PropertiesModel *pm = item->propertiesModel();
            // walk over properties of this object
            QTC_ASSERT(pm->rootItem(), return true);
            pm->forAllItems([&old, &modified](PropertyTreeItem *propItem) {
                const Property &prop = propItem->property();
                if ((prop.isContainer() || prop.isRelativeWidget()) && prop.m_value == old)
                    propItem->setData(2, modified, Qt::EditRole);
            });
            return true;
        });
        emit modelChanged();
    }
}

void ObjectsMapModel::onPropertyChanged(
    ObjectsMapTreeItem *item, const QString &old, const QString &modified, int row, int column)
{
    QTC_ASSERT(item, return );

    if (old == modified)
        return;

    // special handling for changes of container properties
    if (column == 2 || column == 0) {
        PropertiesModel *propModel = item->propertiesModel();
        const QModelIndex propIndex = propModel->index(row, column, QModelIndex());
        auto propertyItem = static_cast<PropertyTreeItem *>(propModel->itemForIndex(propIndex));
        Property property = propertyItem->property();
        if (property.isContainer()) {
            takeItem(item);
            ObjectsMapTreeItem *foundItem = findItem(property.m_value);
            QTC_ASSERT(foundItem, return ); // could not find new parent should not happen
            foundItem->appendChild(item);
            emit requestSelection(indexForItem(item));
        }
    }
    emit modelChanged();
}

void ObjectsMapModel::onPropertyRemoved(ObjectsMapTreeItem *item, const Property &property)
{
    QTC_ASSERT(item, return );

    if (property.isContainer()) {
        takeItem(item);
        QTC_ASSERT(rootItem(), return );
        rootItem()->appendChild(item);
        emit requestSelection(indexForItem(item));
    }
    emit modelChanged();
}

/***************************** SortFilterModel **********************************************/

ObjectsMapSortFilterModel::ObjectsMapSortFilterModel(TreeModel<ObjectsMapTreeItem> *sourceModel,
                                                     QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

bool ObjectsMapSortFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return left.data().toString() > right.data().toString();
}

static bool checkRecursivelyForPattern(const QModelIndex &index,
                                       TreeModel<> *model,
                                       const QString &pattern)
{
    if (index.data().toString().contains(pattern, Qt::CaseInsensitive))
        return true;

    // if no match - check if its properties...
    ObjectsMapTreeItem *item = static_cast<ObjectsMapTreeItem *>(model->itemForIndex(index));
    if (item && anyOf(item->properties(), [&pattern](const Property &p) {
            return p.m_value.contains(pattern, Qt::CaseInsensitive);
        })) {
        return true;
    }
    // ...or a child might have a match
    for (int row = 0, childCount = model->rowCount(index); row < childCount; ++row) {
        const QModelIndex child = model->index(row, 0, index);
        if (checkRecursivelyForPattern(child, model, pattern))
            return true;
    }
    return false;
}

bool ObjectsMapSortFilterModel::filterAcceptsRow(int sourceRow,
                                                 const QModelIndex &sourceParent) const
{
    const QString pattern = filterRegularExpression().pattern();

    if (pattern.isEmpty())
        return true;

    TreeModel<> *srcModel = static_cast<TreeModel<> *>(sourceModel());
    const QModelIndex index = srcModel->index(sourceRow, 0, sourceParent);
    if (!index.isValid())
        return false;

    return checkRecursivelyForPattern(index, srcModel, pattern);
}

} // namespace Internal
} // namespace Squish
