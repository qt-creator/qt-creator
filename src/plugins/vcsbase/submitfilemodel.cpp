/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "submitfilemodel.h"
#include "vcsbaseconstants.h"

#include <QtGui/QStandardItem>
#include <QtCore/QDebug>

namespace VCSBase {

SubmitFileModel::SubmitFileModel(QObject *parent) :
    QStandardItemModel(0, 2, parent)
{
    // setColumnCount(2);
    QStringList headerLabels;
    headerLabels << tr("State") << tr("File");
    setHorizontalHeaderLabels(headerLabels);
}

QList<QStandardItem *> SubmitFileModel::createFileRow(const QString &fileName, const QString &status, bool checked)
{
    if (VCSBase::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << fileName << status << checked;
    QStandardItem *statusItem = new QStandardItem(status);
    statusItem->setCheckable(true);
    statusItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    statusItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    QStandardItem *fileItem = new QStandardItem(fileName);
    fileItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    QList<QStandardItem *> row;
    row << statusItem << fileItem;
    return row;
}

QList<QStandardItem *> SubmitFileModel::addFile(const QString &fileName, const QString &status, bool checked)
{
    const QList<QStandardItem *> row = createFileRow(fileName, status, checked);
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
    if (VCSBase::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << " deleted " << rc << " items using " << filter << " , remaining " << rowCount();
    return rc;
}
}
