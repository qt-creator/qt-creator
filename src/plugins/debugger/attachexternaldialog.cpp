/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "attachexternaldialog.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QStandardItemModel>
#include <QHeaderView>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#endif

using namespace Debugger::Internal;

AttachExternalDialog::AttachExternalDialog(QWidget *parent)
  : QDialog(parent)
{
    setupUi(this);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_model = new QStandardItemModel(this);

    procView->setSortingEnabled(true);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(procView, SIGNAL(activated(const QModelIndex &)),
        this, SLOT(procSelected(const QModelIndex &)));

    rebuildProcessList();
}

static bool isProcessName(const QString &procname)
{
    for (int i = 0; i != procname.size(); ++i)
        if (!procname.at(i).isDigit())
            return false;
    return true;
}

struct ProcData
{
    QString ppid;
    QString name;
    QString state;
};

static void insertItem(QStandardItem *root, const QString &pid,
    const QMap<QString, ProcData> &procs, QMap<QString, QStandardItem *> &known)
{
    //qDebug() << "HANDLING " << pid;
    QStandardItem *parent = 0;
    const ProcData &proc = procs[pid];
    if (1 || pid == "0") { // FIXME: a real tree is not-so-nice to search
        parent = root;
    } else {
        if (!known.contains(proc.ppid))
            insertItem(root, proc.ppid, procs, known);
        parent = known[proc.ppid];
    }
    QList<QStandardItem *> row;
    row.append(new QStandardItem(pid));
    row.append(new QStandardItem(proc.name));
    //row.append(new QStandardItem(proc.ppid));
    row.append(new QStandardItem(proc.state));
    parent->appendRow(row);
    known[pid] = row[0];
}

void AttachExternalDialog::rebuildProcessList()
{
    QStringList procnames = QDir("/proc/").entryList();
    if (procnames.isEmpty()) {
        procView->hide();
        return;
    }
    
    typedef QMap<QString, ProcData> Procs;
    Procs procs;

    foreach (const QString &procname, procnames) {
        if (!isProcessName(procname))
            continue;
        QString filename = "/proc/" + procname + "/stat";
        QFile file(filename);
        file.open(QIODevice::ReadOnly);
        QStringList data = QString::fromLocal8Bit(file.readAll()).split(' ');
        //qDebug() << filename << data;
        ProcData proc;
        proc.name = data.at(1);
        if (proc.name.startsWith('(') && proc.name.endsWith(')'))
            proc.name = proc.name.mid(1, proc.name.size() - 2);
        proc.state = data.at(2);
        proc.ppid = data.at(3);
        procs[procname] = proc;
    }

    m_model->clear();
    QMap<QString, QStandardItem *> known;
    for (Procs::const_iterator it = procs.begin(); it != procs.end(); ++it)
        insertItem(m_model->invisibleRootItem(), it.key(), procs, known);
    m_model->setHeaderData(0, Qt::Horizontal, "Process ID", Qt::DisplayRole);
    m_model->setHeaderData(1, Qt::Horizontal, "Name", Qt::DisplayRole);
    //model->setHeaderData(2, Qt::Horizontal, "Parent", Qt::DisplayRole);
    m_model->setHeaderData(2, Qt::Horizontal, "State", Qt::DisplayRole);

    procView->setModel(m_model);
    procView->expandAll();
    procView->resizeColumnToContents(0);
    procView->resizeColumnToContents(1);
    procView->sortByColumn(1, Qt::AscendingOrder);
}

void AttachExternalDialog::procSelected(const QModelIndex &index0)
{
    QModelIndex index = index0.sibling(index0.row(), 0);
    QStandardItem *item = m_model->itemFromIndex(index);
    if (!item)
        return;
    pidLineEdit->setText(item->text());
    accept();
}

int AttachExternalDialog::attachPID() const
{
    return pidLineEdit->text().toInt();
}
