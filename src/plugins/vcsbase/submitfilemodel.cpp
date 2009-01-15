/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
