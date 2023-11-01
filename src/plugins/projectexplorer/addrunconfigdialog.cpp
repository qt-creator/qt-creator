// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addrunconfigdialog.h"

#include "project.h"
#include "projectexplorertr.h"
#include "target.h"

#include <utils/itemviews.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

const Qt::ItemDataRole IsCustomRole = Qt::UserRole;

class CandidateTreeItem : public TreeItem
{
public:
    CandidateTreeItem(const RunConfigurationCreationInfo &rci, const Target *target)
        : m_creationInfo(rci), m_projectRoot(target->project()->projectDirectory()),
          m_displayName(target->macroExpander()->expand(rci.displayName))
    { }

    RunConfigurationCreationInfo creationInfo() const { return m_creationInfo; }

private:
    QVariant data(int column, int role) const override
    {
        QTC_ASSERT(column < 2, return QVariant());
        if (role == IsCustomRole)
            return m_creationInfo.projectFilePath.isEmpty();
        if (column == 0 && role == Qt::DisplayRole)
            return m_displayName;
        if (column == 1 && role == Qt::DisplayRole) {
            FilePath displayPath = m_creationInfo.projectFilePath.relativeChildPath(m_projectRoot);
            if (displayPath.isEmpty()) {
                displayPath = m_creationInfo.projectFilePath;
                QTC_CHECK(displayPath.isEmpty());
            }
            return displayPath.isEmpty() ? Tr::tr("[none]") : displayPath.toUserOutput();
        }
        return {};
    }

    const RunConfigurationCreationInfo m_creationInfo;
    const FilePath m_projectRoot;
    const QString m_displayName;
};

class CandidatesModel : public TreeModel<TreeItem, CandidateTreeItem>
{
public:
    CandidatesModel(Target *target, QObject *parent) : TreeModel(parent)
    {
        setHeader({Tr::tr("Name"), Tr::tr("Source")});
        for (const RunConfigurationCreationInfo &rci
             : RunConfigurationFactory::creatorsForTarget(target)) {
            rootItem()->appendChild(new CandidateTreeItem(rci, target));
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
    CandidatesTreeView(QWidget *parent) : TreeView(parent) {}

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
    setWindowTitle(Tr::tr("Create Run Configuration"));
    const auto model = new CandidatesModel(target, this);
    const auto proxyModel = new ProxyModel(this);
    proxyModel->setSourceModel(model);
    const auto filterEdit = new FancyLineEdit(this);
    filterEdit->setFocus();
    filterEdit->setFiltering(true);
    filterEdit->setPlaceholderText(Tr::tr("Filter candidates by name"));
    m_view->setSelectionMode(TreeView::SingleSelection);
    m_view->setSelectionBehavior(TreeView::SelectRows);
    m_view->setSortingEnabled(true);
    m_view->setModel(proxyModel);
    m_view->resizeColumnToContents(0);
    m_view->resizeColumnToContents(1);
    m_view->sortByColumn(0, Qt::AscendingOrder);
    const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(Tr::tr("Create"));

    connect(filterEdit, &FancyLineEdit::textChanged, this, [proxyModel](const QString &text) {
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
    layout->addWidget(filterEdit);
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
