/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gerritdialog.h"
#include "gerritmodel.h"
#include "gerritparameters.h"

#include <utils/filterlineedit.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QTreeView>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QSortFilterProxyModel>
#include <QUrl>
#include <QClipboard>
#include <QApplication>
#include <QProcess>

namespace Gerrit {
namespace Internal {

GerritDialog::GerritDialog(const QSharedPointer<GerritParameters> &p,
                           QWidget *parent)
    : QDialog(parent)
    , m_parameters(p)
    , m_filterModel(new QSortFilterProxyModel(this))
    , m_model(new GerritModel(p, this))
    , m_treeView(new QTreeView)
    , m_filterLineEdit(new Utils::FilterLineEdit)
    , m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Close))
{
    setWindowTitle(tr("Gerrit %1@%2").arg(p->user, p->host));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout *l = new QVBoxLayout(this);
    QHBoxLayout *hl = new QHBoxLayout;
    hl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
    m_filterLineEdit->setFixedWidth(300);
    hl->addWidget(m_filterLineEdit);
    connect(m_filterLineEdit, SIGNAL(filterChanged(QString)),
            m_filterModel, SLOT(setFilterFixedString(QString)));
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    l->addLayout(hl);
    l->addWidget(m_treeView);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(GerritModel::FilterRole);
    m_treeView->setModel(m_filterModel);
    m_treeView->setMinimumWidth(600);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

    QItemSelectionModel *selectionModel = m_treeView->selectionModel();
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotEnableButtons()));
    connect(m_treeView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(slotDoubleClicked(QModelIndex)));
    m_openButton = addActionButton(tr("Open"), SLOT(slotOpenBrowser()));
    m_copyUrlButton = addActionButton(tr("Copy URL"), SLOT(slotCopyUrl()));
    m_displayButton = addActionButton(tr("Diff..."), SLOT(slotFetchDisplay()));
    m_applyButton = addActionButton(tr("Apply..."), SLOT(slotFetchApply()));
    m_checkoutButton = addActionButton(tr("Checkout..."), SLOT(slotFetchCheckout()));
    m_refreshButton = addActionButton(tr("Refresh"), SLOT(slotRefresh()));

    connect(m_model, SIGNAL(refreshStateChanged(bool)),
            m_refreshButton, SLOT(setDisabled(bool)));
    connect(m_model, SIGNAL(refreshStateChanged(bool)),
            this, SLOT(slotRefreshStateChanged(bool)));
    l->addWidget(m_buttonBox);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    slotEnableButtons();
    m_model->refresh();

    resize(QSize(1200, 400));
}

QPushButton *GerritDialog::addActionButton(const QString &text, const char *buttonSlot)
{
    QPushButton *button = m_buttonBox->addButton(text, QDialogButtonBox::ActionRole);
    connect(button, SIGNAL(clicked()), this, buttonSlot);
    return button;
}

GerritDialog::~GerritDialog()
{
}

void GerritDialog::slotDoubleClicked(const QModelIndex &i)
{
    if (const QStandardItem *item = itemAt(i))
        openUrl(item->row());
}

void GerritDialog::slotRefreshStateChanged(bool v)
{
    if (!v && m_model->rowCount())
        for (int c = 0; c < GerritModel::ColumnCount; ++c)
            m_treeView->resizeColumnToContents(c);
}

void GerritDialog::slotOpenBrowser()
{
    if (const QStandardItem *item = currentItem())
        openUrl(item->row());
}

void GerritDialog::slotFetchDisplay()
{
    if (const QStandardItem *item = currentItem())
        emit fetchDisplay(m_model->change(item->row()));
}

void GerritDialog::slotFetchApply()
{
    if (const QStandardItem *item = currentItem())
        emit fetchApply(m_model->change(item->row()));
}

void GerritDialog::slotFetchCheckout()
{
    if (const QStandardItem *item = currentItem())
        emit fetchCheckout(m_model->change(item->row()));
}

void GerritDialog::slotCopyUrl()
{
    if (const QStandardItem *item = currentItem())
        QApplication::clipboard()->setText(m_model->change(item->row())->url);
}

void GerritDialog::slotRefresh()
{
    m_model->refresh();
}

void GerritDialog::openUrl(int row) const
{
    QDesktopServices::openUrl(QUrl(m_model->change(row)->url));
}

const QStandardItem *GerritDialog::itemAt(const QModelIndex &i, int column) const
{
    if (i.isValid()) {
        const QModelIndex source = m_filterModel->mapToSource(i);
        if (source.isValid())
            return m_model->item(source.row(), column);
    }
    return 0;
}

const QStandardItem *GerritDialog::currentItem(int column) const
{
    const QModelIndex index = m_treeView->selectionModel()->currentIndex();
    if (index.isValid())
        return itemAt(index, column);
    return 0;
}

void GerritDialog::slotEnableButtons()
{
    const bool valid = m_treeView->selectionModel()->currentIndex().isValid();
    m_openButton->setEnabled(valid);
    m_displayButton->setEnabled(valid);
    m_copyUrlButton->setEnabled(valid);
    m_applyButton->setEnabled(valid);
    m_checkoutButton->setEnabled(valid);
}

} // namespace Internal
} // namespace Gerrit
