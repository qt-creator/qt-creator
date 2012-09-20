/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

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
