/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingsdialog.h"

#include "icore.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/hostosinfo.h>
#include <utils/filterlineedit.h>

#include <QApplication>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPointer>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QStackedLayout>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QToolBar>
#include <QToolButton>

static const char categoryKeyC[] = "General/LastPreferenceCategory";
static const char pageKeyC[] = "General/LastPreferencePage";
const int categoryIconSize = 24;

namespace Core {
namespace Internal {

static QPointer<SettingsDialog> m_instance = 0;

// ----------- Category model

class Category
{
public:
    Id id;
    QString displayName;
    QIcon icon;
    QList<IOptionsPage *> pages;
    QList<IOptionsPageProvider *> providers;
    int index;
    QTabWidget *tabWidget;
};

class CategoryModel : public QAbstractListModel
{
public:
    CategoryModel(QObject *parent = 0);
    ~CategoryModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setPages(const QList<IOptionsPage*> &pages,
                  const QList<IOptionsPageProvider *> &providers);
    const QList<Category*> &categories() const { return m_categories; }

private:
    Category *findCategoryById(Id id);

    QList<Category*> m_categories;
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

    // Put the pages in categories
    foreach (IOptionsPage *page, pages) {
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

    endResetModel();
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
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

bool CategoryFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // Regular contents check, then check page-filter.
    if (QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
        return true;

    const CategoryModel *cm = static_cast<CategoryModel*>(sourceModel());
    foreach (const IOptionsPage *page, cm->categories().at(sourceRow)->pages) {
        const QString pattern = filterRegExp().pattern();
        if (page->displayCategory().contains(pattern, Qt::CaseInsensitive)
            || page->displayName().contains(pattern, Qt::CaseInsensitive)
            || page->matches(pattern))
            return true;
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
    }

    virtual QSize sizeHint() const
    {
        int width = sizeHintForColumn(0) + frameWidth() * 2 + 5;
        if (verticalScrollBar()->isVisible())
            width += verticalScrollBar()->width();
        return QSize(width, 100);
    }
};

// ----------- SettingsDialog

// Helpers to sort by category. id
bool optionsPageLessThan(const IOptionsPage *p1, const IOptionsPage *p2)
{
    if (p1->category() != p2->category())
        return p1->category().alphabeticallyBefore(p2->category());
    return p1->id().alphabeticallyBefore(p2->id());
}

static inline QList<Core::IOptionsPage*> sortedOptionsPages()
{
    QList<Core::IOptionsPage*> rc = ExtensionSystem::PluginManager::getObjects<IOptionsPage>();
    qStableSort(rc.begin(), rc.end(), optionsPageLessThan);
    return rc;
}

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_pages(sortedOptionsPages()),
    m_proxyModel(new CategoryFilterModel(this)),
    m_model(new CategoryModel(this)),
    m_stackedLayout(new QStackedLayout),
    m_filterLineEdit(new Utils::FilterLineEdit),
    m_categoryList(new CategoryListView),
    m_headerLabel(new QLabel),
    m_running(false),
    m_applied(false),
    m_finished(false)
{
    m_applied = false;

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

    connect(m_categoryList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged(QModelIndex)));

    // The order of the slot connection matters here, the filter slot
    // opens the matching page after the model has filtered.
    connect(m_filterLineEdit, SIGNAL(filterChanged(QString)),
            this, SLOT(ensureAllCategoryWidgets()));
    connect(m_filterLineEdit, SIGNAL(filterChanged(QString)),
                m_proxyModel, SLOT(setFilterFixedString(QString)));
    connect(m_filterLineEdit, SIGNAL(filterChanged(QString)), this, SLOT(filter(QString)));
    m_categoryList->setFocus();
}

void SettingsDialog::showPage(Id categoryId, Id pageId)
{
    // handle the case of "show last page"
    Id initialCategory = categoryId;
    Id initialPage = pageId;
    if (!initialCategory.isValid() && !initialPage.isValid()) {
        QSettings *settings = ICore::settings();
        initialCategory = Id::fromSetting(settings->value(QLatin1String(categoryKeyC)));
        initialPage = Id::fromSetting(settings->value(QLatin1String(pageKeyC)));
    }

    int initialCategoryIndex = -1;
    int initialPageIndex = -1;
    const QList<Category*> &categories = m_model->categories();
    for (int i = 0; i < categories.size(); ++i) {
        Category *category = categories.at(i);
        if (category->id == initialCategory) {
            initialCategoryIndex = i;
            for (int j = 0; j < category->pages.size(); ++j) {
                IOptionsPage *page = category->pages.at(j);
                if (page->id() == initialPage)
                    initialPageIndex = j;
            }
        }
    }
    if (initialCategoryIndex != -1) {
        const QModelIndex modelIndex = m_proxyModel->mapFromSource(m_model->index(initialCategoryIndex));
        m_categoryList->setCurrentIndex(modelIndex);
        if (initialPageIndex != -1)
            categories.at(initialCategoryIndex)->tabWidget->setCurrentIndex(initialPageIndex);
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

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Apply |
                                                       QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(apply()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout *mainGridLayout = new QGridLayout;
    mainGridLayout->addWidget(m_filterLineEdit, 0, 0, 1, 1);
    mainGridLayout->addLayout(headerHLayout,    0, 1, 1, 1);
    mainGridLayout->addWidget(m_categoryList,   1, 0, 1, 1);
    mainGridLayout->addLayout(m_stackedLayout,  1, 1, 1, 1);
    mainGridLayout->addWidget(buttonBox,        2, 0, 1, 2);
    mainGridLayout->setColumnStretch(1, 4);
    setLayout(mainGridLayout);
    setMinimumSize(1000, 550);
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
    foreach (const IOptionsPageProvider *provider, category->providers) {
        category->pages += provider->pages();
    }
    qStableSort(category->pages.begin(), category->pages.end(), optionsPageLessThan);

    QTabWidget *tabWidget = new QTabWidget;
    for (int j = 0; j < category->pages.size(); ++j) {
        IOptionsPage *page = category->pages.at(j);
        QWidget *widget = page->createPage(0);
        tabWidget->addTab(widget, page->displayName());
    }

    connect(tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(currentTabChanged(int)));

    category->tabWidget = tabWidget;
    category->index = m_stackedLayout->addWidget(tabWidget);
}

void SettingsDialog::ensureAllCategoryWidgets()
{
    foreach (Category *category, m_model->categories())
        ensureCategoryWidget(category);
}

void SettingsDialog::disconnectTabWidgets()
{
    foreach (Category *category, m_model->categories()) {
        if (category->tabWidget)
            disconnect(category->tabWidget, SIGNAL(currentChanged(int)),
                    this, SLOT(currentTabChanged(int)));
    }
}

void SettingsDialog::updateEnabledTabs(Category *category, const QString &searchText)
{
    for (int i = 0; i < category->pages.size(); ++i) {
        const IOptionsPage *page = category->pages.at(i);
        const bool enabled = searchText.isEmpty()
                             || page->category().toString().contains(searchText, Qt::CaseInsensitive)
                             || page->displayName().contains(searchText, Qt::CaseInsensitive)
                             || page->matches(searchText);
        category->tabWidget->setTabEnabled(i, enabled);
    }
}

void SettingsDialog::currentChanged(const QModelIndex &current)
{
    if (current.isValid())
        showCategory(m_proxyModel->mapToSource(current).row());
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
    ensureAllCategoryWidgets();
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
    settings->setValue(QLatin1String(categoryKeyC), m_currentCategory.toSetting());
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

SettingsDialog *SettingsDialog::getSettingsDialog(QWidget *parent,
    Id initialCategory, Id initialPage)
{
    if (!m_instance)
        m_instance = new SettingsDialog(parent);
    m_instance->showPage(initialCategory, initialPage);
    return m_instance;
}

bool SettingsDialog::execDialog()
{
    if (!m_running) {
        m_running = true;
        m_finished = false;
        exec();
        m_running = false;
        m_instance = 0;
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
