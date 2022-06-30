/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishtesttreemodel.h"
#include "squishfilehandler.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QIcon>

namespace Squish {
namespace Internal {

/**************************** SquishTestTreeItem ***************************************/

SquishTestTreeItem::SquishTestTreeItem(const QString &displayName, Type type)
    : m_displayName(displayName)
    , m_type(type)
    , m_checked(Qt::Checked)
{
    switch (type) {
    case Root:
        m_flags = Qt::NoItemFlags;
        break;
    case SquishSuite:
        m_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserTristate
                  | Qt::ItemIsUserCheckable;
        break;
    case SquishTestCase:
        m_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
        break;
    case SquishSharedFile:
    case SquishSharedFolder:
        m_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    }
}

Qt::ItemFlags SquishTestTreeItem::flags(int /*column*/) const
{
    return m_flags;
}

void SquishTestTreeItem::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
}

void SquishTestTreeItem::setParentName(const QString &parentName)
{
    m_parentName = parentName;
}

void SquishTestTreeItem::setCheckState(Qt::CheckState state)
{
    switch (m_type) {
    case SquishTestCase:
        m_checked = (state == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        static_cast<SquishTestTreeItem *>(parent())->revalidateCheckState();
        break;
    case SquishSuite:
        m_checked = (state == Qt::Unchecked ? Qt::Unchecked : Qt::Checked);
        forChildrenAtLevel(1, [this](TreeItem *item) {
            static_cast<SquishTestTreeItem *>(item)->m_checked = m_checked;
        });
        break;
    default:
        break;
    }
}

bool SquishTestTreeItem::modifyContent(const SquishTestTreeItem &other)
{
    // modification applies only for items of the same type
    if (m_type != other.m_type)
        return false;

    const bool modified = m_displayName != other.m_displayName || m_filePath != other.m_filePath
                          || m_parentName != other.m_parentName;

    m_displayName = other.m_displayName;
    m_filePath = other.m_filePath;
    m_parentName = other.m_parentName;
    return modified;
}

void SquishTestTreeItem::revalidateCheckState()
{
    if (childCount() == 0)
        return;

    bool foundChecked = false;
    bool foundUnchecked = false;

    forChildrenAtLevel(1, [&foundChecked, &foundUnchecked](const TreeItem *item) {
        const SquishTestTreeItem *squishItem = static_cast<const SquishTestTreeItem *>(item);
        foundChecked |= (squishItem->checkState() != Qt::Unchecked);
        foundUnchecked |= (squishItem->checkState() == Qt::Unchecked);
    });
    if (foundChecked && foundUnchecked) {
        m_checked = Qt::PartiallyChecked;
        return;
    }

    m_checked = (foundUnchecked ? Qt::Unchecked : Qt::Checked);
}

/**************************** SquishTestTreeModel **************************************/

static SquishTestTreeModel *m_instance = nullptr;

SquishTestTreeModel::SquishTestTreeModel(QObject *parent)
    : TreeModel<SquishTestTreeItem>(new SquishTestTreeItem(QString(), SquishTestTreeItem::Root),
                                    parent)
    , m_squishSharedFolders(new SquishTestTreeItem(tr("Shared Folders"), SquishTestTreeItem::Root))
    , m_squishSuitesRoot(new SquishTestTreeItem(tr("Test Suites"), SquishTestTreeItem::Root))
    , m_squishFileHandler(new SquishFileHandler(this))
{
    rootItem()->appendChild(m_squishSharedFolders);
    rootItem()->appendChild(m_squishSuitesRoot);

    connect(m_squishFileHandler,
            &SquishFileHandler::testTreeItemCreated,
            this,
            &SquishTestTreeModel::addTreeItem);
    connect(m_squishFileHandler,
            &SquishFileHandler::suiteTreeItemModified,
            this,
            &SquishTestTreeModel::onSuiteTreeItemModified);
    connect(m_squishFileHandler,
            &SquishFileHandler::suiteTreeItemRemoved,
            this,
            &SquishTestTreeModel::onSuiteTreeItemRemoved);

    m_instance = this;
}

SquishTestTreeModel::~SquishTestTreeModel() {}

SquishTestTreeModel *SquishTestTreeModel::instance()
{
    if (!m_instance)
        m_instance = new SquishTestTreeModel;
    return m_instance;
}

static QIcon treeIcon(SquishTestTreeItem::Type type, int column)
{
    static QIcon icons[5] = {QIcon(),
                             Utils::Icons::OPENFILE.icon(),
                             QIcon(":/fancyactionbar/images/mode_Edit.png"),
                             Utils::Icons::OPENFILE.icon(),
                             QIcon(":/fancyactionbar/images/mode_Edit.png")};
    if (column == 0)
        return icons[type];

    switch (type) {
    case SquishTestTreeItem::SquishSuite:
        if (column == 1)
            return QIcon(":/squish/mages/play.png");
        else if (column == 2)
            return QIcon(":/squish/images/objectsmap.png");
        break;
    case SquishTestTreeItem::SquishTestCase:
        if (column == 1)
            return QIcon(":/squish/images/play.png");
        else if (column == 2)
            return QIcon(":/squish/images/record.png");
        break;
    default: // avoid warning of unhandled enum values
        break;
    }
    return icons[0];
}

QVariant SquishTestTreeModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid())
        return QVariant();

    if (SquishTestTreeItem *item = static_cast<SquishTestTreeItem *>(itemForIndex(idx))) {
        const SquishTestTreeItem::Type type = item->type();
        switch (role) {
        case Qt::DisplayRole:
            if (idx.column() > 0)
                return QVariant();
            switch (type) {
            case SquishTestTreeItem::Root:
                if (!item->hasChildren())
                    return tr("%1 (none)").arg(item->displayName());
                return item->displayName();
            case SquishTestTreeItem::SquishSharedFile:
            case SquishTestTreeItem::SquishSharedFolder:
                return item->displayName();
            default: {
            } // avoid warning regarding unhandled enum values
                return item->displayName();
            }
            break;
        case Qt::DecorationRole:
            return treeIcon(type, idx.column());
        case Qt::CheckStateRole:
            if (idx.column() > 0)
                return QVariant();
            if (type == SquishTestTreeItem::SquishSuite
                || type == SquishTestTreeItem::SquishTestCase)
                return item->checkState();
            return QVariant();
        case Qt::ToolTipRole:
            if (type == SquishTestTreeItem::Root)
                return QVariant();
            if (item->displayName() == item->filePath())
                return item->displayName();

            return item->displayName().append('\n').append(item->filePath());
        case LinkRole:
            return item->filePath();
        case TypeRole:
            return type;
        case DisplayNameRole:
            return item->displayName();
        }
    }
    return TreeModel::data(idx, role);
}

bool SquishTestTreeModel::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    if (!idx.isValid())
        return false;

    if (role == Qt::CheckStateRole) {
        SquishTestTreeItem *item = static_cast<SquishTestTreeItem *>(itemForIndex(idx));
        const SquishTestTreeItem::Type type = item->type();
        if (type == SquishTestTreeItem::SquishSharedFolder
            || type == SquishTestTreeItem::SquishSharedFile)
            return false;
        Qt::CheckState old = item->checkState();
        item->setCheckState((Qt::CheckState) data.toInt());
        if (item->checkState() != old) {
            switch (type) {
            case SquishTestTreeItem::SquishSuite:
                emit dataChanged(idx, idx);
                if (rowCount(idx) > 0)
                    emit dataChanged(index(0, 0, idx), index(rowCount(idx), 0, idx));
                break;
            case SquishTestTreeItem::SquishTestCase:
                emit dataChanged(idx, idx);
                emit dataChanged(idx.parent(), idx.parent());
                break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

int SquishTestTreeModel::columnCount(const QModelIndex & /*idx*/) const
{
    return COLUMN_COUNT;
}

void SquishTestTreeModel::addTreeItem(SquishTestTreeItem *item)
{
    switch (item->type()) {
    case SquishTestTreeItem::SquishSharedFolder:
        m_squishSharedFolders->appendChild(item);
        break;
    case SquishTestTreeItem::SquishSuite:
        m_squishSuitesRoot->appendChild(item);
        break;
    case SquishTestTreeItem::SquishTestCase: {
        const QString folderName = item->parentName();
        Utils::TreeItem *parent
            = m_squishSuitesRoot->findChildAtLevel(1, [folderName](Utils::TreeItem *it) {
                  SquishTestTreeItem *squishItem = static_cast<SquishTestTreeItem *>(it);
                  return squishItem->displayName() == folderName;
              });
        if (parent)
            parent->appendChild(item);
        break;
    }
    case SquishTestTreeItem::SquishSharedFile: {
        const QString folderName = item->parentName();
        Utils::TreeItem *parent
            = m_squishSharedFolders->findChildAtLevel(1, [folderName](Utils::TreeItem *it) {
                  SquishTestTreeItem *squishItem = static_cast<SquishTestTreeItem *>(it);
                  return squishItem->displayName() == folderName;
              });
        if (parent)
            parent->appendChild(item);
        break;
    }
    case SquishTestTreeItem::Root:
    default:
        qWarning("Not supposed to be used for Root items or unknown items.");
        delete item;
        break;
    }
}

void SquishTestTreeModel::removeTreeItem(int row, const QModelIndex &parent)
{
    if (!parent.isValid() || row >= rowCount(parent))
        return;

    Utils::TreeItem *toBeRemoved = itemForIndex(index(row, 0, parent));
    takeItem(toBeRemoved);
    delete toBeRemoved;
}

void SquishTestTreeModel::modifyTreeItem(int row,
                                         const QModelIndex &parent,
                                         const SquishTestTreeItem &modified)
{
    if (!parent.isValid() || row >= rowCount(parent))
        return;

    QModelIndex childIndex = index(row, 0, parent);

    SquishTestTreeItem *toBeModified = static_cast<SquishTestTreeItem *>(itemForIndex(childIndex));

    if (toBeModified->modifyContent(modified))
        emit dataChanged(childIndex, childIndex);
}

void SquishTestTreeModel::removeAllSharedFolders()
{
    m_squishSharedFolders->removeChildren();
}

QStringList SquishTestTreeModel::getSelectedSquishTestCases(const QString &suiteConfPath) const
{
    QStringList result;
    const int count = m_squishSuitesRoot->childCount();

    if (count) {
        for (int row = 0; row < count; ++row) {
            auto suiteItem = static_cast<SquishTestTreeItem *>(m_squishSuitesRoot->childAt(row));
            if (suiteItem->filePath() == suiteConfPath) {
                const int testCaseCount = suiteItem->childCount();
                for (int caseRow = 0; caseRow < testCaseCount; ++caseRow) {
                    auto caseItem = static_cast<SquishTestTreeItem *>(suiteItem->childAt(caseRow));
                    if (caseItem->checkState() == Qt::Checked)
                        result.append(caseItem->displayName());
                }
                break;
            }
        }
    }

    return result;
}

SquishTestTreeItem *SquishTestTreeModel::findSuite(const QString &displayName) const
{
    return findNonRootItem([&displayName](SquishTestTreeItem *item) {
        return item->type() == SquishTestTreeItem::SquishSuite
               && item->displayName() == displayName;
    });
}

void SquishTestTreeModel::onSuiteTreeItemRemoved(const QString &suiteName)
{
    if (SquishTestTreeItem *suite = findSuite(suiteName)) {
        const QModelIndex idx = suite->index();
        removeTreeItem(idx.row(), idx.parent());
    }
}

void SquishTestTreeModel::onSuiteTreeItemModified(SquishTestTreeItem *item, const QString &display)
{
    if (SquishTestTreeItem *suite = findSuite(display)) {
        const QModelIndex idx = suite->index();
        modifyTreeItem(idx.row(), idx.parent(), *item);
    }
    // avoid leaking item even when it cannot be found
    delete item;
}

/************************************** SquishTestTreeSortModel **********************************/

SquishTestTreeSortModel::SquishTestTreeSortModel(SquishTestTreeModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

Utils::TreeItem *SquishTestTreeSortModel::itemFromIndex(const QModelIndex &idx) const
{
    return static_cast<SquishTestTreeModel *>(sourceModel())->itemForIndex(mapToSource(idx));
}

bool SquishTestTreeSortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // root items keep intended order
    const SquishTestTreeItem *leftItem = static_cast<SquishTestTreeItem *>(left.internalPointer());
    if (leftItem->type() == SquishTestTreeItem::Root)
        return left.row() > right.row();

    const QString leftVal = left.data().toString();
    const QString rightVal = right.data().toString();

    return QString::compare(leftVal, rightVal, Qt::CaseInsensitive) > 0;
}

} // namespace Internal
} // namespace Squish
