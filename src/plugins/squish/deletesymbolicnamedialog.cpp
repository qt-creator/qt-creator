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

#include "squishtr.h"

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>

#include <QAbstractButton>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QItemSelection>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QRadioButton>
#include <QSortFilterProxyModel>
#include <QStringListModel>

namespace Squish {
namespace Internal {

DeleteSymbolicNameDialog::DeleteSymbolicNameDialog(const QString &symbolicName,
                                                   const QStringList &names,
                                                   QWidget *parent)
    : QDialog(parent)
    , m_result(ResetReference)
{
    m_detailsLabel = new QLabel(Tr::tr("Details"));
    m_detailsLabel->setWordWrap(true);

    auto adjustReferencesRB = new QRadioButton;
    adjustReferencesRB->setText(Tr::tr("Adjust references to the removed symbolic name to point to:"));
    adjustReferencesRB->setChecked(true);

    auto filterLineEdit = new Utils::FancyLineEdit;
    filterLineEdit->setFiltering(true);

    m_symbolicNamesList = new QListView;

    auto removeAndInvalidateRB = new QRadioButton;
    removeAndInvalidateRB->setText(Tr::tr("Remove the symbolic name (invalidates names referencing it)"));

    auto removeAllRB = new QRadioButton;
    removeAllRB->setText(Tr::tr("Remove the symbolic name and all names referencing it"));

    m_buttonBox = new QDialogButtonBox;
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    m_listModel = new QStringListModel(this);
    m_filterModel = new QSortFilterProxyModel(this);
    m_filterModel->setSourceModel(m_listModel);
    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_symbolicNamesList->setModel(m_filterModel);

    updateDetailsLabel(symbolicName);
    populateSymbolicNamesList(names);

    using namespace Utils::Layouting;

    Column {
        m_detailsLabel,
        adjustReferencesRB,
        filterLineEdit,
        m_symbolicNamesList,
        removeAndInvalidateRB,
        removeAllRB,
        m_buttonBox
    }.attachTo(this);

    connect(adjustReferencesRB,
            &QRadioButton::toggled,
            this,
            &DeleteSymbolicNameDialog::onAdjustReferencesToggled);
    connect(removeAndInvalidateRB, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            m_result = InvalidateNames;
    });
    connect(removeAllRB, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked)
            m_result = RemoveNames;
    });
    connect(m_symbolicNamesList->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &DeleteSymbolicNameDialog::onSelectionChanged);
    connect(filterLineEdit,
            &Utils::FancyLineEdit::filterChanged,
            m_filterModel,
            &QSortFilterProxyModel::setFilterFixedString);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

DeleteSymbolicNameDialog::~DeleteSymbolicNameDialog() = default;

void DeleteSymbolicNameDialog::updateDetailsLabel(const QString &nameToDelete)
{
    const char *detailsText = QT_TR_NOOP(
        "The Symbolic Name <span style='white-space: nowrap'>\"%1\"</span> you "
        "want to remove is used in Multi Property Names. Please decide what to do "
        "with the references in these Multi Property Names.");
    m_detailsLabel->setText(tr(detailsText).arg(nameToDelete));
}

void DeleteSymbolicNameDialog::populateSymbolicNamesList(const QStringList &symbolicNames)
{
    m_listModel->setStringList(symbolicNames);
    m_filterModel->sort(0);
}

void DeleteSymbolicNameDialog::onAdjustReferencesToggled(bool checked)
{
    m_symbolicNamesList->setEnabled(checked);
    const bool enable = !checked || m_symbolicNamesList->selectionModel()->hasSelection();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
    if (checked)
        m_result = ResetReference;
}

void DeleteSymbolicNameDialog::onSelectionChanged(const QItemSelection &selection,
                                                  const QItemSelection &)
{
    const bool empty = selection.isEmpty();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!empty);
    if (empty)
        m_selected.clear();
    else
        m_selected = selection.indexes().first().data().toString();
}

} // namespace Internal
} // namespace Squish
