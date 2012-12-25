/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "submitfilemodel.h"
#include "vcsbaseconstants.h"

#include <coreplugin/fileiconprovider.h>

#include <QStandardItem>
#include <QFileInfo>
#include <QDebug>

namespace VcsBase {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static QList<QStandardItem *> createFileRow(const QString &fileName, const QString &status,
                                            CheckMode checked, const QVariant &v)
{
    QStandardItem *statusItem = new QStandardItem(status);
    statusItem->setCheckable(checked != Uncheckable);
    if (checked != Uncheckable)
        statusItem->setCheckState(checked == Checked ? Qt::Checked : Qt::Unchecked);
    statusItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    statusItem->setData(v);
    QStandardItem *fileItem = new QStandardItem(fileName);
    fileItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    fileItem->setIcon(Core::FileIconProvider::instance()->icon(QFileInfo(fileName)));
    QList<QStandardItem *> row;
    row << statusItem << fileItem;
    return row;
}

// --------------------------------------------------------------------------
// SubmitFileModel:
// --------------------------------------------------------------------------

/*!
    \class VcsBase::SubmitFileModel

    \brief A 2-column (checkable, state, file name) model to be used to list the files
    in the submit editor. Provides header items and a convience to add files.
 */

SubmitFileModel::SubmitFileModel(QObject *parent) :
    QStandardItemModel(0, 2, parent)
{
    // setColumnCount(2);
    QStringList headerLabels;
    headerLabels << tr("State") << tr("File");
    setHorizontalHeaderLabels(headerLabels);
}

QList<QStandardItem *> SubmitFileModel::addFile(const QString &fileName, const QString &status, CheckMode checkMode,
                                                const QVariant &v)
{
    const QList<QStandardItem *> row = createFileRow(fileName, status, checkMode, v);
    appendRow(row);
    return row;
}

QList<QStandardItem *> SubmitFileModel::rowAt(int row) const
{
    const int colCount = columnCount();
    QList<QStandardItem *> rc;
    for (int c = 0; c < colCount; c++)
        rc.push_back(item(row, c));
    return rc;
}

QString SubmitFileModel::state(int row) const
{
    if (row < 0 || row >= rowCount())
        return QString();
    return item(row)->text();
}

QString SubmitFileModel::file(int row) const
{
    if (row < 0 || row >= rowCount())
        return QString();
    return item(row, 1)->text();
}

bool SubmitFileModel::checked(int row) const
{
    if (row < 0 || row >= rowCount())
        return false;
    return (item(row)->checkState() == Qt::Checked);
}

QVariant SubmitFileModel::extraData(int row) const
{
    if (row < 0 || row >= rowCount())
        return false;
    return item(row)->data();
}

bool SubmitFileModel::hasCheckedFiles() const
{
    for (int i = 0; i < rowCount(); ++i) {
        if (checked(i))
            return true;
    }
    return false;
}

QList<QStandardItem *> SubmitFileModel::findRow(const QString &text, int column) const
{
    // Single item
    const QList<QStandardItem *> items = findItems(text, Qt::MatchExactly, column);
    if (items.empty())
        return items;
    // Compile row
    return rowAt(items.front()->row());
 }

unsigned SubmitFileModel::filter(const QStringList &filter, int column)
{
    unsigned rc = 0;
    for (int r = rowCount() - 1; r >= 0; r--)
        if (const QStandardItem *i = item(r, column))
            if (!filter.contains(i->text())) {
                qDeleteAll(takeRow(r));
                rc++;
            }
    return rc;
}

} // namespace VcsBase
