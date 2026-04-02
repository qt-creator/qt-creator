// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "groupedmodel.h"

#include "guiutils.h"
#include "qtcassert.h"
#include "stringutils.h"
#include "utilstr.h"

#include <QFont>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>
#include <QTreeView>

namespace Utils {

class GroupedModel::DisplayModel final : public QAbstractItemModel
{
public:
    explicit DisplayModel(QAbstractItemModel *base)
        : m_base(base)
    {}

    ~DisplayModel() { qDeleteAll(m_filters); }

    void setFilters(const QString &defaultTitle,
                    const QList<std::pair<QString, Filter>> &sections);
    void setExtraFilter(const Filter &filter);

    const Filter &extraFilter() const { return m_extraFilter; }

    QModelIndex mapToSource(const QModelIndex &index) const;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

private:
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    class FilterModel final : public QSortFilterProxyModel
    {
    public:
        explicit FilterModel(const Filter &filter, const DisplayModel *dm)
            : m_filter(filter), m_displayModel(dm)
        {}

        Filter filter() const { return m_filter; }
        void invalidate() { invalidateFilter(); }

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &) const final
        {
            const Filter &extra = m_displayModel->extraFilter();
            if (extra && !extra(sourceRow))
                return false;
            return m_filter(sourceRow);
        }

        bool lessThan(const QModelIndex &source_left,  const QModelIndex &source_right) const final
        {
            const QString l = sourceModel()->data(source_left).toString();
            const QString r = sourceModel()->data(source_right).toString();
            return caseFriendlyCompare(l, r) < 0;
        }

    private:
        Filter m_filter;
        const DisplayModel *m_displayModel;
    };

    void addSection(const QString &title, const Filter &filter);

    QAbstractItemModel * const m_base;
    QList<FilterModel *> m_filters;
    Filter m_extraFilter;
};

void GroupedModel::DisplayModel::setFilters(const QString &defaultTitle,
                                            const QList<std::pair<QString, Filter>> &sections)
{
    QTC_ASSERT(m_filters.isEmpty(), qDebug() << "Settings filters twice is not supported"; return);
    // Must fire before FilterModels process the source reset.
    connect(m_base, &QAbstractItemModel::modelAboutToBeReset,
            this, [this] { beginResetModel(); });

    // Unfiltered section.
    addSection(defaultTitle, [this](int sourceRow) {
        for (int i = 1; i < m_filters.size(); ++i) {
            if (m_filters.at(i)->filter()(sourceRow))
                return false;
        }
        return true;
    });

    // Filtered sections.
    for (const auto &[title, filter] : sections)
        addSection(title, filter);

    // Keep last: all FilterModels are now wired to m_base::modelReset, so they
    // rebuild their mappings before endResetModel() notifies the views.
    connect(m_base, &QAbstractItemModel::modelReset, this, [this] { endResetModel(); });
}

void GroupedModel::DisplayModel::setExtraFilter(const Filter &filter)
{
    m_extraFilter = filter;
    for (FilterModel *fm : m_filters)
        fm->invalidate();
}

void GroupedModel::DisplayModel::addSection(const QString &title, const Filter &filter)
{
    auto model = new FilterModel(filter, this);
    model->setObjectName(title);
    model->setSourceModel(m_base);
    m_filters.append(model);

    connect(model, &QAbstractItemModel::dataChanged, this, [this, model]
                (const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
        const int fi = m_filters.indexOf(model);
        emit dataChanged(createIndex(topLeft.row(), topLeft.column(), quintptr(fi)),
                         createIndex(bottomRight.row(), bottomRight.column(), quintptr(fi)), roles);
    });
    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, [this, model](const QModelIndex &, int first, int last) {
        beginInsertRows(createIndex(m_filters.indexOf(model), 0, quintptr(-1)), first, last);
    });
    connect(model, &QAbstractItemModel::rowsInserted,
            this, [this](const QModelIndex &, int, int) { endInsertRows(); });
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, [this, model](const QModelIndex &, int first, int last) {
        beginRemoveRows(createIndex(m_filters.indexOf(model), 0, quintptr(-1)), first, last);
    });
    connect(model, &QAbstractItemModel::rowsRemoved,
            this, [this](const QModelIndex &, int, int) { endRemoveRows(); });

    connect(model, &QAbstractItemModel::layoutAboutToBeChanged,
            this, [this, model](const QList<QPersistentModelIndex> &,
                                QAbstractItemModel::LayoutChangeHint hint) {
        emit layoutAboutToBeChanged({}, hint);
        // Capture after emit: the view may create persistent indices while processing
        // layoutAboutToBeChanged, and those need to be included in the remapping.
        QModelIndexList sourceIndices;
        const int fi = m_filters.indexOf(model);
        for (const QModelIndex &idx : persistentIndexList()) {
            if (idx.isValid() && int(idx.internalId()) == fi)
                sourceIndices.append(model->mapToSource(model->index(idx.row(), 0)));
        }
        connect(model, &QAbstractItemModel::layoutChanged, this, [this, model, fi, sourceIndices] {
            int i = 0;
            for (const QModelIndex &idx : persistentIndexList()) {
                if (!idx.isValid() || int(idx.internalId()) != fi)
                    continue;
                const QModelIndex newProxy = model->mapFromSource(sourceIndices.at(i++));
                changePersistentIndex(idx, newProxy.isValid()
                    ? createIndex(newProxy.row(), idx.column(), quintptr(fi)) : QModelIndex{});
            }
            emit layoutChanged();
        }, Qt::SingleShotConnection);
    });
}

QModelIndex GroupedModel::DisplayModel::mapToSource(const QModelIndex &index) const
{
    if (!index.isValid() || index.internalId() == quintptr(-1))
        return {};
    FilterModel *child = m_filters.at(int(index.internalId()));
    return child->mapToSource(child->index(index.row(), index.column()));
}

QModelIndex GroupedModel::DisplayModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid())
        return {};
    for (int i = 0; i < m_filters.size(); ++i) {
        const QModelIndex proxyIndex = m_filters.at(i)->mapFromSource(sourceIndex);
        if (proxyIndex.isValid())
            return createIndex(proxyIndex.row(), 0, quintptr(i));
    }
    return {};
}

QModelIndex GroupedModel::DisplayModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column < 0 || column >= columnCount({}))
        return {};
    if (!parent.isValid()) {
        if (row < 0 || row >= m_filters.size())
            return {};
        // Parent is invalid, we are on the first level, i.e. the root of one
        // of the groups. use -1 as magic marker, and for row the index
        // of the group in the group list.
        return createIndex(row, column, quintptr(-1));
    }
    if (parent.internalId() != quintptr(-1))
        return {};
    FilterModel *child = m_filters.at(parent.row());
    if (row < 0 || row >= child->rowCount())
        return {};
    // Leafnodes are all in the second level, and represent one of the real
    // values. Use as internal value the index of the group they belong
    // to and the index of the real value in its group as row.
    return createIndex(row, column, quintptr(parent.row()));
}

QModelIndex GroupedModel::DisplayModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.internalId() == quintptr(-1))
        return {};
    return createIndex(int(index.internalId()), 0, quintptr(-1));
}

int GroupedModel::DisplayModel::rowCount(const QModelIndex &parent) const
{
    // Root.
    if (!parent.isValid())
        return m_filters.count();
    // Group entries.
    if (parent.internalId() == quintptr(-1))
        return m_filters.at(parent.row())->rowCount();
    // Leaves have no children.
    return 0;
}

int GroupedModel::DisplayModel::columnCount(const QModelIndex &parent) const
{
    // Leaf items have no children
    if (parent.isValid() && parent.internalId() != quintptr(-1))
        return 0;
    return m_base->columnCount();
}

QVariant GroupedModel::DisplayModel::data(const QModelIndex &index, int role) const
{
    // Root has no data
    if (!index.isValid())
        return {};
    // Groups have DisplayRole, which is stored as objectName() of its FilterModel
    if (index.internalId() == quintptr(-1)) {
        if (role != Qt::DisplayRole || index.column() != 0)
            return {};
        return m_filters.at(index.row())->objectName();
    }
    // Leaf data is coming from the real entries.
    FilterModel *child = m_filters.at(int(index.internalId()));
    return child->data(child->index(index.row(), index.column()), role);
}

QVariant GroupedModel::DisplayModel::headerData(int section, Qt::Orientation orientation,
                                                           int role) const
{
    return m_base->headerData(section, orientation, role);
}

void GroupedModel::DisplayModel::sort(int column, Qt::SortOrder order)
{
    for (FilterModel *fm : m_filters)
        fm->sort(column, order);
}

// GroupedModel

GroupedModel::GroupedModel()
    : m_displayModel(new DisplayModel(this))
{}

GroupedModel::~GroupedModel()
{
    delete m_displayModel;
}

QAbstractItemModel *GroupedModel::groupedDisplayModel() const
{
    return m_displayModel;
}

int GroupedModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_volatileVariants.size();
}

int GroupedModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_columnCount;
}

QVariant GroupedModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_volatileVariants.size())
        return {};
    const int row = index.row();
    const QVariant item = m_volatileVariants.at(row);
    if (role == Qt::FontRole) {
        QFont font = variantData(item, index.column(), Qt::FontRole).value<QFont>();
        if (isDirty(row))
            font.setBold(true);
        if (m_removed.at(row))
            font.setStrikeOut(true);
        if (m_volatileDefaultFlag.at(row))
            font.setItalic(true);
        return font;
    }
    QVariant result = variantData(item, index.column(), role);
    if (role == Qt::DisplayRole && index.column() == 0
            && m_volatileDefaultFlag.at(row) && m_showDefault) {
        return Tr::tr("%1 (Default)").arg(result.toString());
    }
    return result;
}

QVariant GroupedModel::headerData(int section, Qt::Orientation, int role) const
{
    if (section < 0 || section >= m_columnCount || role != Qt::DisplayRole)
        return {};
    return m_header.at(section);
}

int GroupedModel::itemCount() const
{
    return m_volatileVariants.size();
}

bool GroupedModel::isAdded(int row) const
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return false);
    return m_added.at(row);
}

bool GroupedModel::isRemoved(int row) const
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return false);
    return m_removed.at(row);
}

bool GroupedModel::isChanged(int row) const
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return false);
    return m_changed.at(row);
}

bool GroupedModel::isDirty(int row) const
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return false);
    if (m_added.at(row) || m_changed.at(row) || m_removed.at(row))
        return true;
    if (m_showDefault && m_volatileDefaultFlag.at(row) != m_defaultFlag.at(row))
        return true;
    return m_volatileVariants.at(row) != m_variants.at(row);
}

bool GroupedModel::isDirty() const
{
    for (int row = 0; row < m_volatileVariants.size(); ++row) {
        if (isDirty(row))
            return true;
    }
    return false;
}

int GroupedModel::defaultRow() const
{
    return m_volatileDefaultFlag.indexOf(true);
}

bool GroupedModel::isDefault(int row) const
{
    return row >= 0 && row < m_volatileDefaultFlag.size() && m_volatileDefaultFlag.at(row);
}

void GroupedModel::setVolatileDefaultRow(int row)
{
    QTC_ASSERT(row >= -1 && (row < 0 || row < m_volatileVariants.size()), return);
    const int old = defaultRow();
    if (old == row)
        return;
    m_volatileDefaultFlag.fill(false);
    if (row >= 0)
        m_volatileDefaultFlag[row] = true;
    if (old >= 0 && old < m_volatileVariants.size())
        notifyRowChanged(old);
    if (row >= 0)
        notifyRowChanged(row);
}

void GroupedModel::setDefaultRow(int row)
{
    m_volatileDefaultFlag.fill(false);
    m_defaultFlag.fill(false);
    if (row >= 0) {
        m_volatileDefaultFlag[row] = true;
        m_defaultFlag[row] = true;
    }
}

void GroupedModel::setShowDefault(bool on)
{
    m_showDefault = on;
}

void GroupedModel::setChanged(int row, bool changed)
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return);
    m_changed[row] = changed;
}

void GroupedModel::notifyRowChanged(int row)
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return);
    emit dataChanged(index(row, 0), index(row, columnCount() - 1));
    checkSettingsDirty();
}

void GroupedModel::notifyAllRowsChanged()
{
    if (m_volatileVariants.isEmpty())
        return;
    emit dataChanged(index(0, 0), index(m_volatileVariants.size() - 1, columnCount() - 1));
    checkSettingsDirty();
}

void GroupedModel::markRemoved(int row)
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return);
    if (m_added.at(row)) {
        beginRemoveRows({}, row, row);
        m_volatileVariants.removeAt(row);
        m_added.removeAt(row);
        m_removed.removeAt(row);
        m_changed.removeAt(row);
        m_volatileDefaultFlag.removeAt(row);
        endRemoveRows();
    } else {
        m_removed[row] = !m_removed[row];
        notifyRowChanged(row);
    }
    checkSettingsDirty();
}

void GroupedModel::removeItem(int row)
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return);
    if (m_added.at(row))
        return;
    beginRemoveRows({}, row, row);
    m_variants.removeAt(row);
    m_volatileVariants.removeAt(row);
    m_added.removeAt(row);
    m_removed.removeAt(row);
    m_changed.removeAt(row);
    m_volatileDefaultFlag.removeAt(row);
    m_defaultFlag.removeAt(row);
    endRemoveRows();
}

void GroupedModel::apply()
{
    QVariantList committed;
    QList<bool> newDefault;
    for (int i = 0; i < m_volatileVariants.size(); ++i) {
        if (!m_removed.at(i)) {
            committed.append(m_volatileVariants.at(i));
            newDefault.append(m_volatileDefaultFlag.at(i));
        }
    }
    m_defaultFlag = newDefault;
    setVariants(committed);
}

void GroupedModel::cancel()
{
    setVariants(m_variants);
}

QVariant GroupedModel::volatileVariant(int row) const
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return {});
    return m_volatileVariants.at(row);
}

const QVariantList &GroupedModel::volatileVariants() const
{
    return m_volatileVariants;
}

void GroupedModel::setVolatileVariant(int row, const QVariant &item)
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return);
    m_volatileVariants[row] = item;
}

int GroupedModel::appendVolatileVariant(const QVariant &item)
{
    const int row = m_volatileVariants.size();
    beginInsertRows({}, row, row);
    m_volatileVariants.append(item);
    m_added.append(true);
    m_removed.append(false);
    m_changed.append(false);
    m_volatileDefaultFlag.append(false);
    endInsertRows();
    checkSettingsDirty();
    return row;
}

int GroupedModel::appendVariant(const QVariant &item)
{
    const int insertPos = m_volatileVariants.size() - m_added.count(true);
    beginInsertRows({}, insertPos, insertPos);
    m_variants.append(item);
    m_volatileVariants.insert(insertPos, item);
    m_added.insert(insertPos, false);
    m_removed.insert(insertPos, false);
    m_changed.insert(insertPos, false);
    m_volatileDefaultFlag.insert(insertPos, false);
    m_defaultFlag.append(false);
    endInsertRows();
    return insertPos;
}

void GroupedModel::setVariants(const QVariantList &items)
{
    beginResetModel();
    m_variants = items;
    m_volatileVariants = items;
    m_added = QList<bool>(items.size(), false);
    m_removed = QList<bool>(items.size(), false);
    m_changed = QList<bool>(items.size(), false);
    m_volatileDefaultFlag = m_defaultFlag;
    endResetModel();
}

void GroupedModel::setHeader(const QStringList &header)
{
    m_header = header;
    m_columnCount = header.size();
}

QModelIndex GroupedModel::mapToSource(const QModelIndex &index) const
{
    return m_displayModel->mapToSource(index);
}

QModelIndex GroupedModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    return m_displayModel->mapFromSource(sourceIndex);
}

void GroupedModel::setFilters(const QString &defaultTitle,
                              const QList<std::pair<QString, Filter>> &sections)
{
    m_displayModel->setFilters(defaultTitle, sections);
}

void GroupedModel::setExtraFilter(const Filter &filter)
{
    m_displayModel->setExtraFilter(filter);
}

const QVariantList &GroupedModel::variants() const
{
    return m_variants;
}

// GroupedView

GroupedView::GroupedView(GroupedModel &model)
    : m_model(model)
{
    m_view.setRootIsDecorated(false);
    m_view.setExpandsOnDoubleClick(false);
    m_view.setModel(model.groupedDisplayModel());
    m_view.setUniformRowHeights(true);
    m_view.setSelectionMode(QAbstractItemView::SingleSelection);
    m_view.setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view.setSortingEnabled(true);
    m_view.sortByColumn(0, Qt::AscendingOrder);
    m_view.expandAll();

    QHeaderView *header = m_view.header();
    header->setStretchLastSection(true);
    for (int i = 0; i < model.columnCount() - 1; ++i)
        header->setSectionResizeMode(i, QHeaderView::ResizeToContents);

    connect(model.groupedDisplayModel(), &QAbstractItemModel::modelAboutToBeReset, this, [this] {
        const int row = currentRow();
        m_savedVariant = row >= 0 ? m_model.volatileVariant(row) : QVariant{};
    });

    connect(model.groupedDisplayModel(), &QAbstractItemModel::modelReset, this, [this] {
        m_view.expandAll();
        if (m_savedVariant.isValid()) {
            for (int row = 0; row < m_model.itemCount(); ++row) {
                if (m_model.volatileVariant(row) == m_savedVariant) {
                    selectRow(row);
                    return;
                }
            }
        }
        emit currentRowChanged(-1, -1);
    });

    connect(m_view.selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](QItemSelection selected, QItemSelection deselected) {
        const QModelIndex previous = deselected.isEmpty() ? QModelIndex{}
                                                          : deselected.indexes().first();
        const QModelIndex current = selected.isEmpty() ? QModelIndex{}
                                                       : selected.indexes().first();
        emit currentRowChanged(m_model.mapToSource(previous).row(),
                               m_model.mapToSource(current).row());
    });
}

QTreeView &GroupedView::view()
{
    return m_view;
}

int GroupedView::currentRow() const
{
    return m_model.mapToSource(m_view.selectionModel()->currentIndex()).row();
}

void GroupedView::selectRow(int row)
{
    const QModelIndex idx = row >= 0 ? m_model.mapFromSource(m_model.index(row, 0)) : QModelIndex{};
    m_view.selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
    m_view.selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect
                                      | QItemSelectionModel::Rows);
}

void GroupedView::scrollToRow(int row)
{
    if (row >= 0)
        m_view.scrollTo(m_model.mapFromSource(m_model.index(row, 0)));
}

void GroupedView::removeCurrent()
{
    const int row = currentRow();
    QTC_ASSERT(row >= 0, return);
    m_model.markRemoved(row);
}

} // namespace Utils
