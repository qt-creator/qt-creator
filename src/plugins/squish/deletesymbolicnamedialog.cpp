/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "deletesymbolicnamedialog.h"
#include "ui_deletesymbolicnamedialog.h"

#include <QItemSelection>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStringListModel>

namespace Squish {
namespace Internal {

DeleteSymbolicNameDialog::DeleteSymbolicNameDialog(const QString &symbolicName,
                                                   const QStringList &names,
                                                   QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeleteSymbolicNameDialog)
    , m_result(ResetReference)
{
    ui->setupUi(this);
    ui->filterLineEdit->setFiltering(true);

    m_listModel = new QStringListModel(this);
    m_filterModel = new QSortFilterProxyModel(this);
    m_filterModel->setSourceModel(m_listModel);
    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    ui->symbolicNamesList->setModel(m_filterModel);

    updateDetailsLabel(symbolicName);
    populateSymbolicNamesList(names);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->adjustReferencesRB,
            &QRadioButton::toggled,
            this,
            &DeleteSymbolicNameDialog::onAdjustReferencesToggled);
    connect(ui->removeAndInvalidateRB, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            m_result = InvalidateNames;
    });
    connect(ui->removeAllRB, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            m_result = RemoveNames;
    });
    connect(ui->symbolicNamesList->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &DeleteSymbolicNameDialog::onSelectionChanged);
    connect(ui->filterLineEdit,
            &Utils::FancyLineEdit::filterChanged,
            m_filterModel,
            &QSortFilterProxyModel::setFilterFixedString);
}

DeleteSymbolicNameDialog::~DeleteSymbolicNameDialog()
{
    delete ui;
}

void DeleteSymbolicNameDialog::updateDetailsLabel(const QString &nameToDelete)
{
    const char *detailsText = QT_TR_NOOP(
        "The Symbolic Name <span style='white-space: nowrap'>\"%1\"</span> you "
        "want to remove is used in Multi Property Names. Please decide what to do "
        "with the references in these Multi Property Names.");
    ui->detailsLabel->setText(tr(detailsText).arg(nameToDelete));
}

void DeleteSymbolicNameDialog::populateSymbolicNamesList(const QStringList &symbolicNames)
{
    m_listModel->setStringList(symbolicNames);
    m_filterModel->sort(0);
}

void DeleteSymbolicNameDialog::onAdjustReferencesToggled(bool checked)
{
    ui->symbolicNamesList->setEnabled(checked);
    const bool enable = !checked || ui->symbolicNamesList->selectionModel()->hasSelection();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
    if (checked)
        m_result = ResetReference;
}

void DeleteSymbolicNameDialog::onSelectionChanged(const QItemSelection &selection,
                                                  const QItemSelection &)
{
    const bool empty = selection.isEmpty();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!empty);
    if (empty)
        m_selected.clear();
    else
        m_selected = selection.indexes().first().data().toString();
}

} // namespace Internal
} // namespace Squish
