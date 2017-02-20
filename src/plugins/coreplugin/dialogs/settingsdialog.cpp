/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include "settingsdialog.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListView>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QStackedLayout>
#include <QStyle>
#include <QStyledItemDelegate>

static const char pageKeyC[] = "General/LastPreferencePage";
const int categoryIconSize = 24;

namespace Core {
namespace Internal {

static QPointer<SettingsDialog> m_instance = 0;

bool optionsPageLessThan(const IOptionsPage *p1, const IOptionsPage *p2)
{
    if (p1->category() != p2->category())
        return p1->category().alphabeticallyBefore(p2->category());
    return p1->id().alphabeticallyBefore(p2->id());
}

static inline QList<IOptionsPage*> sortedOptionsPages()
{
    QList<IOptionsPage*> rc = ExtensionSystem::PluginManager::getObjects<IOptionsPage>();
    std::stable_sort(rc.begin(), rc.end(), optionsPageLessThan);
    return rc;
}

// ----------- Category model

class Category
{
public:
    Category() : index(-1), providerPagesCreated(false) { }

    bool findPageById(const Id id, int *pageIndex) const
    {
        *pageIndex = Utils::indexOf(pages, Utils::equal(&IOptionsPage::id, id));
        return *pageIndex != -1;
    }

    Id id;
    int index;
    QString displayName;
    QIcon icon;
    QList<IOptionsPage *> pages;
    QList<IOptionsPageProvider *> providers;
    bool providerPagesCreated;
    QTabWidget *tabWidget;
};

class CategoryModel : public QAbstractListModel
{
public:
    CategoryModel(QObject *parent = 0);
    ~CategoryModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setPages(const QList<IOptionsPage*> &pages,
                  const QList<IOptionsPageProvider *> &providers);
    void ensurePages(Category *category);
    const QList<Category*> &categories() const { return m_categories; }

private:
    Category *findCategoryById(Id id);

    QList<Category*> m_categories;
    QSet<Id> m_pageIds;
    QIcon m_emptyIcon;
};

CategoryModel::CategoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QPixmap empty(categoryIconSize, categoryIconSize);
    empty.fill(Qt::transparent);
    m_emptyIcon = QIcon(empty);
}

CategoryModel::~CategoryModel()
{
    qDeleteAll(m_categories);
}

int CategoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_categories.size();
}

QVariant CategoryModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        return m_categories.at(index.row())->displayName;
    case Qt::DecorationRole: {
            QIcon icon = m_categories.at(index.row())->icon;
            if (icon.isNull())
                icon = m_emptyIcon;
            return icon;
        }
    }

    return QVariant();
}

void CategoryModel::setPages(const QList<IOptionsPage*> &pages,
                             const QList<IOptionsPageProvider *> &providers)
{
    beginResetModel();

    // Clear any previous categories
    qDeleteAll(m_categories);
    m_categories.clear();
    m_pageIds.clear();

    // Put the pages in categories
    foreach (IOptionsPage *page, pages) {
        QTC_ASSERT(!m_pageIds.contains(page->id()),
                   qWarning("duplicate options page id '%s'", qPrintable(page->id().toString())));
        m_pageIds.insert(page->id());
        const Id categoryId = page->category();
        Category *category = findCategoryById(categoryId);
        if (!category) {
            category = new Category;
            category->id = categoryId;
            category->tabWidget = 0;
            category->index = -1;
            m_categories.append(category);
        }
        if (category->displayName.isEmpty())
            category->displayName = page->displayCategory();
        if (category->icon.isNull())
            category->icon = page->categoryIcon();
        category->pages.append(page);
    }

    foreach (IOptionsPageProvider *provider, providers) {
        const Id categoryId = provider->category();
        Category *category = findCategoryById(categoryId);
        if (!category) {
            category = new Category;
            category->id = categoryId;
            category->tabWidget = 0;
            category->index = -1;
            m_categories.append(category);
        }
        if (category->displayName.isEmpty())
            category->displayName = provider->displayCategory();
        if (category->icon.isNull())
            category->icon = provider->categoryIcon();
        category->providers.append(provider);
    }

    Utils::sort(m_categories, [](const Category *c1, const Category *c2) {
       return c1->id.alphabeticallyBefore(c2->id);
    });
    endResetModel();
}

void CategoryModel::ensurePages(Category *category)
{
    if (!category->providerPagesCreated) {
        QList<IOptionsPage *> createdPages;
        foreach (const IOptionsPageProvider *provider, category->providers)
            createdPages += provider->pages();

        // check for duplicate ids
        foreach (IOptionsPage *page, createdPages) {
            QTC_ASSERT(!m_pageIds.contains(page->id()),
                       qWarning("duplicate options page id '%s'", qPrintable(page->id().toString())));
        }

        category->pages += createdPages;
        category->providerPagesCreated = true;
        std::stable_sort(category->pages.begin(), category->pages.end(), optionsPageLessThan);
    }
}

Category *CategoryModel::findCategoryById(Id id)
{
    for (int i = 0; i < m_categories.size(); ++i) {
        Category *category = m_categories.at(i);
        if (category->id == id)
            return category;
    }

    return 0;
}

// ----------- Category filter model

/**
 * A filter model that returns true for each category node that has pages that
 * match the search string.
 */
class CategoryFilterModel : public QSortFilterProxyModel
{
public:
    explicit CategoryFilterModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {}

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

bool CategoryFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // Regular contents check, then check page-filter.
    if (QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
        return true;

    const QString pattern = filterRegExp().pattern();
    const CategoryModel *cm = static_cast<CategoryModel*>(sourceModel());
    const Category *category = cm->categories().at(sourceRow);
    foreach (const IOptionsPage *page, category->pages) {
        if (page->displayCategory().contains(pattern, Qt::CaseInsensitive)
                || page->displayName().contains(pattern, Qt::CaseInsensitive)
                || page->matches(pattern))
            return true;
    }

    if (!category->providerPagesCreated) {
        foreach (const IOptionsPageProvider *provider, category->providers) {
            if (provider->matches(pattern))
                return true;
        }
    }

    return false;
}

// ----------- Category list view


class CategoryListViewDelegate : public QStyledItemDelegate
{
public:
    CategoryListViewDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), 32));
        return size;
    }
};

/**
 * Special version of a QListView that has the width of the first column as
 * minimum size.
 */
class CategoryListView : public QListView
{
public:
    CategoryListView(QWidget *parent = 0) : QListView(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
        setItemDelegate(new CategoryListViewDelegate(this));
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    virtual QSize sizeHint() const
    {
        int width = sizeHintForColumn(0) + frameWidth() * 2 + 5;
        width += verticalScrollBar()->width();
        return QSize(width, 100);
    }

    // QListView installs a event filter on its scrollbars
    virtual bool eventFilter(QObject *obj, QEvent *event)
    {
        if (obj == verticalScrollBar()
                && (event->type() == QEvent::Show
                    || event->type() == QEvent::Hide))
            updateGeometry();
        return QListView::eventFilter(obj, event);
    }
};

// ----------- SmartScrollArea

class SmartScrollArea : public QScrollArea
{
public:
    SmartScrollArea(QWidget *parent = 0)
        : QScrollArea(parent)
    {
        setFrameStyle(QFrame::NoFrame | QFrame::Plain);
        viewport()->setAutoFillBackground(false);
        setWidgetResizable(true);
    }
private:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget *inner = widget();
        if (inner) {
            int fw = frameWidth() * 2;
            QSize innerSize = event->size() - QSize(fw, fw);
            QSize innerSizeHint = inner->minimumSizeHint();

            if (innerSizeHint.height() > innerSize.height()) { // Widget wants to be bigger than available space
                innerSize.setWidth(innerSize.width() - scrollBarWidth());
                innerSize.setHeight(innerSizeHint.height());
            }
            inner->resize(innerSize);
        }
        QScrollArea::resizeEvent(event);
    }

    QSize minimumSizeHint() const override {
        QWidget *inner = widget();
        if (inner) {
            int fw = frameWidth() * 2;

            QSize minSize = inner->minimumSizeHint();
            minSize += QSize(fw, fw);
            minSize += QSize(scrollBarWidth(), 0);
            minSize.setHeight(qMin(minSize.height(), 450));
            minSize.setWidth(qMin(minSize.width(), 810));
            return minSize;
        }
        return QSize(0, 0);
    }

    bool event(QEvent *event) override {
        if (event->type() == QEvent::LayoutRequest)
            updateGeometry();
        return QScrollArea::event(event);
    }

    int scrollBarWidth() const
    {
        auto that = const_cast<SmartScrollArea *>(this);
        QWidgetList list = that->scrollBarWidgets(Qt::AlignRight);
        if (list.isEmpty())
            return 0;
        return list.first()->sizeHint().width();
    }
};


// ----------- SettingsDialog

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_pages(sortedOptionsPages()),
    m_proxyModel(new CategoryFilterModel(this)),
    m_model(new CategoryModel(this)),
    m_stackedLayout(new QStackedLayout),
    m_filterLineEdit(new Utils::FancyLineEdit),
    m_categoryList(new CategoryListView),
    m_headerLabel(new QLabel),
    m_running(false),
    m_applied(false),
    m_finished(false)
{
    m_applied = false;
    m_filterLineEdit->setFiltering(true);

    createGui();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    if (Utils::HostOsInfo::isMacHost())
        setWindowTitle(tr("Preferences"));
    else
        setWindowTitle(tr("Options"));

    m_model->setPages(m_pages,
        ExtensionSystem::PluginManager::getObjects<IOptionsPageProvider>());

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_categoryList->setIconSize(QSize(categoryIconSize, categoryIconSize));
    m_categoryList->setModel(m_proxyModel);
    m_categoryList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_categoryList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(m_categoryList->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &SettingsDialog::currentChanged);

    // The order of the slot connection matters here, the filter slot
    // opens the matching page after the model has filtered.
    connect(m_filterLineEdit, &Utils::FancyLineEdit::filterChanged,
            m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_filterLineEdit, &Utils::FancyLineEdit::filterChanged,
            this, &SettingsDialog::filter);
    m_categoryList->setFocus();
}

void SettingsDialog::showPage(const Id pageId)
{
    // handle the case of "show last page"
    Id initialPageId = pageId;
    if (!initialPageId.isValid()) {
        QSettings *settings = ICore::settings();
        initialPageId = Id::fromSetting(settings->value(QLatin1String(pageKeyC)));
    }

    int initialCategoryIndex = -1;
    int initialPageIndex = -1;

    const QList<Category*> &categories = m_model->categories();
    if (initialPageId.isValid()) {
        // First try categories without lazy items.
        for (int i = 0; i < categories.size(); ++i) {
            Category *category = categories.at(i);
            if (category->providers.isEmpty()) {  // no providers
                if (category->findPageById(initialPageId, &initialPageIndex)) {
                    initialCategoryIndex = i;
                    break;
                }
            }
        }

        if (initialPageIndex == -1) {
            // On failure, expand the remaining items.
            for (int i = 0; i < categories.size(); ++i) {
                Category *category = categories.at(i);
                if (!category->providers.isEmpty()) { // has providers
                    ensureCategoryWidget(category);
                    if (category->findPageById(initialPageId, &initialPageIndex)) {
                        initialCategoryIndex = i;
                        break;
                    }
                }
            }
        }
    }

    if (initialPageId.isValid() && initialPageIndex == -1)
        return; // Unknown settings page, probably due to missing plugin.

    if (initialCategoryIndex != -1) {
        QModelIndex modelIndex = m_proxyModel->mapFromSource(m_model->index(initialCategoryIndex));
        if (!modelIndex.isValid()) { // filtered out, so clear filter first
            m_filterLineEdit->setText(QString());
            modelIndex = m_proxyModel->mapFromSource(m_model->index(initialCategoryIndex));
        }
        m_categoryList->setCurrentIndex(modelIndex);
        if (initialPageIndex != -1) {
            if (QTC_GUARD(categories.at(initialCategoryIndex)->tabWidget))
                categories.at(initialCategoryIndex)->tabWidget->setCurrentIndex(initialPageIndex);
        }
    }
}

void SettingsDialog::createGui()
{
    // Header label with large font and a bit of spacing (align with group boxes)
    QFont headerLabelFont = m_headerLabel->font();
    headerLabelFont.setBold(true);
    // Paranoia: Should a font be set in pixels...
    const int pointSize = headerLabelFont.pointSize();
    if (pointSize > 0)
        headerLabelFont.setPointSize(pointSize + 2);
    m_headerLabel->setFont(headerLabelFont);

    QHBoxLayout *headerHLayout = new QHBoxLayout;
    const int leftMargin = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_headerLabel);

    m_stackedLayout->setMargin(0);
    m_stackedLayout->addWidget(new QWidget(this)); // no category selected, for example when filtering

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Apply |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked,
            this, &SettingsDialog::apply);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

    QGridLayout *mainGridLayout = new QGridLayout;
    mainGridLayout->addWidget(m_filterLineEdit, 0, 0, 1, 1);
    mainGridLayout->addLayout(headerHLayout,    0, 1, 1, 1);
    mainGridLayout->addWidget(m_categoryList,   1, 0, 1, 1);
    mainGridLayout->addLayout(m_stackedLayout,  1, 1, 1, 1);
    mainGridLayout->addWidget(buttonBox,        2, 0, 1, 2);
    mainGridLayout->setColumnStretch(1, 4);
    setLayout(mainGridLayout);

    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    mainGridLayout->setSizeConstraint(QLayout::SetMinimumSize);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::showCategory(int index)
{
    Category *category = m_model->categories().at(index);
    ensureCategoryWidget(category);
    // Update current category and page
    m_currentCategory = category->id;
    const int currentTabIndex = category->tabWidget->currentIndex();
    if (currentTabIndex != -1) {
        IOptionsPage *page = category->pages.at(currentTabIndex);
        m_currentPage = page->id();
        m_visitedPages.insert(page);
    }

    m_stackedLayout->setCurrentIndex(category->index);
    m_headerLabel->setText(category->displayName);

    updateEnabledTabs(category, m_filterLineEdit->text());
}

void SettingsDialog::ensureCategoryWidget(Category *category)
{
    if (category->tabWidget != 0)
        return;

    m_model->ensurePages(category);
    QTabWidget *tabWidget = new QTabWidget;
    for (int j = 0; j < category->pages.size(); ++j) {
        IOptionsPage *page = category->pages.at(j);
        QWidget *widget = page->widget();
        SmartScrollArea *ssa = new SmartScrollArea(this);
        ssa->setWidget(widget);
        widget->setAutoFillBackground(false);
        tabWidget->addTab(ssa, page->displayName());
    }

    connect(tabWidget, &QTabWidget::currentChanged,
            this, &SettingsDialog::currentTabChanged);

    category->tabWidget = tabWidget;
    category->index = m_stackedLayout->addWidget(tabWidget);
}

void SettingsDialog::disconnectTabWidgets()
{
    foreach (Category *category, m_model->categories()) {
        if (category->tabWidget)
            disconnect(category->tabWidget, &QTabWidget::currentChanged,
                       this, &SettingsDialog::currentTabChanged);
    }
}

void SettingsDialog::updateEnabledTabs(Category *category, const QString &searchText)
{
    int firstEnabledTab = -1;
    for (int i = 0; i < category->pages.size(); ++i) {
        const IOptionsPage *page = category->pages.at(i);
        const bool enabled = searchText.isEmpty()
                             || page->category().toString().contains(searchText, Qt::CaseInsensitive)
                             || page->displayName().contains(searchText, Qt::CaseInsensitive)
                             || page->matches(searchText);
        category->tabWidget->setTabEnabled(i, enabled);
        if (enabled && firstEnabledTab < 0)
            firstEnabledTab = i;
    }
    if (!category->tabWidget->isTabEnabled(category->tabWidget->currentIndex())
            && firstEnabledTab != -1) {
        // QTabWidget is dumb, so this can happen
        category->tabWidget->setCurrentIndex(firstEnabledTab);
    }
}

void SettingsDialog::currentChanged(const QModelIndex &current)
{
    if (current.isValid()) {
        showCategory(m_proxyModel->mapToSource(current).row());
    } else {
        m_stackedLayout->setCurrentIndex(0);
        m_headerLabel->clear();
    }
}

void SettingsDialog::currentTabChanged(int index)
{
    if (index == -1)
        return;

    const QModelIndex modelIndex = m_proxyModel->mapToSource(m_categoryList->currentIndex());
    if (!modelIndex.isValid())
        return;

    // Remember the current tab and mark it as visited
    const Category *category = m_model->categories().at(modelIndex.row());
    IOptionsPage *page = category->pages.at(index);
    m_currentPage = page->id();
    m_visitedPages.insert(page);
}

void SettingsDialog::filter(const QString &text)
{
    // When there is no current index, select the first one when possible
    if (!m_categoryList->currentIndex().isValid() && m_model->rowCount() > 0)
        m_categoryList->setCurrentIndex(m_proxyModel->index(0, 0));

    const QModelIndex currentIndex = m_proxyModel->mapToSource(m_categoryList->currentIndex());
    if (!currentIndex.isValid())
        return;

    Category *category = m_model->categories().at(currentIndex.row());
    updateEnabledTabs(category, text);
}

void SettingsDialog::accept()
{
    if (m_finished)
        return;
    m_finished = true;
    disconnectTabWidgets();
    m_applied = true;
    foreach (IOptionsPage *page, m_visitedPages)
        page->apply();
    foreach (IOptionsPage *page, m_pages)
        page->finish();
    done(QDialog::Accepted);
}

void SettingsDialog::reject()
{
    if (m_finished)
        return;
    m_finished = true;
    disconnectTabWidgets();
    foreach (IOptionsPage *page, m_pages)
        page->finish();
    done(QDialog::Rejected);
}

void SettingsDialog::apply()
{
    foreach (IOptionsPage *page, m_visitedPages)
        page->apply();
    m_applied = true;
}

void SettingsDialog::done(int val)
{
    QSettings *settings = ICore::settings();
    settings->setValue(QLatin1String(pageKeyC), m_currentPage.toSetting());

    ICore::saveSettings(); // save all settings

    // exit all additional event loops, see comment in execDialog()
    QListIterator<QEventLoop *> it(m_eventLoops);
    it.toBack();
    while (it.hasPrevious()) {
        QEventLoop *loop = it.previous();
        loop->exit();
    }

    QDialog::done(val);
}

/**
 * Override to make sure the settings dialog starts up as small as possible.
 */
QSize SettingsDialog::sizeHint() const
{
    return minimumSize();
}

SettingsDialog *SettingsDialog::getSettingsDialog(QWidget *parent, Id initialPage)
{
    if (!m_instance)
        m_instance = new SettingsDialog(parent);
    m_instance->showPage(initialPage);
    return m_instance;
}

bool SettingsDialog::execDialog()
{
    if (!m_running) {
        m_running = true;
        m_finished = false;
        static const QLatin1String kPreferenceDialogSize("Core/PreferenceDialogSize");
        if (ICore::settings()->contains(kPreferenceDialogSize))
            resize(ICore::settings()->value(kPreferenceDialogSize).toSize());
        exec();
        m_running = false;
        m_instance = 0;
        ICore::settings()->setValue(kPreferenceDialogSize, size());
        // make sure that the current "single" instance is deleted
        // we can't delete right away, since we still access the m_applied member
        deleteLater();
    } else {
        // exec dialog is called while the instance is already running
        // this can happen when a event triggers a code path that wants to
        // show the settings dialog again
        // e.g. when starting the debugger (with non-built debugging helpers),
        // and manually opening the settings dialog, after the debugger hit
        // a break point it will complain about missing helper, and offer the
        // option to open the settings dialog.
        // Keep the UI running by creating another event loop.
        QEventLoop *loop = new QEventLoop(this);
        m_eventLoops.append(loop);
        loop->exec();
    }
    return m_applied;
}

} // namespace Internal
} // namespace Core
