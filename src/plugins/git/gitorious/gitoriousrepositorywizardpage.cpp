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

#include "gitoriousrepositorywizardpage.h"
#include "gitoriousprojectwizardpage.h"
#include "gitoriousprojectwidget.h"
#include "gitorious.h"
#include "ui_gitoriousrepositorywizardpage.h"

#include <utils/qtcassert.h>

#include <QDebug>

#include <QStandardItemModel>
#include <QStandardItem>
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>

enum { TypeRole = Qt::UserRole + 1};
enum { HeaderType, RepositoryType };

enum { debug = 0 };

namespace Gitorious {
namespace Internal {

// A filter model that returns true for the parent (category) nodes
// (which by default do not match the search string and are thus collapsed).
class RepositoryFilterModel : public QSortFilterProxyModel {
public:
    explicit RepositoryFilterModel(QObject *parent = 0) : QSortFilterProxyModel(parent) {}
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

bool RepositoryFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!source_parent.isValid())
        return true; // Always true for parents.
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

// ----------- GitoriousRepositoryWizardPage
enum { RepositoryColumn, OwnerColumn, DescriptionColumn, ColumnCount };

GitoriousRepositoryWizardPage::GitoriousRepositoryWizardPage(const GitoriousProjectWizardPage *projectPage,
                                                             QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::GitoriousRepositoryWizardPage),
    m_projectPage(projectPage),
    m_model(new QStandardItemModel(0, ColumnCount)),
    m_filterModel(new RepositoryFilterModel),
    m_valid(false)
{
    QStringList headers;
    headers << tr("Name") << tr("Owner") << tr("Description");
    m_model->setHorizontalHeaderLabels(headers);
    // Filter on all columns
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterKeyColumn(-1);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    ui->setupUi(this);
    // Filter
    connect(ui->filterLineEdit, SIGNAL(filterChanged(QString)), m_filterModel, SLOT(setFilterFixedString(QString)));
    // Tree view
    ui->repositoryTreeView->setModel(m_filterModel);
    ui->repositoryTreeView->setUniformRowHeights(true);
    ui->repositoryTreeView->setAlternatingRowColors(true);
    ui->repositoryTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui->repositoryTreeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));

    setTitle(tr("Repository"));
}

GitoriousRepositoryWizardPage::~GitoriousRepositoryWizardPage()
{
    delete ui;
}

bool gitRepoLessThanByType(const GitoriousRepository &r1, const GitoriousRepository &r2)
{
    return r1.type < r2.type;
}

static inline QList<QStandardItem *> headerEntry(const QString &h)
{
    QStandardItem *nameItem = new QStandardItem(h);
    nameItem->setFlags(Qt::ItemIsEnabled);
    nameItem->setData(QVariant(HeaderType), TypeRole);
    QStandardItem *ownerItem = new QStandardItem;
    ownerItem->setFlags(Qt::ItemIsEnabled);
    ownerItem->setData(QVariant(HeaderType), TypeRole);
    QStandardItem *descriptionItem = new QStandardItem;
    descriptionItem->setFlags(Qt::ItemIsEnabled);
    descriptionItem->setData(QVariant(HeaderType), TypeRole);
    QList<QStandardItem *> rc;
    rc << nameItem << ownerItem << descriptionItem;
    return rc;
}

static inline QList<QStandardItem *> repositoryEntry(const GitoriousRepository &r)
{
    QStandardItem *nameItem = new QStandardItem(r.name);
    nameItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    nameItem->setData(QVariant(RepositoryType), TypeRole);
    QStandardItem *ownerItem = new QStandardItem(r.owner);
    ownerItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ownerItem->setData(QVariant(RepositoryType), TypeRole);
    QStandardItem *descriptionItem = new QStandardItem;
    descriptionItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    descriptionItem->setData(QVariant(RepositoryType), TypeRole);
    QList<QStandardItem *> rc;
    rc << nameItem << ownerItem << descriptionItem;
    GitoriousProjectWidget::setDescription(r.description, DescriptionColumn, &rc);
    return rc;
}

void GitoriousRepositoryWizardPage::initializePage()
{
    // Populate the model
    ui->repositoryTreeView->selectionModel()->clearSelection();
    if (const int oldRowCount = m_model->rowCount())
        m_model->removeRows(0, oldRowCount);
    ui->filterLineEdit->clear();
    // fill model
    const QSharedPointer<GitoriousProject> proj = m_projectPage->project();
    setSubTitle(tr("Choose a repository of the project '%1'.").arg(proj->name));
    // Create a hierarchical list by repository type, sort by type
    QList<GitoriousRepository> repositories = proj->repositories;
    QStandardItem *firstEntry = 0;
    if (!repositories.empty()) {
        int lastRepoType = -1;
        QStandardItem *header = 0;
        qStableSort(repositories.begin(), repositories.end(), gitRepoLessThanByType);
        const QString types[GitoriousRepository::PersonalRepository + 1] =
            { tr("Mainline Repositories"), tr("Clones"), tr("Baseline Repositories"), tr("Shared Project Repositories"), tr("Personal Repositories") };
        foreach (const GitoriousRepository &r, repositories) {
            // New Header?
            if (r.type != lastRepoType || !header) {
                lastRepoType = r.type;
                const QList<QStandardItem *> headerRow = headerEntry(types[r.type]);
                m_model->appendRow(headerRow);
                header = headerRow.front();
            }
            // Repository row
            const QList<QStandardItem *> row = repositoryEntry(r);
            header->appendRow(row);
            if (!firstEntry)
                firstEntry = row.front();
        }
    }
    ui->repositoryTreeView->expandAll();
    for (int r = 0; r < ColumnCount; r++)
        ui->repositoryTreeView->resizeColumnToContents(r);
    // Select first
    if (firstEntry) {
        const QModelIndex filterIndex = m_filterModel->mapFromSource(m_model->indexFromItem(firstEntry));
        ui->repositoryTreeView->selectionModel()->setCurrentIndex(filterIndex, QItemSelectionModel::Select|QItemSelectionModel::Current|QItemSelectionModel::Rows);
    }
    ui->repositoryTreeView->setFocus();
}

QStandardItem *GitoriousRepositoryWizardPage::currentItem0() const
{
    return item0FromIndex(ui->repositoryTreeView->selectionModel()->currentIndex());
}

QStandardItem *GitoriousRepositoryWizardPage::item0FromIndex(const QModelIndex &filterIndex) const
{
    if (filterIndex.isValid()) {
        const QModelIndex sourceIndex = m_filterModel->mapToSource(filterIndex);
        if (sourceIndex.column() == 0)
            return m_model->itemFromIndex(sourceIndex);
        const QModelIndex sibling0 = sourceIndex.sibling(sourceIndex.row(), 0);
        return m_model->itemFromIndex(sibling0);
    }
    return 0;
}

void GitoriousRepositoryWizardPage::slotCurrentChanged(const QModelIndex &current, const QModelIndex & /*previous */)
{
    const QStandardItem *item = item0FromIndex(current);
    const bool isValid = item && item->data(TypeRole).toInt() == RepositoryType;
    if (isValid != m_valid) {
        m_valid = isValid;
        emit completeChanged();
    }
}

QString GitoriousRepositoryWizardPage::repositoryName() const
{
    if (const QStandardItem *item = currentItem0())
        if (item->data(TypeRole).toInt() == RepositoryType)
            return item->text();
    return QString();
}

QUrl GitoriousRepositoryWizardPage::repositoryURL() const
{
    // Find by name (as we sorted the the repositories)
    const QString repoName = repositoryName();
    foreach (const GitoriousRepository &r, m_projectPage->project()->repositories)
        if (r.name == repoName)
            return r.cloneUrl;
    return QUrl();
}

bool GitoriousRepositoryWizardPage::isComplete() const
{
    return m_valid;
}

void GitoriousRepositoryWizardPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace Internal
} // namespace Gitorious
