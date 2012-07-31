/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "devicesupport/deviceprocessesdialog.h"
#include "devicesupport/deviceprocesslist.h"
#include "ui_deviceprocessesdialog.h"

#include <QMessageBox>
#include <QSortFilterProxyModel>

namespace ProjectExplorer {
namespace Internal {

class DeviceProcessesDialogPrivate
{
public:
    DeviceProcessesDialogPrivate(DeviceProcessList *processList)
        : processList(processList)
    {
    }

    Ui::DeviceProcessesDialog ui;
    DeviceProcessList * const processList;
    QSortFilterProxyModel proxyModel;
};

} // namespace Internal

using namespace Internal;

DeviceProcessesDialog::DeviceProcessesDialog(DeviceProcessList *processList, QWidget *parent)
    : QDialog(parent), d(new DeviceProcessesDialogPrivate(processList))
{
    processList->setParent(this);

    d->ui.setupUi(this);
    d->proxyModel.setSourceModel(d->processList);
    d->proxyModel.setDynamicSortFilter(true);
    d->proxyModel.setFilterKeyColumn(-1);
    d->ui.treeView->setModel(&d->proxyModel);
    d->ui.treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->ui.treeView->setUniformRowHeights(true);
    connect(d->ui.processFilterLineEdit, SIGNAL(textChanged(QString)),
        &d->proxyModel, SLOT(setFilterRegExp(QString)));

    connect(d->ui.treeView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        SLOT(handleSelectionChanged()));
    connect(d->ui.updateListButton, SIGNAL(clicked()),
        SLOT(updateProcessList()));
    connect(d->ui.killProcessButton, SIGNAL(clicked()), SLOT(killProcess()));
    connect(d->processList, SIGNAL(error(QString)),
        SLOT(handleRemoteError(QString)));
    connect(d->processList, SIGNAL(processListUpdated()),
        SLOT(handleProcessListUpdated()));
    connect(d->processList, SIGNAL(processKilled()),
        SLOT(handleProcessKilled()), Qt::QueuedConnection);
    connect(&d->proxyModel, SIGNAL(layoutChanged()),
        SLOT(handleProcessListUpdated()));
    handleSelectionChanged();
    updateProcessList();
}

DeviceProcessesDialog::~DeviceProcessesDialog()
{
    delete d;
}

void DeviceProcessesDialog::handleRemoteError(const QString &errorMsg)
{
    QMessageBox::critical(this, tr("Remote Error"), errorMsg);
    d->ui.updateListButton->setEnabled(true);
    handleSelectionChanged();
}

void DeviceProcessesDialog::handleProcessListUpdated()
{
    d->ui.updateListButton->setEnabled(true);
    handleSelectionChanged();
}

void DeviceProcessesDialog::updateProcessList()
{
    d->ui.updateListButton->setEnabled(false);
    d->ui.killProcessButton->setEnabled(false);
    d->processList->update();
}

void DeviceProcessesDialog::killProcess()
{
    const QModelIndexList &indexes
        = d->ui.treeView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;
    d->ui.updateListButton->setEnabled(false);
    d->ui.killProcessButton->setEnabled(false);
    d->processList->killProcess(d->proxyModel.mapToSource(indexes.first()).row());
}

void DeviceProcessesDialog::handleProcessKilled()
{
    updateProcessList();
}

void DeviceProcessesDialog::handleSelectionChanged()
{
    d->ui.killProcessButton->setEnabled(d->ui.treeView->selectionModel()->hasSelection());
}

} // namespace RemoteLinux
