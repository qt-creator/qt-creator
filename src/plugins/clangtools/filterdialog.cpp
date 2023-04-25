// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filterdialog.h"

#include "clangtoolstr.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/treemodel.h>

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QTreeView>

namespace ClangTools::Internal {

enum Columns { CheckName, Count };

class CheckItem : public Utils::TreeItem
{
public:
    CheckItem(const Check &check) : check(check) {}

    QVariant data(int column, int role) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        switch (column) {
        case Columns::CheckName: return check.displayName;
        case Columns::Count: return check.count;
        }
        return {};
    }

    Check check;
};

static QItemSelectionModel::SelectionFlags selectionFlags()
{
    return QItemSelectionModel::Select | QItemSelectionModel::Rows;
}

class FilterChecksModel : public Utils::TreeModel<Utils::TreeItem, CheckItem>
{
    Q_OBJECT

public:
    FilterChecksModel(const Checks &checks)
    {
        const Checks sortedChecks = Utils::sorted(checks, [](const Check &lhs, const Check &rhs) {
            return lhs.displayName < rhs.displayName;
        });

        setHeader({Tr::tr("Check"), "#"});
        setRootItem(new Utils::StaticTreeItem(QString()));
        for (const Check &check : sortedChecks)
            m_root->appendChild(new CheckItem(check));
    }
};

FilterDialog::FilterDialog(const Checks &checks, QWidget *parent)
    : QDialog(parent)
{
    resize(400, 400);
    setWindowTitle(Tr::tr("Filter Diagnostics"));

    auto selectAll = new QPushButton(Tr::tr("Select All"));
    auto selectWithFixits = new QPushButton(Tr::tr("Select All with Fixits"));
    auto selectNone = new QPushButton(Tr::tr("Clear Selection"));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    m_model = new FilterChecksModel(checks);

    m_view = new QTreeView(this);
    m_view->setModel(m_model);
    m_view->header()->setStretchLastSection(false);
    m_view->header()->setSectionResizeMode(Columns::CheckName, QHeaderView::Stretch);
    m_view->header()->setSectionResizeMode(Columns::Count, QHeaderView::ResizeToContents);
    m_view->setSelectionMode(QAbstractItemView::MultiSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setIndentation(0);

    using namespace Layouting;

    Column {
        Tr::tr("Select the diagnostics to display."),
        Row { selectAll, selectWithFixits, selectNone, st },
        m_view,
        buttonBox,
    }.attachTo(this);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=] {
        const bool hasSelection = !m_view->selectionModel()->selectedRows().isEmpty();
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasSelection);
    });

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Buttons
    connect(selectNone, &QPushButton::clicked, m_view, &QTreeView::clearSelection);
    connect(selectAll, &QPushButton::clicked, m_view, &QTreeView::selectAll);
    connect(selectWithFixits, &QPushButton::clicked, m_view, [this] {
        m_view->clearSelection();
        m_model->forItemsAtLevel<1>([&](CheckItem *item) {
            if (item->check.hasFixit)
                m_view->selectionModel()->select(item->index(), selectionFlags());
        });
    });
    selectWithFixits->setEnabled(
        Utils::anyOf(checks, [](const Check &c) { return c.hasFixit; }));

    // Select checks that are not filtered out
    m_model->forItemsAtLevel<1>([this](CheckItem *item) {
        if (item->check.isShown)
            m_view->selectionModel()->select(item->index(), selectionFlags());
    });
}

FilterDialog::~FilterDialog() = default;

QSet<QString> FilterDialog::selectedChecks() const
{
    QSet<QString> checks;
    m_model->forItemsAtLevel<1>([&](CheckItem *item) {
        if (m_view->selectionModel()->isSelected(item->index()))
            checks << item->check.name;
    });
    return checks;
}

} // ClangTools::Internal

#include "filterdialog.moc"
