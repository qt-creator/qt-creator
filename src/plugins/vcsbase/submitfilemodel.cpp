/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include <utils/qtcassert.h>

#include <QStandardItem>
#include <QFileInfo>
#include <QDebug>

namespace VcsBase {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

enum { fileColumn = 1 };

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
    return item(row, fileColumn)->text();
}

bool SubmitFileModel::checked(int row) const
{
    if (row < 0 || row >= rowCount())
        return false;
    return (item(row)->checkState() == Qt::Checked);
}

void SubmitFileModel::setChecked(int row, bool check)
{
    if (row >= 0 || row < rowCount())
        item(row)->setCheckState(check ? Qt::Checked : Qt::Unchecked);
}

void SubmitFileModel::setAllChecked(bool check)
{
    int rows = rowCount();
    for (int row = 0; row < rows; ++row)
        item(row)->setCheckState(check ? Qt::Checked : Qt::Unchecked);
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

unsigned int SubmitFileModel::filterFiles(const QStringList &filter)
{
    unsigned int rc = 0;
    for (int r = rowCount() - 1; r >= 0; r--)
        if (!filter.contains(file(r))) {
            removeRow(r);
            rc++;
        }
    return rc;
}

/*! Updates user selections from \a source model.
 *
 *  Assumption: Both model are sorted with the same order, and there
 *              are no duplicate entries.
 */
void SubmitFileModel::updateSelections(SubmitFileModel *source)
{
    QTC_ASSERT(source, return);
    int rows = rowCount();
    int sourceRows = source->rowCount();
    int lastMatched = 0;
    for (int i = 0; i < rows; ++i) {
        // Since both models are sorted with the same order, there is no need
        // to test rows earlier than latest match found
        for (int j = lastMatched; j < sourceRows; ++j) {
            if (file(i) == source->file(j) && state(i) == source->state(j)) {
                setChecked(i, source->checked(j));
                lastMatched = j + 1; // No duplicates, start on next entry
                break;
            }
        }
    }
}

} // namespace VcsBase
