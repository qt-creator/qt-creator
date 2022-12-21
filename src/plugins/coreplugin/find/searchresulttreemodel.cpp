// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchresulttreemodel.h"
#include "searchresulttreeitems.h"
#include "searchresulttreeitemroles.h"

#include <utils/algorithm.h>

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QDebug>

namespace Core {
namespace Internal {

class SearchResultTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SearchResultTreeModel(QObject *parent = nullptr);
    ~SearchResultTreeModel() override;

    void setShowReplaceUI(bool show);
    void setTextEditorFont(const QFont &font, const SearchResultColors &colors);

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QModelIndex next(const QModelIndex &idx, bool includeGenerated = false, bool *wrapped = nullptr) const;
    QModelIndex prev(const QModelIndex &idx, bool includeGenerated = false, bool *wrapped = nullptr) const;

    QList<QModelIndex> addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode);

    static SearchResultTreeItem *treeItemAtIndex(const QModelIndex &idx);

signals:
    void jumpToSearchResult(const QString &fileName, int lineNumber,
                            int searchTermStart, int searchTermLength);

public slots:
    void clear();

private:
    QModelIndex index(SearchResultTreeItem *item) const;
    void addResultsToCurrentParent(const QList<SearchResultItem> &items, SearchResult::AddMode mode);
    QSet<SearchResultTreeItem *> addPath(const QStringList &path);
    QVariant data(const SearchResultTreeItem *row, int role) const;
    bool setCheckState(const QModelIndex &idx, Qt::CheckState checkState, bool firstCall = true);
    void updateCheckStateFromChildren(const QModelIndex &idx, SearchResultTreeItem *item);
    QModelIndex nextIndex(const QModelIndex &idx, bool *wrapped = nullptr) const;
    QModelIndex prevIndex(const QModelIndex &idx, bool *wrapped = nullptr) const;

    SearchResultTreeItem *m_rootItem;
    SearchResultTreeItem *m_currentParent;
    SearchResultColors m_colors;
    QModelIndex m_currentIndex;
    QStringList m_currentPath; // the path that belongs to the current parent
    QFont m_textEditorFont;
    bool m_showReplaceUI;
    bool m_editorFontIsUsed;
};

SearchResultTreeModel::SearchResultTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_currentParent(nullptr)
    , m_showReplaceUI(false)
    , m_editorFontIsUsed(false)
{
    m_rootItem = new SearchResultTreeItem;
    m_textEditorFont = QFont(QLatin1String("Courier"));
}

SearchResultTreeModel::~SearchResultTreeModel()
{
    delete m_rootItem;
}

void SearchResultTreeModel::setShowReplaceUI(bool show)
{
    m_showReplaceUI = show;
    // We cannot send dataChanged for the whole hierarchy in one go,
    // because all items in a dataChanged must have the same parent.
    // Send dataChanged for each parent of children individually...
    QList<QModelIndex> changeQueue;
    changeQueue.append(QModelIndex());
    while (!changeQueue.isEmpty()) {
        const QModelIndex current = changeQueue.takeFirst();
        int childCount = rowCount(current);
        if (childCount > 0) {
            emit dataChanged(index(0, 0, current), index(childCount - 1, 0, current));
            for (int r = 0; r < childCount; ++r)
                changeQueue.append(index(r, 0, current));
        }
    }
}

void SearchResultTreeModel::setTextEditorFont(const QFont &font, const SearchResultColors &colors)
{
    emit layoutAboutToBeChanged();
    m_textEditorFont = font;
    m_colors = colors;
    emit layoutChanged();
}

Qt::ItemFlags SearchResultTreeModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(idx);

    if (idx.isValid()) {
        if (m_showReplaceUI)
            flags |= Qt::ItemIsUserCheckable;
    }

    return flags;
}

QModelIndex SearchResultTreeModel::index(int row, int column,
                                         const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const SearchResultTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = treeItemAtIndex(parent);

    const SearchResultTreeItem *childItem = parentItem->childAt(row);
    if (childItem)
        return createIndex(row, column, const_cast<SearchResultTreeItem *>(childItem));
    else
        return QModelIndex();
}

QModelIndex SearchResultTreeModel::index(SearchResultTreeItem *item) const
{
    return createIndex(item->rowOfItem(), 0, item);
}

QModelIndex SearchResultTreeModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    const SearchResultTreeItem *childItem = treeItemAtIndex(idx);
    const SearchResultTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->rowOfItem(), 0, const_cast<SearchResultTreeItem *>(parentItem));
}

int SearchResultTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    const SearchResultTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = treeItemAtIndex(parent);

    return parentItem->childrenCount();
}

int SearchResultTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

SearchResultTreeItem *SearchResultTreeModel::treeItemAtIndex(const QModelIndex &idx)
{
    return static_cast<SearchResultTreeItem*>(idx.internalPointer());
}

QVariant SearchResultTreeModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid())
        return QVariant();

    QVariant result;

    if (role == Qt::SizeHintRole) {
        int height = QApplication::fontMetrics().height();
        if (m_editorFontIsUsed) {
            const int editorFontHeight = QFontMetrics(m_textEditorFont).height();
            height = qMax(height, editorFontHeight);
        }
        result = QSize(0, height);
    } else {
        result = data(treeItemAtIndex(idx), role);
    }

    return result;
}

bool SearchResultTreeModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        auto checkState = static_cast<Qt::CheckState>(value.toInt());
        return setCheckState(idx, checkState);
    }
    return QAbstractItemModel::setData(idx, value, role);
}

bool SearchResultTreeModel::setCheckState(const QModelIndex &idx, Qt::CheckState checkState, bool firstCall)
{
    SearchResultTreeItem *item = treeItemAtIndex(idx);
    if (item->checkState() == checkState)
        return false;
    item->setCheckState(checkState);
    if (firstCall) {
        emit dataChanged(idx, idx);
        updateCheckStateFromChildren(idx.parent(), item->parent());
    }
    // check children
    if (int children = item->childrenCount()) {
        for (int i = 0; i < children; ++i)
            setCheckState(index(i, 0, idx), checkState, false);
        emit dataChanged(index(0, 0, idx), index(children-1, 0, idx));
    }
    return true;
}

void SearchResultTreeModel::updateCheckStateFromChildren(const QModelIndex &idx,
                                                         SearchResultTreeItem *item)
{
    if (!item || item == m_rootItem)
        return;

    bool hasChecked = false;
    bool hasUnchecked = false;
    for (int i = 0; i < item->childrenCount(); ++i) {
        SearchResultTreeItem *child = item->childAt(i);
        if (child->checkState() == Qt::Checked)
            hasChecked = true;
        else if (child->checkState() == Qt::Unchecked)
            hasUnchecked = true;
        else if (child->checkState() == Qt::PartiallyChecked)
            hasChecked = hasUnchecked = true;
    }
    if (hasChecked && hasUnchecked)
        item->setCheckState(Qt::PartiallyChecked);
    else if (hasChecked)
        item->setCheckState(Qt::Checked);
    else
        item->setCheckState(Qt::Unchecked);
    emit dataChanged(idx, idx);

    updateCheckStateFromChildren(idx.parent(), item->parent());
}

void setDataInternal(const QModelIndex &index, const QVariant &value, int role);

QVariant SearchResultTreeModel::data(const SearchResultTreeItem *row, int role) const
{
    QVariant result;

    switch (role)
    {
    case Qt::CheckStateRole:
        result = row->checkState();
        break;
    case Qt::ToolTipRole:
        result = row->item.lineText().trimmed();
        break;
    case Qt::FontRole:
        if (row->item.useTextEditorFont())
            result = m_textEditorFont;
        else
            result = QVariant();
        break;
    case Qt::ForegroundRole:
        result = m_colors.value(row->item.style()).textForeground;
        break;
    case Qt::BackgroundRole:
        result = m_colors.value(row->item.style()).textBackground;
        break;
    case ItemDataRoles::ResultLineRole:
    case Qt::DisplayRole:
        result = row->item.lineText();
        break;
    case ItemDataRoles::ResultItemRole:
        result = QVariant::fromValue(row->item);
        break;
    case ItemDataRoles::ResultBeginLineNumberRole:
        result = row->item.mainRange().begin.line;
        break;
    case ItemDataRoles::ResultIconRole:
        result = row->item.icon();
        break;
    case ItemDataRoles::ResultHighlightBackgroundColor:
        result = m_colors.value(row->item.style()).highlightBackground;
        break;
    case ItemDataRoles::ResultHighlightForegroundColor:
        result = m_colors.value(row->item.style()).highlightForeground;
        break;
    case ItemDataRoles::FunctionHighlightBackgroundColor:
        result = m_colors.value(row->item.style()).containingFunctionBackground;
        break;
    case ItemDataRoles::FunctionHighlightForegroundColor:
        result = m_colors.value(row->item.style()).containingFunctionForeground;
        break;
    case ItemDataRoles::ResultBeginColumnNumberRole:
        result = row->item.mainRange().begin.column;
        break;
    case ItemDataRoles::SearchTermLengthRole:
        result = row->item.mainRange().length(row->item.lineText());
        break;
    case ItemDataRoles::ContainingFunctionNameRole:
        result = row->item.containingFunctionName().value_or(QString{});
        break;
    case ItemDataRoles::IsGeneratedRole:
        result = row->isGenerated();
        break;
    default:
        result = QVariant();
        break;
    }

    return result;
}

QVariant SearchResultTreeModel::headerData(int section, Qt::Orientation orientation,
                                           int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

/**
 * Makes sure that the nodes for a specific path exist and sets
 * m_currentParent to the last final
 */
QSet<SearchResultTreeItem *> SearchResultTreeModel::addPath(const QStringList &path)
{
    QSet<SearchResultTreeItem *> pathNodes;
    SearchResultTreeItem *currentItem = m_rootItem;
    QModelIndex currentItemIndex = QModelIndex();
    SearchResultTreeItem *partItem = nullptr;
    QStringList currentPath;
    for (const QString &part : path) {
        const int insertionIndex = currentItem->insertionIndex(part, &partItem);
        if (!partItem) {
            SearchResultItem item;
            item.setPath(currentPath);
            item.setLineText(part);
            partItem = new SearchResultTreeItem(item, currentItem);
            if (m_showReplaceUI)
                partItem->setCheckState(Qt::Checked);
            partItem->setGenerated(true);
            beginInsertRows(currentItemIndex, insertionIndex, insertionIndex);
            currentItem->insertChild(insertionIndex, partItem);
            endInsertRows();
        }
        pathNodes << partItem;
        currentItemIndex = index(insertionIndex, 0, currentItemIndex);
        currentItem = partItem;
        currentPath << part;
    }

    m_currentParent = currentItem;
    m_currentPath = currentPath;
    m_currentIndex = currentItemIndex;
    return pathNodes;
}

void SearchResultTreeModel::addResultsToCurrentParent(const QList<SearchResultItem> &items, SearchResult::AddMode mode)
{
    if (!m_currentParent)
        return;

    if (mode == SearchResult::AddOrdered) {
        // this is the mode for e.g. text search
        beginInsertRows(m_currentIndex, m_currentParent->childrenCount(), m_currentParent->childrenCount() + items.count());
        for (const SearchResultItem &item : items) {
            m_currentParent->appendChild(item);
        }
        endInsertRows();
    } else if (mode == SearchResult::AddSorted) {
        for (const SearchResultItem &item : items) {
            SearchResultTreeItem *existingItem;
            const int insertionIndex = m_currentParent->insertionIndex(item, &existingItem);
            if (existingItem) {
                existingItem->setGenerated(false);
                existingItem->item = item;
                QModelIndex itemIndex = index(insertionIndex, 0, m_currentIndex);
                emit dataChanged(itemIndex, itemIndex);
            } else {
                beginInsertRows(m_currentIndex, insertionIndex, insertionIndex);
                m_currentParent->insertChild(insertionIndex, item);
                endInsertRows();
            }
        }
    }
    updateCheckStateFromChildren(index(m_currentParent), m_currentParent);
    emit dataChanged(m_currentIndex, m_currentIndex); // Make sure that the number after the file name gets updated
}

static bool lessThanByPath(const SearchResultItem &a, const SearchResultItem &b)
{
    if (a.path().size() < b.path().size())
        return true;
    if (a.path().size() > b.path().size())
        return false;
    for (int i = 0; i < a.path().size(); ++i) {
        if (a.path().at(i) < b.path().at(i))
            return true;
        if (a.path().at(i) > b.path().at(i))
            return false;
    }
    return false;
}

/**
 * Adds the search result to the list of results, creating nodes for the path when
 * necessary.
 */
QList<QModelIndex> SearchResultTreeModel::addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode)
{
    QSet<SearchResultTreeItem *> pathNodes;
    QList<SearchResultItem> sortedItems = items;
    std::stable_sort(sortedItems.begin(), sortedItems.end(), lessThanByPath);
    QList<SearchResultItem> itemSet;
    for (const SearchResultItem &item : sortedItems) {
        m_editorFontIsUsed |= item.useTextEditorFont();
        if (!m_currentParent || (m_currentPath != item.path())) {
            // first add all the items from before
            if (!itemSet.isEmpty()) {
                addResultsToCurrentParent(itemSet, mode);
                itemSet.clear();
            }
            // switch parent
            pathNodes += addPath(item.path());
        }
        itemSet << item;
    }
    if (!itemSet.isEmpty()) {
        addResultsToCurrentParent(itemSet, mode);
        itemSet.clear();
    }
    QList<QModelIndex> pathIndices;
    for (SearchResultTreeItem *item : std::as_const(pathNodes))
        pathIndices << index(item);
    return pathIndices;
}

void SearchResultTreeModel::clear()
{
    beginResetModel();
    m_currentParent = nullptr;
    m_rootItem->clearChildren();
    m_editorFontIsUsed = false;
    endResetModel();
}

QModelIndex SearchResultTreeModel::nextIndex(const QModelIndex &idx, bool *wrapped) const
{
    // pathological
    if (!idx.isValid())
        return index(0, 0);

    if (rowCount(idx) > 0) {
        // node with children
        return index(0, 0, idx);
    }
    // leaf node
    QModelIndex nextIndex;
    QModelIndex current = idx;
    while (!nextIndex.isValid()) {
        int row = current.row();
        current = current.parent();
        if (row + 1 < rowCount(current)) {
            // Same parent has another child
            nextIndex = index(row + 1, 0, current);
        } else {
            // go up one parent
            if (!current.isValid()) {
                // we start from the beginning
                if (wrapped)
                    *wrapped = true;
                nextIndex = index(0, 0);
            }
        }
    }
    return nextIndex;
}

QModelIndex SearchResultTreeModel::next(const QModelIndex &idx, bool includeGenerated, bool *wrapped) const
{
    QModelIndex value = idx;
    do {
        value = nextIndex(value, wrapped);
    } while (value != idx && !includeGenerated && treeItemAtIndex(value)->isGenerated());
    return value;
}

QModelIndex SearchResultTreeModel::prevIndex(const QModelIndex &idx, bool *wrapped) const
{
    QModelIndex current = idx;
    bool checkForChildren = true;
    if (current.isValid()) {
        int row = current.row();
        if (row > 0) {
            current = index(row - 1, 0, current.parent());
        } else {
            current = current.parent();
            checkForChildren = !current.isValid();
            if (checkForChildren && wrapped) {
                // we start from the end
                *wrapped = true;
            }
        }
    }
    if (checkForChildren) {
        // traverse down the hierarchy
        while (int rc = rowCount(current)) {
            current = index(rc - 1, 0, current);
        }
    }
    return current;
}

QModelIndex SearchResultTreeModel::prev(const QModelIndex &idx, bool includeGenerated, bool *wrapped) const
{
    QModelIndex value = idx;
    do {
        value = prevIndex(value, wrapped);
    } while (value != idx && !includeGenerated && treeItemAtIndex(value)->isGenerated());
    return value;
}

SearchResultFilterModel::SearchResultFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setSourceModel(new SearchResultTreeModel(this));
}

void SearchResultFilterModel::setFilter(SearchResultFilter *filter)
{
    if (m_filter)
        m_filter->disconnect(this);
    m_filter = filter;
    if (m_filter) {
        connect(m_filter, &SearchResultFilter::filterChanged,
                this, [this] {
            invalidateFilter();
            emit filterInvalidated();
        });
    }
    invalidateFilter();
}

void SearchResultFilterModel::setShowReplaceUI(bool show)
{
    sourceModel()->setShowReplaceUI(show);
}

void SearchResultFilterModel::setTextEditorFont(const QFont &font, const SearchResultColors &colors)
{
    sourceModel()->setTextEditorFont(font, colors);
}

QList<QModelIndex> SearchResultFilterModel::addResults(const QList<SearchResultItem> &items,
                                                       SearchResult::AddMode mode)
{
    QList<QModelIndex> sourceIndexes = sourceModel()->addResults(items, mode);
    sourceIndexes = Utils::filtered(sourceIndexes, [this](const QModelIndex &idx) {
        return filterAcceptsRow(idx.row(), idx.parent());
    });
    return Utils::transform(sourceIndexes,
                            [this](const QModelIndex &idx) { return mapFromSource(idx); });
}

void SearchResultFilterModel::clear()
{
    sourceModel()->clear();
}

QModelIndex SearchResultFilterModel::nextOrPrev(const QModelIndex &idx, bool *wrapped,
        const std::function<QModelIndex (const QModelIndex &)> &func) const
{
    if (wrapped)
        *wrapped = false;
    const QModelIndex sourceIndex = mapToSource(idx);
    QModelIndex nextOrPrevSourceIndex = func(sourceIndex);
    while (nextOrPrevSourceIndex != sourceIndex
           && !filterAcceptsRow(nextOrPrevSourceIndex.row(), nextOrPrevSourceIndex.parent())) {
        nextOrPrevSourceIndex = func(nextOrPrevSourceIndex);
    }
    return mapFromSource(nextOrPrevSourceIndex);
}

QModelIndex SearchResultFilterModel::next(const QModelIndex &idx, bool includeGenerated,
                                          bool *wrapped) const
{
    return nextOrPrev(idx, wrapped, [this, includeGenerated, wrapped](const QModelIndex &index) {
        return sourceModel()->next(index, includeGenerated, wrapped); });
}

QModelIndex SearchResultFilterModel::prev(const QModelIndex &idx, bool includeGenerated,
                                          bool *wrapped) const
{
    return nextOrPrev(idx, wrapped, [this, includeGenerated, wrapped](const QModelIndex &index) {
        return sourceModel()->prev(index, includeGenerated, wrapped); });
}

SearchResultTreeItem *SearchResultFilterModel::itemForIndex(const QModelIndex &index) const
{
    return static_cast<SearchResultTreeItem *>(mapToSource(index).internalPointer());
}

bool SearchResultFilterModel::filterAcceptsRow(int source_row,
                                               const QModelIndex &source_parent) const
{
    const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
    const SearchResultTreeItem * const item = SearchResultTreeModel::treeItemAtIndex(idx);
    if (!item)
        return false;
    if (!m_filter)
        return true;
    if (item->item.userData().isValid())
        return m_filter->matches(item->item);
    const int childCount = sourceModel()->rowCount(idx);
    for (int i = 0; i < childCount; ++i) {
        if (filterAcceptsRow(i, idx))
            return true;
    }
    return false;
}

SearchResultTreeModel *SearchResultFilterModel::sourceModel() const
{
    return static_cast<SearchResultTreeModel *>(QSortFilterProxyModel::sourceModel());
}

} // namespace Internal
} // namespace Core

#include <searchresulttreemodel.moc>
