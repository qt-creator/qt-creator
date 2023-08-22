// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorsettingspage.h"

#include "directoryfilter.h"
#include "ilocatorfilter.h"
#include "locator.h"
#include "locatorconstants.h"
#include "urllocatorfilter.h"
#include "../coreconstants.h"
#include "../coreplugintr.h"

#include <utils/algorithm.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/fancylineedit.h>
#include <utils/headerviewstretcher.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QAbstractTextDocumentLayout>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QStyledItemDelegate>

using namespace Utils;

static const int SortRole = Qt::UserRole + 1;

namespace Core {
namespace Internal {

enum FilterItemColumn
{
    FilterName = 0,
    FilterPrefix,
    FilterIncludedByDefault
};

class FilterItem : public TreeItem
{
public:
    FilterItem(ILocatorFilter *filter);

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;
    bool setData(int column, const QVariant &data, int role) override;

    ILocatorFilter *filter() const;

private:
    ILocatorFilter *m_filter = nullptr;
};

class CategoryItem : public TreeItem
{
public:
    CategoryItem(const QString &name, int order);
    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override { Q_UNUSED(column) return Qt::ItemIsEnabled; }

private:
    QString m_name;
    int m_order = 0;
};

FilterItem::FilterItem(ILocatorFilter *filter)
    : m_filter(filter)
{
}

QVariant FilterItem::data(int column, int role) const
{
    switch (column) {
    case FilterName:
        if (role == SortRole)
            return m_filter->displayName();
        if (role == Qt::DisplayRole) {
            if (m_filter->description().isEmpty())
                return m_filter->displayName();
            return QString("<html>%1<br/><span style=\"font-weight: 70\">%2</span>")
                .arg(m_filter->displayName(), m_filter->description().toHtmlEscaped());
        }
        break;
    case FilterPrefix:
        if (role == Qt::DisplayRole || role == SortRole || role == Qt::EditRole)
            return m_filter->shortcutString();
        break;
    case FilterIncludedByDefault:
        if (role == Qt::CheckStateRole || role == SortRole)
            return m_filter->isIncludedByDefault() ? Qt::Checked : Qt::Unchecked;
        break;
    default:
        break;
    }

    if (role == Qt::ToolTipRole) {
        const QString description = m_filter->description();
        return description.isEmpty() ? QString() : ("<html>" + description.toHtmlEscaped());
    }
    return QVariant();
}

Qt::ItemFlags FilterItem::flags(int column) const
{
    if (column == FilterPrefix)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    if (column == FilterIncludedByDefault)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool FilterItem::setData(int column, const QVariant &data, int role)
{
    switch (column) {
    case FilterName:
        break;
    case FilterPrefix:
        if (role == Qt::EditRole && data.canConvert<QString>()) {
            m_filter->setShortcutString(data.toString());
            return true;
        }
        break;
    case FilterIncludedByDefault:
        if (role == Qt::CheckStateRole && data.canConvert<bool>()) {
            m_filter->setIncludedByDefault(data.toBool());
            return true;
        }
    }
    return false;
}

ILocatorFilter *FilterItem::filter() const
{
    return m_filter;
}

CategoryItem::CategoryItem(const QString &name, int order)
    : m_name(name), m_order(order)
{
}

QVariant CategoryItem::data(int column, int role) const
{
    Q_UNUSED(column)
    if (role == SortRole)
        return m_order;
    if (role == Qt::DisplayRole)
        return m_name;
    return QVariant();
}

class RichTextDelegate : public QStyledItemDelegate
{
public:
    RichTextDelegate(QObject *parent);
    ~RichTextDelegate();

    QTextDocument &doc() { return m_doc; }

    void setMaxWidth(int width);
    int maxWidth() const;

private:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    int m_maxWidth = -1;
    mutable QTextDocument m_doc;
};

RichTextDelegate::RichTextDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void RichTextDelegate::setMaxWidth(int width)
{
    m_maxWidth = width;
    emit sizeHintChanged({});
}

int RichTextDelegate::maxWidth() const
{
    return m_maxWidth;
}

RichTextDelegate::~RichTextDelegate() = default;

void RichTextDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    painter->save();
    QTextOption textOption;
    if (m_maxWidth > 0) {
        textOption.setWrapMode(QTextOption::WordWrap);
        m_doc.setDefaultTextOption(textOption);
        if (options.rect.width() > m_maxWidth)
            options.rect.setWidth(m_maxWidth);
    }
    m_doc.setHtml(options.text);
    m_doc.setTextWidth(options.rect.width());
    options.text = "";
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);
    painter->translate(options.rect.left(), options.rect.top());
    QRect clip(0, 0, options.rect.width(), options.rect.height());
    QAbstractTextDocumentLayout::PaintContext paintContext;
    paintContext.palette = options.palette;
    painter->setClipRect(clip);
    paintContext.clip = clip;
    if (qobject_cast<const QAbstractItemView *>(options.widget)->selectionModel()->isSelected(index)) {
        QAbstractTextDocumentLayout::Selection selection;
        selection.cursor = QTextCursor(&m_doc);
        selection.cursor.select(QTextCursor::Document);
        selection.format.setBackground(options.palette.brush(QPalette::Highlight));
        selection.format.setForeground(options.palette.brush(QPalette::HighlightedText));
        paintContext.selections << selection;
    }
    m_doc.documentLayout()->draw(painter, paintContext);
    painter->restore();
}

QSize RichTextDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);
    QTextOption textOption;
    if (m_maxWidth > 0) {
        textOption.setWrapMode(QTextOption::WordWrap);
        m_doc.setDefaultTextOption(textOption);
        if (!options.rect.isValid() || options.rect.width() > m_maxWidth)
            options.rect.setWidth(m_maxWidth);
    }
    m_doc.setHtml(options.text);
    m_doc.setTextWidth(options.rect.width());
    return QSize(m_doc.idealWidth(), m_doc.size().height());
}

class LocatorSettingsWidget : public IOptionsPageWidget
{
public:
    LocatorSettingsWidget()
    {
        m_plugin = Locator::instance();
        m_filters = Locator::filters();
        m_customFilters = m_plugin->customFilters();

        auto addButton = new QPushButton(Tr::tr("Add..."));

        auto refreshIntervalLabel = new QLabel(Tr::tr("Refresh interval:"));
        refreshIntervalLabel->setToolTip(
            Tr::tr("Locator filters that do not update their cached data immediately, such as the "
               "custom directory filters, update it after this time interval."));

        m_refreshInterval = new QSpinBox;
        m_refreshInterval->setToolTip(refreshIntervalLabel->toolTip());
        m_refreshInterval->setSuffix(Tr::tr(" min"));
        m_refreshInterval->setFrame(true);
        m_refreshInterval->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        m_refreshInterval->setMaximum(320);
        m_refreshInterval->setSingleStep(5);
        m_refreshInterval->setValue(60);

        auto filterEdit = new FancyLineEdit;
        filterEdit->setFiltering(true);

        m_filterList = new TreeView;
        m_filterList->setUniformRowHeights(false);
        m_filterList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_filterList->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_filterList->setSortingEnabled(true);
        m_filterList->setActivationMode(Utils::DoubleClickActivation);
        m_filterList->setAlternatingRowColors(true);
        auto nameDelegate = new RichTextDelegate(m_filterList);
        connect(m_filterList->header(),
                &QHeaderView::sectionResized,
                nameDelegate,
                [nameDelegate](int col, [[maybe_unused]] int old, int updated) {
                    if (col == 0)
                        nameDelegate->setMaxWidth(updated);
                });
        m_filterList->setItemDelegateForColumn(0, nameDelegate);

        m_model = new TreeModel<>(m_filterList);
        initializeModel();
        m_proxyModel = new CategorySortFilterModel(m_filterList);
        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setSortRole(SortRole);
        m_proxyModel->setFilterKeyColumn(-1/*all*/);
        m_filterList->setModel(m_proxyModel);
        m_filterList->expandAll();

        new HeaderViewStretcher(m_filterList->header(), FilterName);
        m_filterList->header()->setSortIndicator(FilterName, Qt::AscendingOrder);

        m_removeButton = new QPushButton(Tr::tr("Remove"));
        m_removeButton->setEnabled(false);

        m_editButton = new QPushButton(Tr::tr("Edit..."));
        m_editButton->setEnabled(false);

        using namespace Layouting;

        Column buttons{addButton, m_removeButton, m_editButton, st};

        Grid{filterEdit,
             br,
             m_filterList,
             buttons,
             br,
             Span(2, Row{refreshIntervalLabel, m_refreshInterval, st})}
            .attachTo(this);

        connect(filterEdit, &FancyLineEdit::filterChanged, this, &LocatorSettingsWidget::setFilter);
        connect(m_filterList->selectionModel(),
                &QItemSelectionModel::currentChanged,
                this,
                &LocatorSettingsWidget::updateButtonStates);
        connect(m_filterList,
                &Utils::TreeView::activated,
                this,
                &LocatorSettingsWidget::configureFilter);
        connect(m_editButton, &QPushButton::clicked, this, [this] {
            configureFilter(m_filterList->currentIndex());
        });
        connect(m_removeButton,
                &QPushButton::clicked,
                this,
                &LocatorSettingsWidget::removeCustomFilter);

        auto addMenu = new QMenu(addButton);
        addMenu->addAction(Tr::tr("Files in Directories"), this, [this] {
            addCustomFilter(new DirectoryFilter(Utils::Id(Constants::CUSTOM_DIRECTORY_FILTER_BASEID)
                                                    .withSuffix(m_customFilters.size() + 1)));
        });
        addMenu->addAction(Tr::tr("URL Template"), this, [this] {
            auto filter = new UrlLocatorFilter(
                Utils::Id(Constants::CUSTOM_URL_FILTER_BASEID)
                    .withSuffix(m_customFilters.size() + 1));
            filter->setIsCustomFilter(true);
            addCustomFilter(filter);
        });
        addButton->setMenu(addMenu);

        m_refreshInterval->setValue(m_plugin->refreshInterval());
        saveFilterStates();
    }

    void apply() final;
    void finish() final;

private:
    void updateButtonStates();
    void configureFilter(const QModelIndex &proxyIndex);
    void addCustomFilter(ILocatorFilter *filter);
    void removeCustomFilter();
    void initializeModel();
    void saveFilterStates();
    void restoreFilterStates();
    void requestRefresh();
    void setFilter(const QString &text);

    Utils::TreeView *m_filterList;
    QPushButton *m_removeButton;
    QPushButton *m_editButton;
    QSpinBox *m_refreshInterval;
    Locator *m_plugin = nullptr;
    Utils::TreeModel<> *m_model = nullptr;
    QSortFilterProxyModel *m_proxyModel = nullptr;
    Utils::TreeItem *m_customFilterRoot = nullptr;
    QList<ILocatorFilter *> m_filters;
    QList<ILocatorFilter *> m_addedFilters;
    QList<ILocatorFilter *> m_removedFilters;
    QList<ILocatorFilter *> m_customFilters;
    QList<ILocatorFilter *> m_refreshFilters;
    QHash<ILocatorFilter *, QByteArray> m_filterStates;
};

void LocatorSettingsWidget::apply()
{
    // Delete removed filters and clear added filters
    qDeleteAll(m_removedFilters);
    m_removedFilters.clear();
    m_addedFilters.clear();

    // Pass the new configuration on to the plugin
    m_plugin->setFilters(m_filters);
    m_plugin->setCustomFilters(m_customFilters);
    m_plugin->setRefreshInterval(m_refreshInterval->value());
    requestRefresh();
    m_plugin->saveSettings();
    saveFilterStates();
}

void LocatorSettingsWidget::finish()
{
    // If settings were applied, this shouldn't change anything. Otherwise it
    // makes sure the filter states aren't changed permanently.
    restoreFilterStates();

    // Delete added filters and clear removed filters
    qDeleteAll(m_addedFilters);
    m_addedFilters.clear();
    m_removedFilters.clear();

    // Further cleanup
    m_filters.clear();
    m_customFilters.clear();
    m_refreshFilters.clear();
}

void LocatorSettingsWidget::requestRefresh()
{
    if (!m_refreshFilters.isEmpty())
        m_plugin->refresh(m_refreshFilters);
}

void LocatorSettingsWidget::setFilter(const QString &text)
{
    m_proxyModel->setFilterRegularExpression(
        QRegularExpression(QRegularExpression::escape(text),
                           QRegularExpression::CaseInsensitiveOption));
    m_filterList->expandAll();
}

void LocatorSettingsWidget::saveFilterStates()
{
    m_filterStates.clear();
    for (ILocatorFilter *filter : std::as_const(m_filters))
        m_filterStates.insert(filter, filter->saveState());
}

void LocatorSettingsWidget::restoreFilterStates()
{
    const QList<ILocatorFilter *> filterStatesKeys = m_filterStates.keys();
    for (ILocatorFilter *filter : filterStatesKeys)
        filter->restoreState(m_filterStates.value(filter));
}

void LocatorSettingsWidget::initializeModel()
{
    m_model->setHeader({Tr::tr("Name"), Tr::tr("Prefix"), Tr::tr("Default")});
    m_model->setHeaderToolTip({
        QString(),
        ILocatorFilter::msgPrefixToolTip(),
        ILocatorFilter::msgIncludeByDefaultToolTip()
    });
    m_model->clear();
    QSet<ILocatorFilter *> customFilterSet = Utils::toSet(m_customFilters);
    auto builtIn = new CategoryItem(Tr::tr("Built-in"), 0/*order*/);
    for (ILocatorFilter *filter : std::as_const(m_filters))
        if (!filter->isHidden() && !customFilterSet.contains(filter))
            builtIn->appendChild(new FilterItem(filter));
    m_customFilterRoot = new CategoryItem(Tr::tr("Custom"), 1/*order*/);
    for (ILocatorFilter *customFilter : std::as_const(m_customFilters))
        m_customFilterRoot->appendChild(new FilterItem(customFilter));

    m_model->rootItem()->appendChild(builtIn);
    m_model->rootItem()->appendChild(m_customFilterRoot);
}

void LocatorSettingsWidget::updateButtonStates()
{
    const QModelIndex currentIndex = m_proxyModel->mapToSource(m_filterList->currentIndex());
    bool selected = currentIndex.isValid();
    ILocatorFilter *filter = nullptr;
    if (selected) {
        auto item = dynamic_cast<FilterItem *>(m_model->itemForIndex(currentIndex));
        if (item)
            filter = item->filter();
    }
    m_editButton->setEnabled(filter && filter->isConfigurable());
    m_removeButton->setEnabled(filter && m_customFilters.contains(filter));
}

void LocatorSettingsWidget::configureFilter(const QModelIndex &proxyIndex)
{
    const QModelIndex index = m_proxyModel->mapToSource(proxyIndex);
    QTC_ASSERT(index.isValid(), return);
    auto item = dynamic_cast<FilterItem *>(m_model->itemForIndex(index));
    QTC_ASSERT(item, return);
    ILocatorFilter *filter = item->filter();
    QTC_ASSERT(filter->isConfigurable(), return);
    bool includedByDefault = filter->isIncludedByDefault();
    QString shortcutString = filter->shortcutString();
    bool needsRefresh = false;
    filter->openConfigDialog(this, needsRefresh);
    if (needsRefresh && !m_refreshFilters.contains(filter))
        m_refreshFilters.append(filter);
    if (filter->isIncludedByDefault() != includedByDefault)
        item->updateColumn(FilterIncludedByDefault);
    if (filter->shortcutString() != shortcutString)
        item->updateColumn(FilterPrefix);
}

void LocatorSettingsWidget::addCustomFilter(ILocatorFilter *filter)
{
    bool needsRefresh = false;
    if (filter->openConfigDialog(this, needsRefresh)) {
        m_filters.append(filter);
        m_addedFilters.append(filter);
        m_customFilters.append(filter);
        m_refreshFilters.append(filter);
        m_customFilterRoot->appendChild(new FilterItem(filter));
    }
}

void LocatorSettingsWidget::removeCustomFilter()
{
    QModelIndex currentIndex = m_proxyModel->mapToSource(m_filterList->currentIndex());
    QTC_ASSERT(currentIndex.isValid(), return);
    auto item = dynamic_cast<FilterItem *>(m_model->itemForIndex(currentIndex));
    QTC_ASSERT(item, return);
    ILocatorFilter *filter = item->filter();
    QTC_ASSERT(m_customFilters.contains(filter), return);
    m_model->destroyItem(item);
    m_filters.removeAll(filter);
    m_customFilters.removeAll(filter);
    m_refreshFilters.removeAll(filter);
    if (m_addedFilters.contains(filter)) {
        m_addedFilters.removeAll(filter);
        delete filter;
    } else {
        m_removedFilters.append(filter);
    }
}

// LocatorSettingsPage

LocatorSettingsPage::LocatorSettingsPage()
{
    setId(Constants::FILTER_OPTIONS_PAGE);
    setDisplayName(Tr::tr(Constants::FILTER_OPTIONS_PAGE));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setWidgetCreator([] { return new LocatorSettingsWidget; });
}

} // Internal
} // Core
