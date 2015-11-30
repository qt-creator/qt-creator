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

#ifndef SECTIONEDTABLEMODEL_H
#define SECTIONEDTABLEMODEL_H

#include <QAbstractTableModel>

namespace Core {
namespace Internal {
namespace ThemeEditor {

class SectionedTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SectionedTableModel(QObject *parent = 0);

    virtual int sectionRowCount(int section) const = 0;
    virtual QVariant sectionBodyData(int section, int row, int column, int role) const = 0;
    virtual QVariant sectionHeaderData(int section, int role) const = 0;
    virtual Qt::ItemFlags sectionBodyFlags(int section, int row, int column) const = 0;
    virtual Qt::ItemFlags sectionHeaderFlags(int section) const = 0;
    virtual int sectionCount() const = 0;
    QSize span(const QModelIndex &index) const override;

    int inSectionBody(int row) const;
    int modelToSectionRow(int row) const;
    int sectionHeader(int row) const;

protected:
    int rowCount(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core

#endif // SECTIONEDTABLEMODEL_H
