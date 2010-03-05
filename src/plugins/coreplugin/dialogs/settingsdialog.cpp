/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "settingsdialog.h"

#include <extensionsystem/pluginmanager.h>
#include "icore.h"

#include <utils/qtcassert.h>
#include <utils/filterlineedit.h>

#include <QtCore/QSettings>
#include <QtGui/QHeaderView>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStandardItem>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QItemSelectionModel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QIcon>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QToolBar>
#include <QtGui/QSpacerItem>
#include <QtGui/QStyle>
#include <QtGui/QStackedLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QFrame>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QTreeView>
#include <QtGui/QApplication>

enum ItemType { CategoryItem, PageItem };

enum { TypeRole = Qt::UserRole + 1,
       IndexRole = Qt::UserRole + 2,
       PageRole = Qt::UserRole + 3 };

Q_DECLARE_METATYPE(Core::IOptionsPage*)

static const char categoryKeyC[] = "General/LastPreferenceCategory";
static const char pageKeyC[] = "General/LastPreferencePage";

namespace Core {
namespace Internal {

// Create item on either model or parent item
template<class Parent>
    inline QStandardItem *createStandardItem(Parent *parent,
                                             const QString &text,
                                             ItemType type = CategoryItem,
                                             int index = -1,
                                             IOptionsPage *page = 0)
{
    QStandardItem *rc = new QStandardItem(text);
    rc->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    rc->setData(QVariant(int(type)), TypeRole);
    rc->setData(QVariant(index), IndexRole);
    rc->setData(qVariantFromValue(page), PageRole);
    parent->appendRow(rc);
    return rc;
}

static inline ItemType itemTypeOfItem(const QStandardItem *item)
{
    return static_cast<ItemType>(item->data(TypeRole).toInt());
}

static inline ItemType itemTypeOfItem(const QAbstractItemModel *model, const QModelIndex &index)
{
    return static_cast<ItemType>(model->data(index, TypeRole).toInt());
}

static inline int indexOfItem(const QStandardItem *item)
{
    return item->data(IndexRole).toInt();
}

static inline IOptionsPage *pageOfItem(const QStandardItem *item)
{
    return qvariant_cast<IOptionsPage *>(item->data(PageRole));
}

static inline IOptionsPage *pageOfItem(const QAbstractItemModel *model, const QModelIndex &index)
{
    return qvariant_cast<IOptionsPage *>(model->data(index, PageRole));
}

// A filter model that returns true for the parent (category) nodes
// (which by default do not match the search string and are thus collapsed)
// and additionally checks IOptionsPage::matches().
class PageFilterModel : public QSortFilterProxyModel {
public:
    explicit PageFilterModel(QObject *parent = 0) : QSortFilterProxyModel(parent) {}
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

bool PageFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!source_parent.isValid())
        return true; // Always true for parents/categories.
    // Regular contents check, then check page-filter.
    if (QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent))
        return true;
    if (const IOptionsPage *page = pageOfItem(sourceModel(), source_parent.child(source_row, 0))) {
        const QString pattern = filterRegExp().pattern();
        return page->displayCategory().contains(pattern, Qt::CaseInsensitive) ||
        page->matches(pattern);
    }
    return false;
}

// Populate a model with pages.
static QStandardItemModel *pageModel(const QList<IOptionsPage*> &pages,
                                     QObject *parent,
                                     const QString &initialCategory,
                                     const QString &initialPageId,
                                     QModelIndex *initialIndex)
{
    QStandardItemModel *model = new QStandardItemModel(0, 1, parent);
    const QChar hierarchySeparator = QLatin1Char('|');
    int index = 0;
    QMap<QString, QStandardItem *> categories;
    foreach (IOptionsPage *page, pages) {
        const QStringList categoriesId = page->category().split(hierarchySeparator);
        const QStringList displayCategories = page->displayCategory().split(hierarchySeparator);
        const int categoryDepth = categoriesId.size();
        if (categoryDepth != displayCategories.size()) {
            qWarning("Internal error: Hierarchy mismatch in settings page %s.", qPrintable(page->id()));
            continue;
        }

        // Retrieve/Create parent items for nested categories "Cat1|Cat2|Cat3"
        QString currentCategory = categoriesId.at(0);
        QStandardItem *treeItem = categories.value(currentCategory, 0);
        if (!treeItem) {
            treeItem = createStandardItem(model, displayCategories.at(0), CategoryItem);
            categories.insert(currentCategory, treeItem);
        }

        for (int cat = 1; cat < categoryDepth; cat++) {
            const QString fullCategory = currentCategory + hierarchySeparator + categoriesId.at(cat);
            treeItem = categories.value(fullCategory, 0);
            if (!treeItem) {
                QStandardItem *parentItem = categories.value(currentCategory);
                QTC_ASSERT(parentItem, return model)
                treeItem = createStandardItem(parentItem, displayCategories.at(cat), CategoryItem);
                categories.insert(fullCategory, treeItem);
            }
            currentCategory = fullCategory;
        }

        // Append page item
        QTC_ASSERT(treeItem, return model)
        QStandardItem *item = createStandardItem(treeItem, page->displayName(), PageItem, index, page);
        if (currentCategory == initialCategory && page->id() == initialPageId) {
            *initialIndex = model->indexFromItem(item);
        }
        index++;
    }
    return model;
}

// ----------- Pages tree view

/**
 * Special version of a QTreeView that has the width of the first column as
 * minimum size.
 */
class PageTree : public QTreeView
{
public:
    PageTree(QWidget *parent = 0) : QTreeView(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    }

    virtual QSize sizeHint() const
    {
        return QSize(sizeHintForColumn(0) + frameWidth() * 2, 100);
    }
};

// ----------- SettingsDialog

// Helpers to sort by category. id
bool optionsPageLessThan(const IOptionsPage *p1, const IOptionsPage *p2)
{
    if (const int cc = p1->category().compare(p2->category()))
        return cc < 0;
    return p1->id().compare(p2->id()) < 0;
}

static inline QList<Core::IOptionsPage*> sortedOptionsPages()
{
    QList<Core::IOptionsPage*> rc = ExtensionSystem::PluginManager::instance()->getObjects<IOptionsPage>();
    qStableSort(rc.begin(), rc.end(), optionsPageLessThan);
    return rc;
}

SettingsDialog::SettingsDialog(QWidget *parent, const QString &categoryId,
                               const QString &pageId) :
    QDialog(parent),
    m_pages(sortedOptionsPages()),
    m_proxyModel(new PageFilterModel),
    m_model(0),
    m_applied(false),
    m_stackedLayout(new QStackedLayout),
    m_filterLineEdit(new Utils::FilterLineEdit),
    m_pageTree(new PageTree),
    m_headerLabel(new QLabel)
{
    createGui();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
#ifdef Q_OS_MAC
    setWindowTitle(tr("Preferences"));
#else
    setWindowTitle(tr("Options"));
#endif
    QString initialCategory = categoryId;
    QString initialPage = pageId;
    if (initialCategory.isEmpty() && initialPage.isEmpty()) {
        QSettings *settings = ICore::instance()->settings();
        initialCategory = settings->value(QLatin1String(categoryKeyC), QVariant(QString())).toString();
        initialPage = settings->value(QLatin1String(pageKeyC), QVariant(QString())).toString();
    }

    // Create pages with title labels with a larger, bold font, left-aligned
    // with the group boxes of the page.
    foreach (IOptionsPage *page, m_pages)
        m_stackedLayout->addWidget(page->createPage(0));

    QModelIndex initialIndex;
    m_model = pageModel(m_pages, 0, initialCategory, initialPage, &initialIndex);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSourceModel(m_model);
    m_pageTree->setModel(m_proxyModel);
    m_pageTree->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_pageTree->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged(QModelIndex,QModelIndex)));
    if (initialIndex.isValid()) {
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(initialIndex);
        m_pageTree->selectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::ClearAndSelect);
    }

    // The order of the slot connection matters here, the filter slot
    // opens the matching page after the model has filtered.
    connect(m_filterLineEdit, SIGNAL(filterChanged(QString)),
                m_proxyModel, SLOT(setFilterFixedString(QString)));
    connect(m_filterLineEdit, SIGNAL(filterChanged(QString)), this, SLOT(filter(QString)));
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
    const int leftMargin = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) +
                           qApp->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_headerLabel);

    // Tree
    m_pageTree->header()->setVisible(false);
    m_stackedLayout->setMargin(0);

    // Separator Line
    QFrame *bottomLine = new QFrame;
    bottomLine->setFrameShape(QFrame::HLine);
    bottomLine->setFrameShadow(QFrame::Sunken);

    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(apply()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout *mainGridLayout = new QGridLayout;
    mainGridLayout->addWidget(m_filterLineEdit, 0, 0, 1, 1);
    mainGridLayout->addLayout(headerHLayout,    0, 1, 1, 1);
    mainGridLayout->addWidget(m_pageTree,       1, 0, 2, 1);
    mainGridLayout->addLayout(m_stackedLayout,  1, 1, 1, 1);
    mainGridLayout->addWidget(bottomLine,       2, 1, 1, 1);
    mainGridLayout->addWidget(buttonBox,        3, 0, 1, 2);
    mainGridLayout->setColumnStretch(0, 1);
    mainGridLayout->setColumnStretch(1, 4);
    setLayout(mainGridLayout);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::showPage(const QStandardItem *item)
{
    // Show a page item or recurse to the first page of a category
    // if a category was hit.
    switch (itemTypeOfItem(item)) {
    case PageItem: {
            IOptionsPage *page = pageOfItem(item);
            m_currentCategory = page->category();
            m_currentPage = page->id();
            m_stackedLayout->setCurrentIndex(indexOfItem(item));
            m_visitedPages.insert(page);
            m_headerLabel->setText(page->displayName());
        }
        break;
    case CategoryItem:
        if (const QStandardItem *child = item->child(0, 0))
            showPage(child);
        break;
    }
}

void SettingsDialog::currentChanged(const QModelIndex & current, const QModelIndex & /*previous */)
{
    if (current.isValid())
        if (const QStandardItem *item = m_model->itemFromIndex(m_proxyModel->mapToSource(current)))
            showPage(item);
}

// Helpers that recurse down the model to find a page.
static QModelIndex findPage(const QModelIndex &root)
{
    QTC_ASSERT(root.isValid(), return root)
    const QAbstractItemModel *model = root.model();
    // Found a page!
    if (itemTypeOfItem(model, root) == PageItem)
        return root;
    // Recurse down category.
    const int childCount = model->rowCount(root);
    for (int c = 0; c < childCount; c++) {
        const QModelIndex page = findPage(root.child(c, 0));
        if (page.isValid())
            return page;
    }
    return QModelIndex();
}

static QModelIndex findPage(const QAbstractItemModel *model)
{
    const QModelIndex invalid;
    // Traverse top categories
    const int rootItemCount = model->rowCount(invalid);
    for (int c = 0; c < rootItemCount; c++) {
        const QModelIndex page = findPage(model->index(c, 0, invalid));
        if (page.isValid())
            return page;
    }
    return invalid;
}

void SettingsDialog::filter(const QString &text)
{
    // Filter cleared, collapse all.
    if (text.isEmpty()) {
        m_pageTree->collapseAll();
        return;
    }
    // Expand match and select the first page. Note: This depends
    // on the order of slot invocation, needs to be done after filtering
    if (!m_proxyModel->rowCount(QModelIndex()))
        return;
    m_pageTree->expandAll();
    const QModelIndex firstVisiblePage = findPage(m_proxyModel);
    if (firstVisiblePage.isValid())
        m_pageTree->selectionModel()->setCurrentIndex(firstVisiblePage, QItemSelectionModel::ClearAndSelect);
}

void SettingsDialog::accept()
{
    m_applied = true;
    foreach (IOptionsPage *page, m_visitedPages)
        page->apply();
    foreach (IOptionsPage *page, m_pages)
        page->finish();
    done(QDialog::Accepted);
}

void SettingsDialog::reject()
{
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

bool SettingsDialog::execDialog()
{
    m_pageTree->setFocus();
    m_applied = false;
    exec();
    return m_applied;
}

void SettingsDialog::done(int val)
{
    QSettings *settings = ICore::instance()->settings();
    settings->setValue(QLatin1String(categoryKeyC), m_currentCategory);
    settings->setValue(QLatin1String(pageKeyC), m_currentPage);
    QDialog::done(val);
}

/**
 * Override to make sure the settings dialog starts up as small as possible.
 */
QSize SettingsDialog::sizeHint() const
{
    return minimumSize();
}

}
}
