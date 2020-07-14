/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "addrunconfigdialog.h"

#include "project.h"
#include "target.h"

#include <utils/itemviews.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

const Qt::ItemDataRole IsCustomRole = Qt::UserRole;

class CandidateTreeItem : public TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::AddRunConfigDialog)
public:
    CandidateTreeItem(const RunConfigurationCreationInfo &rci, const FilePath &projectRoot)
        : m_creationInfo(rci), m_projectRoot(projectRoot)
    { }

    RunConfigurationCreationInfo creationInfo() const { return m_creationInfo; }

private:
    QVariant data(int column, int role) const override
    {
        QTC_ASSERT(column < 2, return QVariant());
        if (role == IsCustomRole)
            return m_creationInfo.projectFilePath.isEmpty();
        if (column == 0 && role == Qt::DisplayRole)
            return m_creationInfo.displayName;
        if (column == 1 && role == Qt::DisplayRole) {
            FilePath displayPath = m_creationInfo.projectFilePath.relativeChildPath(m_projectRoot);
            if (displayPath.isEmpty()) {
                displayPath = m_creationInfo.projectFilePath;
                QTC_CHECK(displayPath.isEmpty());
            }
            return displayPath.isEmpty() ? tr("[none]") : displayPath.toUserOutput();
        }
        return QVariant();
    }

    const RunConfigurationCreationInfo m_creationInfo;
    const FilePath m_projectRoot;
};

class CandidatesModel : public TreeModel<TreeItem, CandidateTreeItem>
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::AddRunConfigDialog)
public:
    CandidatesModel(Target *target, QObject *parent) : TreeModel(parent)
    {
        setHeader({tr("Name"), tr("Source")});
        for (const RunConfigurationCreationInfo &rci
             : RunConfigurationFactory::creatorsForTarget(target)) {
            rootItem()->appendChild(new CandidateTreeItem(rci,
                                                          target->project()->projectDirectory()));
        }
    }
};

class ProxyModel : public QSortFilterProxyModel
{
public:
    ProxyModel(QObject *parent) : QSortFilterProxyModel(parent) { }

private:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override
    {
        if (source_left.column() == 0) {
            // Let's put the fallback candidates last.
            const bool leftIsCustom = sourceModel()->data(source_left, IsCustomRole).toBool();
            const bool rightIsCustom = sourceModel()->data(source_right, IsCustomRole).toBool();
            if (leftIsCustom != rightIsCustom)
                return rightIsCustom;
        }
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }
};

class CandidatesTreeView : public TreeView
{
public:
    CandidatesTreeView(QWidget *parent) : TreeView(parent)
    {
        setUniformRowHeights(true);
    }

private:
    QSize sizeHint() const override
    {
        const int width = columnWidth(0) + columnWidth(1);
        const int height = qMin(model()->rowCount() + 10, 10) * rowHeight(model()->index(0, 0))
                + header()->sizeHint().height();
        return {width, height};
    }
};

AddRunConfigDialog::AddRunConfigDialog(Target *target, QWidget *parent)
    : QDialog(parent), m_view(new CandidatesTreeView(this))
{
    setWindowTitle(tr("Create Run Configuration"));
    const auto model = new CandidatesModel(target, this);
    const auto proxyModel = new ProxyModel(this);
    proxyModel->setSourceModel(model);
    const auto filterEdit = new QLineEdit(this);
    m_view->setSelectionMode(TreeView::SingleSelection);
    m_view->setSelectionBehavior(TreeView::SelectRows);
    m_view->setSortingEnabled(true);
    m_view->setModel(proxyModel);
    m_view->resizeColumnToContents(0);
    m_view->resizeColumnToContents(1);
    m_view->sortByColumn(0, Qt::AscendingOrder);
    const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create"));

    connect(filterEdit, &QLineEdit::textChanged, this, [proxyModel](const QString &text) {
        proxyModel->setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
    });
    connect(m_view, &TreeView::doubleClicked, this, [this] { accept(); });
    const auto updateOkButton = [buttonBox, this] {
        buttonBox->button(QDialogButtonBox::Ok)
                ->setEnabled(m_view->selectionModel()->hasSelection());
    };
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, updateOkButton);
    updateOkButton();
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const auto layout = new QVBoxLayout(this);
    const auto filterLayout = new QHBoxLayout;
    filterLayout->addWidget(new QLabel(tr("Filter candidates by name:"), this));
    filterLayout->addWidget(filterEdit);
    layout->addLayout(filterLayout);
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
}

void AddRunConfigDialog::accept()
{
    const QModelIndexList selected = m_view->selectionModel()->selectedRows();
    QTC_ASSERT(selected.count() == 1, return);
    const auto * const proxyModel = static_cast<ProxyModel *>(m_view->model());
    const auto * const model = static_cast<CandidatesModel *>(proxyModel->sourceModel());
    const TreeItem * const item = model->itemForIndex(proxyModel->mapToSource(selected.first()));
    QTC_ASSERT(item, return);
    m_creationInfo = static_cast<const CandidateTreeItem *>(item)->creationInfo();
    QTC_ASSERT(m_creationInfo.factory, return);
    QDialog::accept();
}

} // namespace Internal
} // namespace ProjectExplorer
