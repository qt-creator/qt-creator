/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "remotelinuxprocessesdialog.h"
#include "ui_remotelinuxprocessesdialog.h"

#include "remotelinuxprocesslist.h"

#include <QtGui/QMessageBox>
#include <QtGui/QSortFilterProxyModel>

namespace RemoteLinux {
namespace Internal {
class RemoteLinuxProcessesDialogPrivate
{
public:
    RemoteLinuxProcessesDialogPrivate(AbstractRemoteLinuxProcessList *processList)
        : processList(processList)
    {
    }

    Ui::RemoteLinuxProcessesDialog ui;
    AbstractRemoteLinuxProcessList * const processList;
    QSortFilterProxyModel proxyModel;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxProcessesDialog::RemoteLinuxProcessesDialog(AbstractRemoteLinuxProcessList *processList,
        QWidget *parent)
    : QDialog(parent), m_d(new RemoteLinuxProcessesDialogPrivate(processList))
{
    processList->setParent(this);

    m_d->ui.setupUi(this);
    m_d->ui.tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_d->proxyModel.setSourceModel(m_d->processList);
    m_d->proxyModel.setDynamicSortFilter(true);
    m_d->proxyModel.setFilterKeyColumn(1);
    m_d->ui.tableView->setModel(&m_d->proxyModel);
    connect(m_d->ui.processFilterLineEdit, SIGNAL(textChanged(QString)),
        &m_d->proxyModel, SLOT(setFilterRegExp(QString)));

    connect(m_d->ui.tableView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        SLOT(handleSelectionChanged()));
    connect(m_d->ui.updateListButton, SIGNAL(clicked()),
        SLOT(updateProcessList()));
    connect(m_d->ui.killProcessButton, SIGNAL(clicked()), SLOT(killProcess()));
    connect(m_d->processList, SIGNAL(error(QString)),
        SLOT(handleRemoteError(QString)));
    connect(m_d->processList, SIGNAL(modelReset()),
        SLOT(handleProcessListUpdated()));
    connect(m_d->processList, SIGNAL(processKilled()),
        SLOT(handleProcessKilled()), Qt::QueuedConnection);
    connect(&m_d->proxyModel, SIGNAL(layoutChanged()),
        SLOT(handleProcessListUpdated()));
    handleSelectionChanged();
    updateProcessList();
}

RemoteLinuxProcessesDialog::~RemoteLinuxProcessesDialog()
{
    delete m_d;
}

void RemoteLinuxProcessesDialog::handleRemoteError(const QString &errorMsg)
{
    QMessageBox::critical(this, tr("Remote Error"), errorMsg);
    m_d->ui.updateListButton->setEnabled(true);
    handleSelectionChanged();
}

void RemoteLinuxProcessesDialog::handleProcessListUpdated()
{
    m_d->ui.updateListButton->setEnabled(true);
    m_d->ui.tableView->resizeRowsToContents();
    handleSelectionChanged();
}

void RemoteLinuxProcessesDialog::updateProcessList()
{
    m_d->ui.updateListButton->setEnabled(false);
    m_d->ui.killProcessButton->setEnabled(false);
    m_d->processList->update();
}

void RemoteLinuxProcessesDialog::killProcess()
{
    const QModelIndexList &indexes
        = m_d->ui.tableView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;
    m_d->ui.updateListButton->setEnabled(false);
    m_d->ui.killProcessButton->setEnabled(false);
    m_d->processList->killProcess(m_d->proxyModel.mapToSource(indexes.first()).row());
}

void RemoteLinuxProcessesDialog::handleProcessKilled()
{
    updateProcessList();
}

void RemoteLinuxProcessesDialog::handleSelectionChanged()
{
    m_d->ui.killProcessButton->setEnabled(m_d->ui.tableView->selectionModel()->hasSelection());
}

} // namespace RemoteLinux
