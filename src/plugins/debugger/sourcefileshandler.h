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

#ifndef DEBUGGER_SOURCEFILESHANDLER_H
#define DEBUGGER_SOURCEFILESHANDLER_H

#include <QAbstractItemModel>
#include <QStringList>

namespace Debugger {
namespace Internal {

class SourceFilesHandler : public QAbstractItemModel
{
    Q_OBJECT

public:
    SourceFilesHandler();

    int columnCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : 2; }
    int rowCount(const QModelIndex &parent) const
        { return parent.isValid() ? 0 : m_shortNames.size(); }
    QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    QModelIndex index(int row, int column, const QModelIndex &) const
        { return createIndex(row, column); }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void clearModel();

    void setSourceFiles(const QMap<QString, QString> &sourceFiles);
    void removeAll();

    QAbstractItemModel *model() { return m_proxyModel; }

private:
    QStringList m_shortNames;
    QStringList m_fullNames;
    QAbstractItemModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_SOURCEFILESHANDLER_H
