/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "testcodeparser.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <texteditor/texteditor.h>

#include <QIcon>

namespace Autotest {
namespace Internal {

TestTreeModel::TestTreeModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_rootItem(new TestTreeItem(QString(), QString(), TestTreeItem::ROOT)),
    m_autoTestRootItem(new TestTreeItem(tr("Auto Tests"), QString(), TestTreeItem::ROOT, m_rootItem)),
//    m_quickTestRootItem(new TestTreeItem(tr("Qt Quick Tests"), QString(), TestTreeItem::ROOT, m_rootItem)),
    m_parser(new TestCodeParser(this))
{
    m_rootItem->appendChild(m_autoTestRootItem);
//    m_rootItem->appendChild(m_quickTestRootItem);
    m_parser->updateTestTree();

//    CppTools::CppModelManagerInterface *cppMM = CppTools::CppModelManagerInterface::instance();
//    if (cppMM) {
//        // replace later on by
//        // cppMM->registerAstProcessor([this](const CplusPlus::Document::Ptr &doc,
//        //                             const CPlusPlus::Snapshot &snapshot) {
//        //        checkForQtTestStuff(doc, snapshot);
//        //      });
//        connect(cppMM, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
//                this, SLOT(checkForQtTestStuff(CPlusPlus::Document::Ptr)),
//                Qt::DirectConnection);

//    }
}

static TestTreeModel *m_instance = 0;

TestTreeModel *TestTreeModel::instance()
{
    if (!m_instance)
        m_instance = new TestTreeModel;
    return m_instance;
}

TestTreeModel::~TestTreeModel()
{
    delete m_rootItem;
    m_instance = 0;
}

QModelIndex TestTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TestTreeItem *parentItem = parent.isValid()
            ? static_cast<TestTreeItem *>(parent.internalPointer())
            : m_rootItem;

    TestTreeItem *childItem = parentItem->child(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex TestTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TestTreeItem *childItem = static_cast<TestTreeItem *>(index.internalPointer());
    TestTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool TestTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;

    TestTreeItem *item = static_cast<TestTreeItem *>(parent.internalPointer());
    return item->childCount() > 0;
}

int TestTreeModel::rowCount(const QModelIndex &parent) const
{
    TestTreeItem *parentItem = parent.isValid()
            ? static_cast<TestTreeItem *>(parent.internalPointer()) : m_rootItem;
    return parentItem->childCount();
}

int TestTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant TestTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
    if (!item)
        return QVariant();

    if (role == Qt::DisplayRole) {
        if ((item == m_autoTestRootItem && m_autoTestRootItem->childCount() == 0)
                /*|| (item == m_quickTestRootItem && m_quickTestRootItem->childCount() == 0)*/) {
            return QString(item->name() + tr(" (none)"));
        } else {
            return item->name();
        }

        return QVariant(); // TODO ?
    }
    switch(role) {
    case Qt::ToolTipRole:
        return item->filePath();
    case Qt::DecorationRole:
        switch(item->type()) {
        case TestTreeItem::TEST_CLASS:
            return QIcon(QLatin1String(":/images/class.png"));
        case TestTreeItem::TEST_FUNCTION:
            return QIcon(QLatin1String(":/images/func.png"));
        case TestTreeItem::ROOT:
        default:
            return QVariant();
        }
    case Qt::CheckStateRole:
        if (item->type() == TestTreeItem::ROOT)
            return QVariant();
        return item->checked();
    case LinkRole:
        QVariant itemLink;
        TextEditor::TextEditorWidget::Link link(item->filePath(), item->line(), item->column());
        itemLink.setValue(link);
        return itemLink;
    }

    // TODO ?
    return QVariant();
}

bool TestTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (role == Qt::CheckStateRole) {
        TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
        Qt::CheckState old = item->checked();
        item->setChecked((Qt::CheckState)value.toInt());
        if (item->checked() != old) {
            switch(item->type()) {
            case TestTreeItem::TEST_CLASS:
                emit dataChanged(index, index);
                if (item->childCount() > 0) {
                    emit dataChanged(index.child(0, 0), index.child(item->childCount() - 1, 0));
                }
                break;
            case TestTreeItem::TEST_FUNCTION:
                emit dataChanged(index, index);
                emit dataChanged(index.parent(), index.parent());
                break;
            default: // avoid warning regarding unhandled enum member
                break;
            }
            return true;
        }
    }
    return false;
}

Qt::ItemFlags TestTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    TestTreeItem *item = static_cast<TestTreeItem *>(index.internalPointer());
    switch(item->type()) {
    case TestTreeItem::TEST_CLASS:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsTristate | Qt::ItemIsUserCheckable;
    case TestTreeItem::TEST_FUNCTION:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    case TestTreeItem::ROOT:
    default:
        return Qt::ItemIsEnabled;
    }
}

bool TestTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!parent.isValid())
        return false;
    TestTreeItem *parentItem = static_cast<TestTreeItem *>(parent.internalPointer());
    if (!parentItem)
        return false;

    bool subItemsSuccess = true;
    bool itemSuccess = true;
    for (int i = row + count - 1;  i >= row; --i) {
        QModelIndex child = index(i, 0, parent);
        subItemsSuccess &= removeRows(0, rowCount(child), child);
        beginRemoveRows(parent, i, i);
        itemSuccess &= parentItem->removeChild(i);
        endRemoveRows();
    }
    return subItemsSuccess && itemSuccess;
}

bool TestTreeModel::hasTests() const
{
    return m_autoTestRootItem->childCount() > 0 /*|| m_quickTestRootItem->childCount() > 0*/;
}

void TestTreeModel::modifyAutoTestSubtree(int row, TestTreeItem *newItem)
{
    static QVector<int> modificationRoles = QVector<int>() << Qt::DisplayRole
                                                           << Qt::ToolTipRole << LinkRole;
    QModelIndex toBeModifiedIndex = index(0, 0).child(row, 0);
    if (!toBeModifiedIndex.isValid())
        return;
    TestTreeItem *toBeModifiedItem = static_cast<TestTreeItem *>(toBeModifiedIndex.internalPointer());
    if (toBeModifiedItem->modifyContent(newItem))
        emit dataChanged(toBeModifiedIndex, toBeModifiedIndex, modificationRoles);

    // process sub-items as well...
    int childCount = toBeModifiedItem->childCount();
    int newChildCount = newItem->childCount();

    // for keeping the CheckState on modifications
    QHash<QString, Qt::CheckState> originalItems;
    for (int row = 0; row < childCount; ++row) {
        const TestTreeItem *child = toBeModifiedItem->child(row);
        originalItems.insert(child->name(), child->checked());
    }

    if (childCount <= newChildCount) {
        for (int row = 0; row < childCount; ++row) {
            QModelIndex child = toBeModifiedIndex.child(row, 0);
            TestTreeItem *toBeModifiedChild = toBeModifiedItem->child(row);
            TestTreeItem *modifiedChild = newItem->child(row);
            if (toBeModifiedChild->modifyContent(modifiedChild)) {
                emit dataChanged(child, child, modificationRoles);
            }
            if (originalItems.contains(toBeModifiedChild->name())) {
                    Qt::CheckState state = originalItems.value(toBeModifiedChild->name());
                    if (state != toBeModifiedChild->checked()) {
                        toBeModifiedChild->setChecked(state);
                        emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
                }
            } else { // newly added (BAD: happens for renaming as well)
                toBeModifiedChild->setChecked(Qt::Checked);
                emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
            }
        }
        if (childCount < newChildCount) { // add aditional items
            for (int row = childCount; row < newChildCount; ++row) {
                TestTreeItem *newChild = newItem->child(row);
                TestTreeItem *toBeAdded = new TestTreeItem(newChild->name(), newChild->filePath(),
                                                           newChild->type(), toBeModifiedItem);
                toBeAdded->setLine(newChild->line());
                beginInsertRows(toBeModifiedIndex, row, row);
                toBeModifiedItem->appendChild(toBeAdded);
                endInsertRows();
                if (originalItems.contains(toBeAdded->name())
                        && originalItems.value(toBeAdded->name()) != Qt::Checked)
                    toBeAdded->setChecked(originalItems.value(toBeAdded->name()));
            }
        }
    } else {
        for (int row = 0; row < newChildCount; ++row) {
            QModelIndex child = toBeModifiedIndex.child(row, 0);
            TestTreeItem *toBeModifiedChild = toBeModifiedItem->child(row);
            TestTreeItem *modifiedChild = newItem->child(row);
            if (toBeModifiedChild->modifyContent(modifiedChild))
                emit dataChanged(child, child, modificationRoles);
            if (originalItems.contains(toBeModifiedChild->name())) {
                    Qt::CheckState state = originalItems.value(toBeModifiedChild->name());
                    if (state != toBeModifiedChild->checked()) {
                        toBeModifiedChild->setChecked(state);
                        emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
                }
            } else { // newly added (BAD: happens for renaming as well)
                toBeModifiedChild->setChecked(Qt::Checked);
                emit dataChanged(child, child, QVector<int>() << Qt::CheckStateRole);
            }
        } // remove rest of the items
        removeRows(newChildCount, childCount - newChildCount, toBeModifiedIndex);
    }
}

void TestTreeModel::removeAutoTestSubtreeByFilePath(const QString &file)
{
    QModelIndex atRootIndex = index(0, 0);
    int count = rowCount(atRootIndex);
    for (int row = 0; row < count; ++row) {
        QModelIndex childIndex = atRootIndex.child(row, 0);
        TestTreeItem *childItem = static_cast<TestTreeItem *>(childIndex.internalPointer());
        if (file == childItem->filePath()) {
            removeRow(row, atRootIndex);
            break;
        }
    }
}

} // namespace Internal
} // namespace Autotest
