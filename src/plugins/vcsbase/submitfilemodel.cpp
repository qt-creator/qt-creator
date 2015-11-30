/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "submitfilemodel.h"

#include <coreplugin/fileiconprovider.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QStandardItem>
#include <QFileInfo>
#include <QDebug>

namespace VcsBase {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

enum { stateColumn = 0, fileColumn = 1 };

static QBrush fileStatusTextForeground(SubmitFileModel::FileStatusHint statusHint)
{
    using Utils::Theme;
    Theme::Color statusTextColor = Theme::VcsBase_FileStatusUnknown_TextColor;
    switch (statusHint) {
    case SubmitFileModel::FileStatusUnknown:
        statusTextColor = Theme::VcsBase_FileStatusUnknown_TextColor;
        break;
    case SubmitFileModel::FileAdded:
        statusTextColor = Theme::VcsBase_FileAdded_TextColor;
        break;
    case SubmitFileModel::FileModified:
        statusTextColor = Theme::VcsBase_FileModified_TextColor;
        break;
    case SubmitFileModel::FileDeleted:
        statusTextColor = Theme::VcsBase_FileDeleted_TextColor;
        break;
    case SubmitFileModel::FileRenamed:
        statusTextColor = Theme::VcsBase_FileRenamed_TextColor;
        break;
    }
    return QBrush(Utils::creatorTheme()->color(statusTextColor));
}

static QList<QStandardItem *> createFileRow(const QString &repositoryRoot,
                                            const QString &fileName,
                                            const QString &status,
                                            SubmitFileModel::FileStatusHint statusHint,
                                            CheckMode checked,
                                            const QVariant &v)
{
    auto statusItem = new QStandardItem(status);
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (checked != Uncheckable) {
        flags |= Qt::ItemIsUserCheckable;
        statusItem->setCheckState(checked == Checked ? Qt::Checked : Qt::Unchecked);
    }
    statusItem->setFlags(flags);
    statusItem->setData(v);
    auto fileItem = new QStandardItem(fileName);
    fileItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    // For some reason, Windows (at least) requires a valid (existing) file path to the icon, so
    // the repository root is needed here.
    // Note: for "overlaid" icons in Core::FileIconProvider a valid file path is not required
    const QFileInfo fi(repositoryRoot + QLatin1Char('/') + fileName);
    fileItem->setIcon(Core::FileIconProvider::icon(fi));
    QList<QStandardItem *> row;
    row << statusItem << fileItem;
    if (statusHint != SubmitFileModel::FileStatusUnknown) {
        const QBrush textForeground = fileStatusTextForeground(statusHint);
        foreach (QStandardItem *item, row)
            item->setForeground(textForeground);
    }
    return row;
}

// --------------------------------------------------------------------------
// SubmitFileModel:
// --------------------------------------------------------------------------

/*!
    \class VcsBase::SubmitFileModel

    \brief The SubmitFileModel class is a 2-column (checkable, state, file name)
    model to be used to list the files in the submit editor.

    Provides header items and a convenience function to add files.
 */

SubmitFileModel::SubmitFileModel(QObject *parent) :
    QStandardItemModel(0, 2, parent)
{
    // setColumnCount(2);
    QStringList headerLabels;
    headerLabels << tr("State") << tr("File");
    setHorizontalHeaderLabels(headerLabels);
}

const QString &SubmitFileModel::repositoryRoot() const
{
    return m_repositoryRoot;
}

void SubmitFileModel::setRepositoryRoot(const QString &repoRoot)
{
    m_repositoryRoot = repoRoot;
}

QList<QStandardItem *> SubmitFileModel::addFile(const QString &fileName, const QString &status, CheckMode checkMode,
                                                const QVariant &v)
{
    const FileStatusHint statusHint =
            m_fileStatusQualifier ? m_fileStatusQualifier(status, v) : FileStatusUnknown;
    const QList<QStandardItem *> row =
            createFileRow(m_repositoryRoot, fileName, status, statusHint, checkMode, v);
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

bool SubmitFileModel::isCheckable(int row) const
{
    if (row < 0 || row >= rowCount())
        return false;
    return item(row)->isCheckable();
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
    for (int row = 0; row < rows; ++row) {
        QStandardItem *i = item(row);
        if (i->isCheckable())
            i->setCheckState(check ? Qt::Checked : Qt::Unchecked);
    }
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
 *  Assumes that both models are sorted with the same order, and there
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
                if (isCheckable(i) && source->isCheckable(j))
                    setChecked(i, source->checked(j));
                lastMatched = j + 1; // No duplicates, start on next entry
                break;
            }
        }
    }
}

const SubmitFileModel::FileStatusQualifier &SubmitFileModel::fileStatusQualifier() const
{
    return m_fileStatusQualifier;
}

void SubmitFileModel::setFileStatusQualifier(FileStatusQualifier &&func)
{
    const int topLevelRowCount = rowCount();
    const int topLevelColCount = columnCount();
    for (int row = 0; row < topLevelRowCount; ++row) {
        const QStandardItem *statusItem = item(row, stateColumn);
        const FileStatusHint statusHint =
                func ? func(statusItem->text(), statusItem->data()) : FileStatusUnknown;
        const QBrush textForeground = fileStatusTextForeground(statusHint);
        for (int col = 0; col < topLevelColCount; ++col)
            item(row, col)->setForeground(textForeground);
    }
    m_fileStatusQualifier = func;
}

} // namespace VcsBase
