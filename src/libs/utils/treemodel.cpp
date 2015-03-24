/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "treemodel.h"
#include "qtcassert.h"

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
        Q_ASSERT(variant.canConvert(QVariant::String));
    variant = model->data(model->index(0, 0), Qt::StatusTipRole);
    if (variant.isValid())
        Q_ASSERT(variant.canConvert(QVariant::String));
    variant = model->data(model->index(0, 0), Qt::WhatsThisRole);
    if (variant.isValid())
        Q_ASSERT(variant.canConvert(QVariant::String));

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
    Store what is about to be inserted to make sure it actually happens

    \sa rowsInserted()
 */
void ModelTest::rowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(end);
    Changing c;
    c.parent = parent;
    c.oldSize = model->rowCount(parent);
    c.last = model->data(model->index(start - 1, 0, parent));
    c.next = model->data(model->index(start, 0, parent));
    insert.push(c);
}

/*!
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
TreeItem::TreeItem()
    : m_parent(0), m_model(0), m_displays(0), m_lazy(false), m_populated(false),
      m_flags(Qt::ItemIsEnabled|Qt::ItemIsSelectable)
{
}

TreeItem::TreeItem(const QStringList &displays, int flags)
    : m_parent(0), m_model(0), m_displays(new QStringList(displays)), m_lazy(false), m_populated(false),
      m_flags(flags)
{
}

TreeItem::~TreeItem()
{
    clear();
    delete m_displays;
}

TreeItem *TreeItem::child(int pos) const
{
    ensurePopulated();
    QTC_ASSERT(pos >= 0, return 0);
    return pos < m_children.size() ? m_children.at(pos) : 0;
}

bool TreeItem::isLazy() const
{
    return m_lazy;
}

int TreeItem::rowCount() const
{
    ensurePopulated();
    return m_children.size();
}

void TreeItem::populate()
{
}

QVariant TreeItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole && m_displays && column >= 0 && column < m_displays->size())
        return m_displays->at(column);
    return QVariant();
}

bool TreeItem::setData(int column, const QVariant &data, int role)
{
    Q_UNUSED(column);
    Q_UNUSED(data);
    Q_UNUSED(role);
    return false;
}

Qt::ItemFlags TreeItem::flags(int column) const
{
    Q_UNUSED(column);
    return m_flags;
}

bool TreeItem::hasChildren() const
{
    return canFetchMore() || rowCount() > 0;
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
    insertChild(m_children.size(), item);
}

void TreeItem::insertChild(int pos, TreeItem *item)
{
    QTC_CHECK(!item->parent());
    QTC_ASSERT(0 <= pos && pos <= m_children.size(), return); // '<= size' is intentional.

    if (m_model && !m_lazy) {
        QModelIndex idx = index();
        m_model->beginInsertRows(idx, pos, pos);
        item->m_parent = this;
        item->propagateModel(m_model);
        m_children.insert(m_children.begin() + pos, item);
        m_model->endInsertRows();
    } else {
        item->m_parent = this;
        if (m_model)
            item->propagateModel(m_model);
        m_children.insert(m_children.begin() + pos, item);
    }
}

void TreeItem::removeChildren()
{
    if (rowCount() == 0)
        return;
    if (m_model) {
        QModelIndex idx = index();
        m_model->beginRemoveRows(idx, 0, rowCount() - 1);
        clear();
        m_model->endRemoveRows();
    } else {
        clear();
    }
}

void TreeItem::sortChildren(const std::function<bool(const TreeItem *, const TreeItem *)> &cmp)
{
    if (m_model) {
        m_model->layoutAboutToBeChanged();
        std::sort(m_children.begin(), m_children.end(), cmp);
        m_model->layoutChanged();
    } else {
        std::sort(m_children.begin(), m_children.end(), cmp);
    }
}

void TreeItem::update()
{
    if (m_model) {
        QModelIndex idx = index();
        m_model->dataChanged(idx.sibling(idx.row(), 0), idx.sibling(idx.row(), m_model->m_columnCount - 1));
    }
}

void TreeItem::updateColumn(int column)
{
    if (m_model) {
        QModelIndex idx = index();
        m_model->dataChanged(idx.sibling(idx.row(), column), idx.sibling(idx.row(), column));
    }
}

TreeItem *TreeItem::firstChild() const
{
    return m_children.isEmpty() ? 0 : m_children.first();
}

TreeItem *TreeItem::lastChild() const
{
    return m_children.isEmpty() ? 0 : m_children.last();
}

int TreeItem::level() const
{
    int l = 0;
    for (TreeItem *item = this->parent(); item; item = item->parent())
        ++l;
    return l;
}

void TreeItem::setLazy(bool on)
{
    m_lazy = on;
}

void TreeItem::setFlags(Qt::ItemFlags flags)
{
    m_flags = flags;
}

QModelIndex TreeItem::index() const
{
    QTC_ASSERT(m_model, return QModelIndex());
    return m_model->indexFromItem(this);
}

void TreeItem::setModel(TreeModel *model)
{
    if (m_model == model)
        return;
    m_model = model;
    foreach (TreeItem *item, m_children)
        item->setModel(model);
}

void TreeItem::walkTree(TreeItemVisitor *visitor)
{
    if (visitor->preVisit(this)) {
        ++visitor->m_level;
        visitor->visit(this);
        foreach (TreeItem *item, m_children)
            item->walkTree(visitor);
        --visitor->m_level;
    }
    visitor->postVisit(this);
}

void TreeItem::walkTree(std::function<void (TreeItem *)> f)
{
    f(this);
    foreach (TreeItem *item, m_children)
        item->walkTree(f);
}

void TreeItem::clear()
{
    while (m_children.size()) {
        TreeItem *item = m_children.takeLast();
        item->m_parent = 0;
        delete item;
    }
}

void TreeItem::expand()
{
    QTC_ASSERT(m_model, return);
    m_model->requestExpansion(index());
}

void TreeItem::ensurePopulated() const
{
    if (!m_populated) {
        if (isLazy())
            const_cast<TreeItem *>(this)->populate();
        m_populated = true;
    }
}

void TreeItem::propagateModel(TreeModel *m)
{
    QTC_ASSERT(m, return);
    QTC_ASSERT(m_model == 0 || m_model == m, return);
    if (m && !m_model) {
        m_model = m;
        foreach (TreeItem *item, m_children)
            item->propagateModel(m);
    }
}

/*!
    \class Utils::TreeModel

    \brief The TreeModel class is a convienience base class for models
    to use in a QTreeView.
*/

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent),
      m_root(new TreeItem)
{
    m_columnCount = 1;
    m_root->m_model = this;
#if USE_MODEL_TEST
    new ModelTest(this, this);
#endif
}

TreeModel::TreeModel(TreeItem *root, QObject *parent)
    : QAbstractItemModel(parent),
      m_root(root)
{
    m_columnCount = 1;
    m_root->propagateModel(this);
}

TreeModel::~TreeModel()
{
    delete m_root;
}

QModelIndex TreeModel::parent(const QModelIndex &idx) const
{
    CHECK_INDEX(idx);
    if (!idx.isValid())
        return QModelIndex();

    const TreeItem *item = itemFromIndex(idx);
    QTC_ASSERT(item, return QModelIndex());
    const TreeItem *parent = item->parent();
    if (!parent || parent == m_root)
        return QModelIndex();

    const TreeItem *grandparent = parent->parent();
    if (!grandparent)
        return QModelIndex();

    for (int i = 0, n = grandparent->rowCount(); i < n; ++i)
        if (grandparent->child(i) == parent)
            return createIndex(i, 0, (void*) parent);

    return QModelIndex();
}

int TreeModel::rowCount(const QModelIndex &idx) const
{
    CHECK_INDEX(idx);
    if (!idx.isValid())
        return m_root->rowCount();
    if (idx.column() > 0)
        return 0;
    const TreeItem *item = itemFromIndex(idx);
    QTC_ASSERT(item, return 0);
    return item->rowCount();
}

int TreeModel::columnCount(const QModelIndex &idx) const
{
    CHECK_INDEX(idx);
    if (idx.column() > 0)
        return 0;
    return m_columnCount;
}

bool TreeModel::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    TreeItem *item = itemFromIndex(idx);
    bool res = item ? item->setData(idx.column(), data, role) : false;
    if (res)
        emit dataChanged(idx, idx);
    return res;
}

QVariant TreeModel::data(const QModelIndex &idx, int role) const
{
    TreeItem *item = itemFromIndex(idx);
    return item ? item->data(idx.column(), role) : QVariant();
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < m_header.size())
        return m_header.at(section);
    return QVariant();
}

bool TreeModel::hasChildren(const QModelIndex &idx) const
{
    TreeItem *item = itemFromIndex(idx);
    return !item || item->hasChildren();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return 0;
    TreeItem *item = itemFromIndex(idx);
    return item ? item->flags(idx.column())
                : (Qt::ItemIsEnabled|Qt::ItemIsSelectable);
}

bool TreeModel::canFetchMore(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;
    TreeItem *item = itemFromIndex(idx);
    return item ? item->canFetchMore() : false;
}

void TreeModel::fetchMore(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    TreeItem *item = itemFromIndex(idx);
    if (item)
        item->fetchMore();
}

TreeItem *TreeModel::rootItem() const
{
    return m_root;
}

void TreeModel::setRootItem(TreeItem *item)
{
    delete m_root;
    m_root = item;
    item->setModel(this);
}

void TreeModel::setHeader(const QStringList &displays)
{
    m_header = displays;
    m_columnCount = displays.size();
}

void TreeModel::setColumnCount(int columnCount)
{
    m_columnCount = columnCount;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    CHECK_INDEX(parent);
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const TreeItem *item = itemFromIndex(parent);
    QTC_ASSERT(item, return QModelIndex());
    if (row >= item->rowCount())
        return QModelIndex();
    return createIndex(row, column, (void*)(item->child(row)));
}

TreeItem *TreeModel::itemFromIndex(const QModelIndex &idx) const
{
    CHECK_INDEX(idx);
    TreeItem *item = idx.isValid() ? static_cast<TreeItem*>(idx.internalPointer()) : m_root;
    QTC_ASSERT(item, return 0);
    QTC_ASSERT(item->m_model == this, return 0);
    return item;
}

QModelIndex TreeModel::indexFromItem(const TreeItem *item) const
{
    QTC_ASSERT(item, return QModelIndex());
    if (item == m_root)
        return QModelIndex();

    TreeItem *mitem = const_cast<TreeItem *>(item);
    int row = item->parent()->m_children.indexOf(mitem);
    return createIndex(row, 0, mitem);
}

void TreeModel::removeItems()
{
    if (m_root)
        m_root->removeChildren();
}

UntypedTreeLevelItems TreeModel::untypedLevelItems(int level, TreeItem *start) const
{
    if (start == 0)
        start = m_root;
    return UntypedTreeLevelItems(start, level);
}

UntypedTreeLevelItems TreeModel::untypedLevelItems(TreeItem *start) const
{
    return UntypedTreeLevelItems(start, 1);
}

void TreeModel::removeItem(TreeItem *item)
{
#if USE_MODEL_TEST
    (void) new ModelTest(this, this);
#endif

    QTC_ASSERT(item, return);
    TreeItem *parent = item->parent();
    QTC_ASSERT(parent, return);
    int pos = parent->m_children.indexOf(item);
    QTC_ASSERT(pos != -1, return);

    QModelIndex idx = indexFromItem(parent);
    beginRemoveRows(idx, pos, pos);
    item->m_parent = 0;
    parent->m_children.removeAt(pos);
    endRemoveRows();
}

//
// TreeLevelItems
//

UntypedTreeLevelItems::UntypedTreeLevelItems(TreeItem *item, int level)
    : m_item(item), m_level(level)
{}

UntypedTreeLevelItems::const_iterator::const_iterator(TreeItem *base, int level)
    : m_level(level)
{
    QTC_ASSERT(level > 0, return);
    if (base) {
        // "begin()"
        m_depth = 0;
        // Level x: The item m_item[x] is the m_pos[x]'th child of its
        // parent, out of m_size[x].
        m_pos[0] = 0;
        m_size[0] = 1;
        m_item[0] = base;
        goDown();
    } else {
        // "end()"
        m_depth = -1;
    }
}

UntypedTreeLevelItems::const_iterator UntypedTreeLevelItems::begin() const
{
    return const_iterator(m_item, m_level);
}

UntypedTreeLevelItems::const_iterator UntypedTreeLevelItems::end() const
{
    return const_iterator(0, m_level);
}

} // namespace Utils
