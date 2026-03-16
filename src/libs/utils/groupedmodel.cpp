// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "groupedmodel.h"

#include "qtcassert.h"

#include <QFont>
#include <QSortFilterProxyModel>

namespace Utils {

class GroupedModel::DisplayModel final : public QAbstractItemModel
{
public:
    explicit DisplayModel(QAbstractItemModel *base)
        : m_base(base)
    {
        auto filter0 = [this](int sourceRow) {
            for (int i = 1; i < m_filters.size(); ++i) {
                if (m_filters.at(i)->filter()(sourceRow))
                    return false;
            }
            return true;
        };
        addFilter(tr("Unfiltered"), filter0);

        connect(m_base, &QAbstractItemModel::modelAboutToBeReset, this, [this] { beginResetModel(); });
        connect(m_base, &QAbstractItemModel::modelReset, this, [this] { endResetModel(); });
    }

    ~DisplayModel() { qDeleteAll(m_filters); }

    void setUnfilteredSectionTitle(const QString &defaultGroupName);
    void addFilter(const QString &title, const Filter &filter);
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

    class FilterModel : public QSortFilterProxyModel
    {
    public:
        explicit FilterModel(const Filter &filter, const DisplayModel *dm)
            : m_filter(filter), m_displayModel(dm) {}

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

    private:
        Filter m_filter;
        const DisplayModel *m_displayModel;
    };

    QAbstractItemModel * const m_base;
    QList<FilterModel *> m_filters;
    Filter m_extraFilter;
};

void GroupedModel::DisplayModel::setUnfilteredSectionTitle(const QString &defaultGroupName)
{
    QTC_ASSERT(!m_filters.isEmpty(), return);
    m_filters.at(0)->setObjectName(defaultGroupName);
}

void GroupedModel::DisplayModel::setExtraFilter(const Filter &filter)
{
    m_extraFilter = filter;
    for (FilterModel *fm : m_filters)
        fm->invalidate();
}

void GroupedModel::DisplayModel::addFilter(const QString &title, const Filter &filter)
{
    auto model = new FilterModel(filter, this);
    model->setObjectName(title);
    model->setSourceModel(m_base);
    //model->sort(0);  //  FIXME: Double-check. This sorts alphabetically, which is not wanted for Kits
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

// GroupedModel

GroupedModel::GroupedModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_displayModel(new DisplayModel(this))
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
        return font;
    }
    return variantData(item, index.column(), role);
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

bool GroupedModel::isDirty(int row) const
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return false);
    if (m_added.at(row))
        return true;
    return m_volatileVariants.at(row) != variants().at(row);
}

void GroupedModel::notifyRowChanged(int row)
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return);
    emit dataChanged(index(row, 0), index(row, columnCount() - 1));
}

void GroupedModel::notifyAllRowsChanged()
{
    if (m_volatileVariants.isEmpty())
        return;
    emit dataChanged(index(0, 0), index(m_volatileVariants.size() - 1, columnCount() - 1));
}

void GroupedModel::markRemoved(int row)
{
    QTC_ASSERT(row >= 0 && row < m_volatileVariants.size(), return);
    if (m_added.at(row)) {
        beginRemoveRows({}, row, row);
        m_volatileVariants.removeAt(row);
        m_added.removeAt(row);
        m_removed.removeAt(row);
        endRemoveRows();
    } else {
        m_removed[row] = !m_removed[row];
        notifyRowChanged(row);
    }
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
    endRemoveRows();
}

void GroupedModel::apply()
{
    QVariantList committed;
    for (int i = 0; i < m_volatileVariants.size(); ++i) {
        if (!m_removed.at(i))
            committed.append(m_volatileVariants.at(i));
    }
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

QModelIndex GroupedModel::appendVolatileVariant(const QVariant &item)
{
    const int row = m_volatileVariants.size();
    beginInsertRows({}, row, row);
    m_volatileVariants.append(item);
    m_added.append(true);
    m_removed.append(false);
    endInsertRows();
    return mapFromSource(index(row, 0));
}

QModelIndex GroupedModel::appendVariant(const QVariant &item)
{
    const int insertPos = m_volatileVariants.size() - m_added.count(true);
    beginInsertRows({}, insertPos, insertPos);
    m_variants.append(item);
    m_volatileVariants.insert(insertPos, item);
    m_added.insert(insertPos, false);
    m_removed.insert(insertPos, false);
    endInsertRows();
    return index(insertPos, 0);
}

void GroupedModel::setVariants(const QVariantList &items)
{
    beginResetModel();
    m_variants = items;
    m_volatileVariants = items;
    m_added = QList<bool>(items.size(), false);
    m_removed = QList<bool>(items.size(), false);
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

void GroupedModel::setUnfilteredSectionTitle(const QString &title)
{
    m_displayModel->setUnfilteredSectionTitle(title);
}

void GroupedModel::addFilter(const QString &title, const Filter &filter)
{
    m_displayModel->addFilter(title, filter);
}

void GroupedModel::setExtraFilter(const Filter &filter)
{
    m_displayModel->setExtraFilter(filter);
}

const QVariantList &GroupedModel::variants() const
{
    return m_variants;
}

} // namespace Utils
