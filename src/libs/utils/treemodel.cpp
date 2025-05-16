// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "treemodel.h"

#include "qtcassert.h"
#include "stringutils.h"

#include <QStack>
#include <QSize>

#define USE_MODEL_TEST 0
#define USE_INDEX_CHECKS 0

#if USE_INDEX_CHECKS
#define CHECK_INDEX(index) \
    do { \
        if (index.isValid()) { \
            QTC_CHECK(index.model() == this); \
        } else { \
            QTC_CHECK(index.model() == 0); \
        } \
    } while (false)
#else
#define CHECK_INDEX(index)
#endif

#if USE_MODEL_TEST

namespace  {

class ModelTest : public QObject
{
public:
    ModelTest(QAbstractItemModel *model, QObject *parent = 0);

private:
    void nonDestructiveBasicTest();
    void rowCount();
    void columnCount();
    void hasIndex();
    void index();
    void parent();
    void data();

protected:
    void runAllTests();
    void layoutAboutToBeChanged();
    void layoutChanged();
    void rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void rowsInserted(const QModelIndex & parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex & parent, int start, int end);

private:
    void checkChildren(const QModelIndex &parent, int currentDepth = 0);

    QAbstractItemModel *model;

    struct Changing
    {
        QModelIndex parent;
        int oldSize;
        QVariant last;
        QVariant next;
    };
    QStack<Changing> insert;
    QStack<Changing> remove;

    bool fetchingMore;

    QList<QPersistentModelIndex> changing;
};

/*!
    \internal
    Connect to all of the models signals.  Whenever anything happens
    recheck everything.
*/
ModelTest::ModelTest(QAbstractItemModel *_model, QObject *parent) : QObject(parent), model(_model), fetchingMore(false)
{
    Q_ASSERT(model);

    connect(model, &QAbstractItemModel::columnsAboutToBeInserted,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::columnsAboutToBeRemoved,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::columnsInserted,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::columnsRemoved,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::dataChanged,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::headerDataChanged,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::layoutAboutToBeChanged, this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::layoutChanged, this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::modelReset, this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &ModelTest::runAllTests);
    connect(model, &QAbstractItemModel::rowsRemoved,
            this, &ModelTest::runAllTests);

    // Special checks for inserting/removing
    connect(model, &QAbstractItemModel::layoutAboutToBeChanged,
            this, &ModelTest::layoutAboutToBeChanged);
    connect(model, &QAbstractItemModel::layoutChanged,
            this, &ModelTest::layoutChanged);

    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, &ModelTest::rowsAboutToBeInserted);
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &ModelTest::rowsAboutToBeRemoved);
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &ModelTest::rowsInserted);
    connect(model, &QAbstractItemModel::rowsRemoved,
            this, &ModelTest::rowsRemoved);

    runAllTests();
}

void ModelTest::runAllTests()
{
    if (fetchingMore)
        return;
    nonDestructiveBasicTest();
    rowCount();
    columnCount();
    hasIndex();
    index();
    parent();
    data();
}

/*!
    \internal
    nonDestructiveBasicTest tries to call a number of the basic functions (not all)
    to make sure the model doesn't outright segfault, testing the functions that makes sense.
*/
void ModelTest::nonDestructiveBasicTest()
{
    Q_ASSERT(model->buddy(QModelIndex()) == QModelIndex());
    model->canFetchMore(QModelIndex());
    Q_ASSERT(model->columnCount(QModelIndex()) >= 0);
    Q_ASSERT(model->data(QModelIndex()) == QVariant());
    fetchingMore = true;
    model->fetchMore(QModelIndex());
    fetchingMore = false;
    Qt::ItemFlags flags = model->flags(QModelIndex());
    Q_ASSERT(flags == Qt::ItemIsDropEnabled || flags == 0);
    model->hasChildren(QModelIndex());
    model->hasIndex(0, 0);
    model->headerData(0, Qt::Horizontal);
    model->index(0, 0);
    model->itemData(QModelIndex());
    QVariant cache;
    model->match(QModelIndex(), -1, cache);
    model->mimeTypes();
    QModelIndex m1 = model->parent(QModelIndex());
    QModelIndex m2 = QModelIndex();
    Q_ASSERT(m1 == m2);
    Q_ASSERT(model->parent(QModelIndex()) == QModelIndex());
    Q_ASSERT(model->rowCount() >= 0);
    QVariant variant;
    model->setData(QModelIndex(), variant, -1);
    model->setHeaderData(-1, Qt::Horizontal, QVariant());
    model->setHeaderData(999999, Qt::Horizontal, QVariant());
    QMap<int, QVariant> roles;
    model->sibling(0, 0, QModelIndex());
    model->span(QModelIndex());
    model->supportedDropActions();
}

/*!
    \internal
    Tests model's implementation of QAbstractItemModel::rowCount() and hasChildren()

    Models that are dynamically populated are not as fully tested here.
 */
void ModelTest::rowCount()
{
    // check top row
    QModelIndex topIndex = model->index(0, 0, QModelIndex());
    int rows = model->rowCount(topIndex);
    Q_ASSERT(rows >= 0);
    if (rows > 0)
        Q_ASSERT(model->hasChildren(topIndex) == true);

    QModelIndex secondLevelIndex = model->index(0, 0, topIndex);
    if (secondLevelIndex.isValid()) { // not the top level
        // check a row count where parent is valid
        rows = model->rowCount(secondLevelIndex);
        Q_ASSERT(rows >= 0);
        if (rows > 0)
            Q_ASSERT(model->hasChildren(secondLevelIndex) == true);
    }

    // The models rowCount() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*!
    \internal
    Tests model's implementation of QAbstractItemModel::columnCount() and hasChildren()
 */
void ModelTest::columnCount()
{
    // check top row
    QModelIndex topIndex = model->index(0, 0, QModelIndex());
    Q_ASSERT(model->columnCount(topIndex) >= 0);

    // check a column count where parent is valid
    QModelIndex childIndex = model->index(0, 0, topIndex);
    if (childIndex.isValid())
        Q_ASSERT(model->columnCount(childIndex) >= 0);

    // columnCount() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*!
    \internal
    Tests model's implementation of QAbstractItemModel::hasIndex()
 */
void ModelTest::hasIndex()
{
    // Make sure that invalid values returns an invalid index
    Q_ASSERT(model->hasIndex(-2, -2) == false);
    Q_ASSERT(model->hasIndex(-2, 0) == false);
    Q_ASSERT(model->hasIndex(0, -2) == false);

    int rows = model->rowCount();
    int columns = model->columnCount();

    // check out of bounds
    Q_ASSERT(model->hasIndex(rows, columns) == false);
    Q_ASSERT(model->hasIndex(rows + 1, columns + 1) == false);

    if (rows > 0)
        Q_ASSERT(model->hasIndex(0, 0) == true);

    // hasIndex() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*!
    \internal
    Tests model's implementation of QAbstractItemModel::index()
 */
void ModelTest::index()
{
    // Make sure that invalid values returns an invalid index
    Q_ASSERT(model->index(-2, -2) == QModelIndex());
    Q_ASSERT(model->index(-2, 0) == QModelIndex());
    Q_ASSERT(model->index(0, -2) == QModelIndex());

    int rows = model->rowCount();
    int columns = model->columnCount();

    if (rows == 0)
        return;

    // Catch off by one errors
    QModelIndex tmp;
    tmp = model->index(rows, columns);
    Q_ASSERT(tmp == QModelIndex());
    tmp = model->index(0, 0);
    Q_ASSERT(tmp.isValid() == true);

    // Make sure that the same index is *always* returned
    QModelIndex a = model->index(0, 0);
    QModelIndex b = model->index(0, 0);
    Q_ASSERT(a == b);

    // index() is tested more extensively in checkChildren(),
    // but this catches the big mistakes
}

/*!
    \internal
    Tests model's implementation of QAbstractItemModel::parent()
 */
void ModelTest::parent()
{
    // Make sure the model wont crash and will return an invalid QModelIndex
    // when asked for the parent of an invalid index.
    Q_ASSERT(model->parent(QModelIndex()) == QModelIndex());

    if (model->rowCount() == 0)
        return;

    QModelIndex tmp;

    // Column 0                | Column 1    |
    // QModelIndex()           |             |
    //    \- topIndex          | topIndex1   |
    //         \- childIndex   | childIndex1 |

    // Common error test #1, make sure that a top level index has a parent
    // that is a invalid QModelIndex.
    QModelIndex topIndex = model->index(0, 0, QModelIndex());
    tmp = model->parent(topIndex);
    Q_ASSERT(tmp == QModelIndex());

    // Common error test #2, make sure that a second level index has a parent
    // that is the first level index.
    if (model->rowCount(topIndex) > 0) {
        QModelIndex childIndex = model->index(0, 0, topIndex);
        tmp = model->parent(childIndex);
        Q_ASSERT(tmp == topIndex);
    }

    // Common error test #3, the second column should NOT have the same children
    // as the first column in a row.
    // Usually the second column shouldn't have children.
    QModelIndex topIndex1 = model->index(0, 1, QModelIndex());
    if (model->rowCount(topIndex1) > 0) {
        QModelIndex childIndex = model->index(0, 0, topIndex);
        QModelIndex childIndex1 = model->index(0, 0, topIndex1);
        Q_ASSERT(childIndex != childIndex1);
    }

    // Full test, walk n levels deep through the model making sure that all
    // parent's children correctly specify their parent.
    checkChildren(QModelIndex());
}

/*!
    \internal
    Called from the parent() test.

    A model that returns an index of parent X should also return X when asking
    for the parent of the index.

    This recursive function does pretty extensive testing on the whole model in an
    effort to catch edge cases.

    This function assumes that rowCount(), columnCount() and index() already work.
    If they have a bug it will point it out, but the above tests should have already
    found the basic bugs because it is easier to figure out the problem in
    those tests then this one.
 */
void ModelTest::checkChildren(const QModelIndex &parent, int currentDepth)
{
    QModelIndex tmp;

    // First just try walking back up the tree.
    QModelIndex p = parent;
    while (p.isValid())
        p = p.parent();

    // For models that are dynamically populated
    if (model->canFetchMore(parent)) {
        fetchingMore = true;
        model->fetchMore(parent);
        fetchingMore = false;
    }

    int rows = model->rowCount(parent);
    int columns = model->columnCount(parent);

    if (rows > 0)
        Q_ASSERT(model->hasChildren(parent));

    // Some further testing against rows(), columns(), and hasChildren()
    Q_ASSERT(rows >= 0);
    Q_ASSERT(columns >= 0);
    if (rows > 0)
        Q_ASSERT(model->hasChildren(parent) == true);

    //qDebug() << "parent:" << model->data(parent).toString() << "rows:" << rows
    //         << "columns:" << columns << "parent column:" << parent.column();

    Q_ASSERT(model->hasIndex(rows + 1, 0, parent) == false);
    for (int r = 0; r < rows; ++r) {
        if (model->canFetchMore(parent)) {
            fetchingMore = true;
            model->fetchMore(parent);
            fetchingMore = false;
        }
        Q_ASSERT(model->hasIndex(r, columns + 1, parent) == false);
        for (int c = 0; c < columns; ++c) {
            Q_ASSERT(model->hasIndex(r, c, parent) == true);
            QModelIndex index = model->index(r, c, parent);
            // rowCount() and columnCount() said that it existed...
            Q_ASSERT(index.isValid() == true);

            // index() should always return the same index when called twice in a row
            QModelIndex modifiedIndex = model->index(r, c, parent);
            Q_ASSERT(index == modifiedIndex);

            // Make sure we get the same index if we request it twice in a row
            QModelIndex a = model->index(r, c, parent);
            QModelIndex b = model->index(r, c, parent);
            Q_ASSERT(a == b);

            // Some basic checking on the index that is returned
            Q_ASSERT(index.model() == model);
            Q_ASSERT(index.row() == r);
            Q_ASSERT(index.column() == c);
            // While you can technically return a QVariant usually this is a sign
            // of an bug in data()  Disable if this really is ok in your model.
            //Q_ASSERT(model->data(index, Qt::DisplayRole).isValid() == true);

            // If the next test fails here is some somewhat useful debug you play with.
            /*
            if (model->parent(index) != parent) {
                qDebug() << r << c << currentDepth << model->data(index).toString()
                         << model->data(parent).toString();
                qDebug() << index << parent << model->parent(index);
                // And a view that you can even use to show the model.
                //QTreeView view;
                //view.setModel(model);
                //view.show();
            }*/

            // Check that we can get back our real parent.
            //qDebug() << "TTT 1: " << model->parent(index);
            //qDebug() << "TTT 2: " << parent;
            //qDebug() << "TTT 3: " << index;
            tmp = model->parent(index);
            Q_ASSERT(tmp == parent);

            // recursively go down the children
            if (model->hasChildren(index) && currentDepth < 10 ) {
                //qDebug() << r << c << "has children" << model->rowCount(index);
                checkChildren(index, ++currentDepth);
            }/* else { if (currentDepth >= 10) qDebug() << "checked 10 deep"; };*/

            // make sure that after testing the children that the index doesn't change.
            QModelIndex newerIndex = model->index(r, c, parent);
            Q_ASSERT(index == newerIndex);
        }
    }
}

/*!
    \internal
    Tests model's implementation of QAbstractItemModel::data()
 */
void ModelTest::data()
{
    // Invalid index should return an invalid qvariant
    Q_ASSERT(!model->data(QModelIndex()).isValid());

    if (model->rowCount() == 0)
        return;

    // A valid index should have a valid QVariant data
    Q_ASSERT(model->index(0, 0).isValid());

    // shouldn't be able to set data on an invalid index
    Q_ASSERT(model->setData(QModelIndex(), QLatin1String("foo"), Qt::DisplayRole) == false);

    // General Purpose roles that should return a QString
    QVariant variant = model->data(model->index(0, 0), Qt::ToolTipRole);
    if (variant.isValid())
        Q_ASSERT(variant.canConvert(QMetaType::QString));
    variant = model->data(model->index(0, 0), Qt::StatusTipRole);
    if (variant.isValid())
        Q_ASSERT(variant.canConvert(QMetaType::QString));
    variant = model->data(model->index(0, 0), Qt::WhatsThisRole);
    if (variant.isValid())
        Q_ASSERT(variant.canConvert(QMetaType::QString));

    // General Purpose roles that should return a QSize
    variant = model->data(model->index(0, 0), Qt::SizeHintRole);
    if (variant.isValid())
        Q_ASSERT(variant.canConvert(QVariant::Size));

    // General Purpose roles that should return a QFont
    QVariant fontVariant = model->data(model->index(0, 0), Qt::FontRole);
    if (fontVariant.isValid())
        Q_ASSERT(fontVariant.canConvert(QVariant::Font));

    // Check that the alignment is one we know about
    QVariant textAlignmentVariant = model->data(model->index(0, 0), Qt::TextAlignmentRole);
    if (textAlignmentVariant.isValid()) {
        int alignment = textAlignmentVariant.toInt();
       Q_ASSERT(alignment == (alignment & (Qt::AlignHorizontal_Mask | Qt::AlignVertical_Mask)));
    }

    // General Purpose roles that should return a QColor
    QVariant colorVariant = model->data(model->index(0, 0), Qt::BackgroundColorRole);
    if (colorVariant.isValid())
        Q_ASSERT(colorVariant.canConvert(QVariant::Color));

    colorVariant = model->data(model->index(0, 0), Qt::TextColorRole);
    if (colorVariant.isValid())
        Q_ASSERT(colorVariant.canConvert(QVariant::Color));

    // Check that the "check state" is one we know about.
    QVariant checkStateVariant = model->data(model->index(0, 0), Qt::CheckStateRole);
    if (checkStateVariant.isValid()) {
        int state = checkStateVariant.toInt();
        Q_ASSERT(state == Qt::Unchecked ||
                 state == Qt::PartiallyChecked ||
                 state == Qt::Checked);
    }
}

/*!
    \internal
    Store what is about to be inserted to make sure it actually happens

    \sa rowsInserted()
 */
void ModelTest::rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(end)
    Changing c;
    c.parent = parent;
    c.oldSize = model->rowCount(parent);
    c.last = model->data(model->index(start - 1, 0, parent));
    c.next = model->data(model->index(start, 0, parent));
    insert.push(c);
}

/*!
    \internal
    Confirm that what was said was going to happen actually did

    \sa rowsAboutToBeInserted()
 */
void ModelTest::rowsInserted(const QModelIndex & parent, int start, int end)
{
    Changing c = insert.pop();
    Q_ASSERT(c.parent == parent);
    Q_ASSERT(c.oldSize + (end - start + 1) == model->rowCount(parent));
    Q_ASSERT(c.last == model->data(model->index(start - 1, 0, c.parent)));
    /*
    if (c.next != model->data(model->index(end + 1, 0, c.parent))) {
        qDebug() << start << end;
        for (int i=0; i < model->rowCount(); ++i)
            qDebug() << model->index(i, 0).data().toString();
        qDebug() << c.next << model->data(model->index(end + 1, 0, c.parent));
    }
    */
    Q_ASSERT(c.next == model->data(model->index(end + 1, 0, c.parent)));
}

void ModelTest::layoutAboutToBeChanged()
{
    for (int i = 0; i < qBound(0, model->rowCount(), 100); ++i)
        changing.append(QPersistentModelIndex(model->index(i, 0)));
}

void ModelTest::layoutChanged()
{
    for (int i = 0; i < changing.count(); ++i) {
        QPersistentModelIndex p = changing[i];
        Q_ASSERT(p == model->index(p.row(), p.column(), p.parent()));
    }
    changing.clear();
}

/*!
    \internal
    Store what is about to be inserted to make sure it actually happens

    \sa rowsRemoved()
 */
void ModelTest::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Changing c;
    c.parent = parent;
    c.oldSize = model->rowCount(parent);
    c.last = model->data(model->index(start - 1, 0, parent));
    c.next = model->data(model->index(end + 1, 0, parent));
    remove.push(c);
}

/*!
    \internal
    Confirm that what was said was going to happen actually did

    \sa rowsAboutToBeRemoved()
 */
void ModelTest::rowsRemoved(const QModelIndex & parent, int start, int end)
{
    Changing c = remove.pop();
    Q_ASSERT(c.parent == parent);
    Q_ASSERT(c.oldSize - (end - start + 1) == model->rowCount(parent));
    Q_ASSERT(c.last == model->data(model->index(start - 1, 0, c.parent)));
    Q_ASSERT(c.next == model->data(model->index(start, 0, c.parent)));
}

} // anon namespace

#endif // ModelTest

namespace Utils {

//
// TreeItem
//
TreeItem::TreeItem() = default;

TreeItem::~TreeItem()
{
    QTC_CHECK(m_parent == nullptr);
    QTC_CHECK(m_model == nullptr);
    removeChildrenSilently();
}

TreeItem *TreeItem::childAt(int pos) const
{
    QTC_ASSERT(pos >= 0, return nullptr);
    return pos < childCount() ? *(begin() + pos) : nullptr;
}

int TreeItem::indexOf(const TreeItem *item) const
{
    // We use a handwritten loop here because QList::indexOf() is considerably slower
    // in debug mode due to the use of iterators.
    for (qsizetype i = 0, n = m_children.size(); i < n; ++i)
        if (m_children.at(i) == item)
            return int(i);
    return -1;
}

QVariant TreeItem::data(int column, int role) const
{
    Q_UNUSED(column)
    Q_UNUSED(role)
    return QVariant();
}

bool TreeItem::setData(int column, const QVariant &data, int role)
{
    Q_UNUSED(column)
    Q_UNUSED(data)
    Q_UNUSED(role)
    return false;
}

Qt::ItemFlags TreeItem::flags(int column) const
{
    Q_UNUSED(column)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool TreeItem::hasChildren() const
{
    return canFetchMore() || childCount() > 0;
}

bool TreeItem::canFetchMore() const
{
    return false;
}

void TreeItem::prependChild(TreeItem *item)
{
    insertChild(0, item);
}

void TreeItem::appendChild(TreeItem *item)
{
    insertChild(childCount(), item);
}

void TreeItem::insertChild(int pos, TreeItem *item)
{
    QTC_CHECK(!item->model());
    QTC_CHECK(!item->parent());
    QTC_ASSERT(0 <= pos && pos <= childCount(), return); // '<=' is intentional.

    if (m_model) {
        QModelIndex idx = index();
        m_model->beginInsertRows(idx, pos, pos);
        item->m_parent = this;
        item->propagateModel(m_model);
        m_children.insert(m_children.cbegin() + pos, item);
        m_model->endInsertRows();
    } else {
        item->m_parent = this;
        m_children.insert(m_children.cbegin() + pos, item);
    }
}

void TreeItem::insertOrderedChild(TreeItem *item,
    const std::function<bool (const TreeItem *, const TreeItem *)> &cmp)
{
    auto where = std::lower_bound(begin(), end(), item, cmp);
    insertChild(int(where - begin()), item);
}

void TreeItem::removeChildAt(int pos)
{
    QTC_ASSERT(0 <= pos && pos < m_children.count(), return);

    if (m_model) {
        QModelIndex idx = index();
        m_model->beginRemoveRows(idx, pos, pos);
        removeItemAt(pos);
        m_model->endRemoveRows();
    } else {
        removeItemAt(pos);
    }
}

void TreeItem::removeChildren()
{
    if (childCount() == 0)
        return;
    if (m_model) {
        QModelIndex idx = index();
        m_model->beginRemoveRows(idx, 0, childCount() - 1);
        clear();
        m_model->endRemoveRows();
    } else {
        clear();
    }
}

void TreeItem::removeChildrenSilently()
{
    if (childCount() == 0)
        return;
    clear();
}

void TreeItem::sortChildren(const std::function<bool(const TreeItem *, const TreeItem *)> &cmp)
{
    if (m_model) {
        if (const int n = childCount()) {
            QList<TreeItem *> tmp = m_children;
            std::sort(tmp.begin(), tmp.end(), cmp);
            if (tmp == m_children) {
                // Nothing changed.
            } else {
                QModelIndex idx = index();
                m_model->beginRemoveRows(idx, 0, n - 1);
                m_children.clear();
                m_model->endRemoveRows();
                m_model->beginInsertRows(idx, 0, n - 1);
                tmp.swap(m_children);
                m_model->endInsertRows();
            }
        }
    } else {
        std::sort(m_children.begin(), m_children.end(), cmp);
    }
}

void TreeItem::update()
{
    if (m_model) {
        QModelIndex idx = index();
        emit m_model->dataChanged(idx.sibling(idx.row(), 0),
                                  idx.sibling(idx.row(), m_model->m_columnCount - 1));
    }
}

void TreeItem::updateAll()
{
    update();
    updateChildrenRecursively();
}

void TreeItem::updateChildrenRecursively()
{
    if (!m_model)
        return;

    const qsizetype rowCount = m_children.size();
    if (rowCount == 0)
        return;
    const QModelIndex topLeft = m_model->createIndex(0, 0, m_children.first());
    const QModelIndex bottomRight = rowCount == 1 && m_model->m_columnCount == 1
        ? topLeft
        : m_model->createIndex(m_children.size() - 1, m_model->m_columnCount - 1, m_children.last());
    emit m_model->dataChanged(topLeft, bottomRight);
    for (TreeItem * const item : std::as_const(m_children))
        item->updateChildrenRecursively();
}

void TreeItem::updateColumn(int column)
{
    if (m_model) {
        QModelIndex idx = index();
        emit m_model->dataChanged(idx.sibling(idx.row(), column), idx.sibling(idx.row(), column));
    }
}

TreeItem *TreeItem::firstChild() const
{
    return childCount() == 0 ? nullptr : *begin();
}

TreeItem *TreeItem::lastChild() const
{
    return childCount() == 0 ? nullptr : *(end() - 1);
}

int TreeItem::level() const
{
    int l = 0;
    for (TreeItem *item = this->parent(); item; item = item->parent())
        ++l;
    return l;
}

int TreeItem::indexInParent() const
{
    return m_parent ? m_parent->indexOf(this) : -1;
}

QModelIndex TreeItem::index() const
{
    QTC_ASSERT(m_model, return QModelIndex());
    return m_model->indexForItem(this);
}

QAbstractItemModel *TreeItem::model() const
{
    return m_model;
}

void TreeItem::forAllChildren(const std::function<void (TreeItem *)> &pred) const
{
    for (TreeItem *item : *this) {
        pred(item);
        item->forAllChildren(pred);
    }
}

void TreeItem::forSelectedChildren(const std::function<bool (TreeItem *)> &pred) const
{
    for (TreeItem *item : *this) {
        if (pred(item))
            item->forSelectedChildren(pred);
    }
}

void TreeItem::forChildrenAtLevel(int level, const std::function<void(TreeItem *)> &pred) const
{
    QTC_ASSERT(level > 0, return);
    if (level == 1) {
        for (TreeItem *item : *this)
            pred(item);
    } else {
        for (TreeItem *item : *this)
            item->forChildrenAtLevel(level - 1, pred);
    }
}

TreeItem *TreeItem::findChildAtLevel(int level, const std::function<bool(TreeItem *)> &pred) const
{
    QTC_ASSERT(level > 0, return nullptr);
    if (level == 1) {
        for (TreeItem *item : *this)
            if (pred(item))
                return item;
    } else {
        for (TreeItem *item : *this) {
            if (auto found = item->findChildAtLevel(level - 1, pred))
                return found;
        }
    }
    return nullptr;
}

TreeItem *TreeItem::findAnyChild(const std::function<bool(TreeItem *)> &pred) const
{
    for (TreeItem *item : *this) {
        if (pred(item))
            return item;
        if (TreeItem *found = item->findAnyChild(pred))
            return found;
    }
    return nullptr;
}

TreeItem *TreeItem::reverseFindAnyChild(const std::function<bool (TreeItem *)> &pred) const
{
    auto end = m_children.rend();
    for (auto it = m_children.rbegin(); it != end; ++it) {
        if (TreeItem *found = (*it)->reverseFindAnyChild(pred))
            return found;
        if (pred(*it))
            return *it;
    }
    return nullptr;
}

void TreeItem::clear()
{
    while (childCount() != 0) {
        TreeItem *item = m_children.takeLast();
        item->m_model = nullptr;
        item->m_parent = nullptr;
        delete item;
    }
}

void TreeItem::removeItemAt(int pos)
{
    TreeItem *item = m_children.at(pos);
    item->m_model = nullptr;
    item->m_parent = nullptr;
    delete item;
    m_children.removeAt(pos);
}

void TreeItem::expand()
{
    QTC_ASSERT(m_model, return);
    emit m_model->requestExpansion(index());
}

void TreeItem::collapse()
{
    QTC_ASSERT(m_model, return);
    emit m_model->requestCollapse(index());
}

void TreeItem::propagateModel(BaseTreeModel *m)
{
    QTC_ASSERT(m, return);
    QTC_ASSERT(m_model == nullptr || m_model == m, return);
    if (m && !m_model) {
        m_model = m;
        for (TreeItem *item : *this)
            item->propagateModel(m);
    }
}

/*!
    \class Utils::TreeModel
    \inmodule QtCreator

    \brief The TreeModel class is a convienience base class for models
    to use in a QTreeView.
*/

BaseTreeModel::BaseTreeModel(QObject *parent)
    : QAbstractItemModel(parent),
      m_root(new TreeItem)
{
    m_columnCount = 1;
    m_root->m_model = this;
#if USE_MODEL_TEST
    new ModelTest(this, this);
#endif
}

BaseTreeModel::BaseTreeModel(TreeItem *root, QObject *parent)
    : QAbstractItemModel(parent),
      m_root(root)
{
    m_columnCount = 1;
    m_root->propagateModel(this);
}

BaseTreeModel::~BaseTreeModel()
{
    QTC_ASSERT(m_root, return);
    QTC_ASSERT(m_root->m_parent == nullptr, return);
    QTC_ASSERT(m_root->m_model == this, return);
    m_root->m_model = nullptr;
    delete m_root;
}

QModelIndex BaseTreeModel::parent(const QModelIndex &idx) const
{
    CHECK_INDEX(idx);
    if (!idx.isValid())
        return QModelIndex();

    const TreeItem *item = itemForIndex(idx);
    QTC_ASSERT(item, return QModelIndex());
    TreeItem *parent = item->parent();
    if (!parent || parent == m_root)
        return QModelIndex();

    const TreeItem *grandparent = parent->parent();
    if (!grandparent)
        return QModelIndex();

    // This is on the performance-critical path for ItemViewFind.
    const int i = grandparent->m_children.indexOf(parent);
    return createIndex(i, 0, static_cast<void*>(parent));
}

QModelIndex BaseTreeModel::sibling(int row, int column, const QModelIndex &idx) const
{
    const TreeItem *item = itemForIndex(idx);
    QTC_ASSERT(item, return QModelIndex());
    QModelIndex result;
    if (TreeItem *parent = item->parent()) {
        if (TreeItem *sibl = parent->childAt(row))
            result = createIndex(row, column, static_cast<void*>(sibl));
    }
    return result;
}

int BaseTreeModel::rowCount(const QModelIndex &idx) const
{
    CHECK_INDEX(idx);
    if (!idx.isValid())
        return m_root->childCount();
    if (idx.column() > 0)
        return 0;
    const TreeItem *item = itemForIndex(idx);
    return item ? item->childCount() : 0;
}

int BaseTreeModel::columnCount(const QModelIndex &idx) const
{
    CHECK_INDEX(idx);
    if (idx.column() > 0)
        return 0;
    return m_columnCount;
}

bool BaseTreeModel::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    TreeItem *item = itemForIndex(idx);
    bool res = item ? item->setData(idx.column(), data, role) : false;
    if (res)
        emit dataChanged(idx, idx);
    return res;
}

QVariant BaseTreeModel::data(const QModelIndex &idx, int role) const
{
    TreeItem *item = itemForIndex(idx);
    return item ? item->data(idx.column(), role) : QVariant();
}

QVariant BaseTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < m_header.size())
        return m_header.at(section);
    if (role == Qt::ToolTipRole && section < m_headerToolTip.size())
        return m_headerToolTip.at(section);
    return QVariant();
}

bool BaseTreeModel::hasChildren(const QModelIndex &idx) const
{
    if (idx.column() > 0)
        return false;
    TreeItem *item = itemForIndex(idx);
    return !item || item->hasChildren();
}

Qt::ItemFlags BaseTreeModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return {};
    TreeItem *item = itemForIndex(idx);
    return item ? item->flags(idx.column())
                : (Qt::ItemIsEnabled|Qt::ItemIsSelectable);
}

bool BaseTreeModel::canFetchMore(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;
    TreeItem *item = itemForIndex(idx);
    return item ? item->canFetchMore() : false;
}

void BaseTreeModel::fetchMore(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    TreeItem *item = itemForIndex(idx);
    if (item)
        item->fetchMore();
}

TreeItem *BaseTreeModel::rootItem() const
{
    return m_root;
}

void BaseTreeModel::setRootItem(TreeItem *item)
{
    beginResetModel();
    setRootItemInternal(item);
    endResetModel();
}

void BaseTreeModel::setRootItemInternal(TreeItem *item)
{
    QTC_ASSERT(item, return);
    QTC_ASSERT(item->m_model == nullptr, return);
    QTC_ASSERT(item->m_parent == nullptr, return);
    QTC_ASSERT(item != m_root, return);
    QTC_CHECK(m_root);

    if (m_root) {
        QTC_CHECK(m_root->m_parent == nullptr);
        QTC_CHECK(m_root->m_model == this);
        // needs to be done explicitly before setting the model to 0, otherwise it might lead to a
        // crash inside a view or proxy model, especially if there are selected items
        m_root->removeChildrenSilently();
        m_root->m_model = nullptr;
        delete m_root;
    }
    m_root = item;
    item->propagateModel(this);
}

void BaseTreeModel::setHeader(const QStringList &displays)
{
    m_header = displays;
    m_columnCount = displays.size();
}

void BaseTreeModel::setHeaderToolTip(const QStringList &tips)
{
    m_headerToolTip = tips;
}

QModelIndex BaseTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    CHECK_INDEX(parent);
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *item = itemForIndex(parent);
    QTC_ASSERT(item, return QModelIndex());
    if (row >= item->childCount())
        return QModelIndex();
    return createIndex(row, column, static_cast<void*>(item->childAt(row)));
}

TreeItem *BaseTreeModel::itemForIndex(const QModelIndex &idx) const
{
    CHECK_INDEX(idx);
    TreeItem *item = idx.isValid() ? static_cast<TreeItem*>(idx.internalPointer()) : m_root;
    QTC_ASSERT(item, return nullptr);
    QTC_ASSERT(item->m_model == static_cast<const BaseTreeModel *>(this), return nullptr);
    return item;
}

QModelIndex BaseTreeModel::indexForItem(const TreeItem *item) const
{
    QTC_ASSERT(item, return QModelIndex());
    if (item == m_root)
        return QModelIndex();

    TreeItem *p = item->parent();
    QTC_ASSERT(p, return QModelIndex());

    auto mitem = const_cast<TreeItem *>(item);
    int row = p->indexOf(mitem);
    return createIndex(row, 0, mitem);
}

/*!
  Destroys all items in them model except the invisible root item.
*/
void BaseTreeModel::clear()
{
    if (m_root)
        m_root->removeChildren();
}

/*!
   Removes the specified item from the model.

   \note The item is not destroyed, ownership is effectively passed to the caller.
   */

TreeItem *BaseTreeModel::takeItem(TreeItem *item)
{
#if USE_MODEL_TEST
    (void) new ModelTest(this, this);
#endif

    QTC_ASSERT(item, return item);
    TreeItem *parent = item->parent();
    QTC_ASSERT(parent, return item);
    int pos = parent->indexOf(item);
    QTC_ASSERT(pos != -1, return item);

    QModelIndex idx = indexForItem(parent);
    beginRemoveRows(idx, pos, pos);
    item->m_parent = nullptr;
    item->m_model = nullptr;
    parent->m_children.removeAt(pos);
    endRemoveRows();
    return item;
}

void BaseTreeModel::destroyItem(TreeItem *item)
{
    delete takeItem(item);
}

StaticTreeItem::StaticTreeItem(const QStringList &displays)
    : m_displays(displays)
{
}

StaticTreeItem::StaticTreeItem(const QString &display)
    : m_displays(display)
{
}

StaticTreeItem::StaticTreeItem(const QStringList &displays, const QStringList &toolTips)
    : m_displays(displays)
    , m_toolTips(toolTips)
{}

QVariant StaticTreeItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole && column >= 0 && column < m_displays.size())
        return m_displays.at(column);
    if (role == Qt::ToolTipRole && column >= 0 && column < m_toolTips.size())
        return m_toolTips.at(column);
    return QVariant();
}

Qt::ItemFlags StaticTreeItem::flags(int column) const
{
    Q_UNUSED(column)
    return Qt::ItemIsEnabled;
}

bool SortModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (m_lessThan)
        return m_lessThan(source_left, source_right);
    return caseFriendlyCompare(sourceModel()->data(source_left).toString(),
                               sourceModel()->data(source_right).toString()) < 0;
}

} // namespace Utils
