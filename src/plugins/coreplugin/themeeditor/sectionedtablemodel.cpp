/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
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

#include "sectionedtablemodel.h"

#include <QSize>

namespace Core {
namespace Internal {
namespace ThemeEditor {

SectionedTableModel::SectionedTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

int SectionedTableModel::sectionHeader(int row) const
{
    int currRow = 0;
    int i = 0;
    do {
        if (row == currRow)
            return i;
        currRow += sectionRowCount(i) + 1; //account for next header
        ++i;
    } while (i < sectionCount());
    return -1;
}

int SectionedTableModel::inSectionBody(int row) const
{
    int currRow = 0;
    int i = 0;
    do {
        ++currRow;
        if (row >= currRow && row < currRow + sectionRowCount(i))
            return i;
        currRow += sectionRowCount(i);
        ++i;
    } while (i < sectionCount());
    return -1;
}

int SectionedTableModel::modelToSectionRow(int row) const
{
    int currRow = 0;
    for (int i = 0; i < sectionCount(); ++i) {
        ++currRow;
        if (row >= currRow && row < currRow + sectionRowCount(i))
            return row-currRow;
        currRow += sectionRowCount(i);
    }
    return row;
}

QSize SectionedTableModel::span(const QModelIndex &index) const
{
    if (sectionHeader(index.row()) >= 0 && index.column() == 0)
        return QSize(1, columnCount(index));
    return QSize(1, 1);
}

int SectionedTableModel::rowCount(const QModelIndex &index) const
{
    if (index.isValid())
        return 0;

    int r = 0;
    for (int i = 0; i < sectionCount(); ++i)
        r += sectionRowCount(i) + 1;
    return r;
}

QVariant SectionedTableModel::data(const QModelIndex &index, int role) const
{
    int header = sectionHeader(index.row());
    if (header >= 0)
        return (index.column() == 0) ? sectionHeaderData(header, role)
                                     : QVariant(QString());
    return sectionBodyData(inSectionBody(index.row()),
                           modelToSectionRow(index.row()),
                           index.column(),
                           role);
}

Qt::ItemFlags SectionedTableModel::flags(const QModelIndex &index) const
{
    if (int h = sectionHeader(index.row()) >= 0)
        return sectionHeaderFlags(h);
    return sectionBodyFlags(inSectionBody(index.row()),
                            modelToSectionRow(index.row()),
                            index.column());
}

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core
