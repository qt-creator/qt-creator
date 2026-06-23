// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processpickerdialog.h"

#include <profiler/profilertr.h>

#include <utils/fancylineedit.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Profiler;
using namespace Utils;

namespace QmlTraceViewer {

enum Column { PidColumn, NameColumn, CommandColumn, ColumnCount };
// The list index into m_processes, stored on the first column so a proxy/sorted
// row maps back to the original ProcessInfo.
constexpr int IndexRole = Qt::UserRole;

ProcessPickerDialog::ProcessPickerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Tr::tr("Attach to Process"));
    resize(700, 500);

    m_filter = new FancyLineEdit(this);
    m_filter->setFiltering(true);
    m_filter->setPlaceholderText(Tr::tr("Filter by name, command line or process id"));

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(
        {Tr::tr("Process ID"), Tr::tr("Name"), Tr::tr("Command Line")});

    if (const Result<QList<ProcessInfo>> infos = ProcessInfo::processInfoList()) {
        m_processes = *infos;
        for (int i = 0; i < m_processes.size(); ++i) {
            const ProcessInfo &info = m_processes.at(i);
            auto pid = new QStandardItem(QString::number(info.processId));
            pid->setData(i, IndexRole);
            const QString name = FilePath::fromUserInput(info.executable).fileName();
            QList<QStandardItem *>
                row{pid, new QStandardItem(name), new QStandardItem(info.commandLine)};
            for (QStandardItem *item : row)
                item->setEditable(false);
            m_model->appendRow(row);
        }
    }

    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(-1); // Match against all columns.
    m_proxy->setSortRole(Qt::DisplayRole);

    m_view = new QTreeView(this);
    m_view->setModel(m_proxy);
    m_view->setRootIsDecorated(false);
    m_view->setUniformRowHeights(true);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSortingEnabled(true);
    m_view->sortByColumn(NameColumn, Qt::AscendingOrder);
    m_view->header()->setStretchLastSection(true);
    m_view->setColumnWidth(PidColumn, 90);
    m_view->setColumnWidth(NameColumn, 220);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setText(Tr::tr("Attach"));
    m_okButton->setEnabled(false);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_filter);
    layout->addWidget(m_view);
    layout->addWidget(buttons);

    connect(
        m_filter,
        &FancyLineEdit::filterChanged,
        m_proxy,
        &QSortFilterProxyModel::setFilterFixedString);
    connect(
        m_view->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &ProcessPickerDialog::updateOkButton);
    connect(m_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        if (index.isValid())
            accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_filter->setFocus();
}

std::optional<ProcessInfo> ProcessPickerDialog::selectedProcess() const
{
    const QModelIndexList selected = m_view->selectionModel()->selectedRows(PidColumn);
    if (selected.isEmpty())
        return std::nullopt;
    const int index = selected.first().data(IndexRole).toInt();
    if (index < 0 || index >= m_processes.size())
        return std::nullopt;
    return m_processes.at(index);
}

std::optional<ProcessInfo> ProcessPickerDialog::pickProcess(QWidget *parent)
{
    ProcessPickerDialog dialog(parent);
    if (dialog.exec() != QDialog::Accepted)
        return std::nullopt;
    return dialog.selectedProcess();
}

void ProcessPickerDialog::updateOkButton()
{
    m_okButton->setEnabled(selectedProcess().has_value());
}

} // namespace QmlTraceViewer
