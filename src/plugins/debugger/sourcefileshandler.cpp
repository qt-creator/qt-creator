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

#include "sourcefileshandler.h"

#include <QDebug>
#include <QFileInfo>

#include <QSortFilterProxyModel>

namespace Debugger {
namespace Internal {

SourceFilesHandler::SourceFilesHandler()
{
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(this);
    m_proxyModel = proxy;
}

void SourceFilesHandler::clearModel()
{
    if (m_shortNames.isEmpty())
        return;
    beginResetModel();
    m_shortNames.clear();
    m_fullNames.clear();
    endResetModel();
}

QVariant SourceFilesHandler::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            tr("Internal Name") + QLatin1String("        "),
            tr("Full Name") + QLatin1String("        "),
        };
        return headers[section];
    }
    return QVariant();
}

Qt::ItemFlags SourceFilesHandler::flags(const QModelIndex &index) const
{
    if (index.row() >= m_fullNames.size())
        return 0;
    QFileInfo fi(m_fullNames.at(index.row()));
    return fi.isReadable() ? QAbstractItemModel::flags(index) : Qt::ItemFlags(0);
}

QVariant SourceFilesHandler::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_shortNames.size())
        return QVariant();

    switch (index.column()) {
        case 0:
            if (role == Qt::DisplayRole)
                return m_shortNames.at(row);
            // FIXME: add icons
            //if (role == Qt::DecorationRole)
            //    return module.symbolsRead ? icon2 : icon;
            break;
        case 1:
            if (role == Qt::DisplayRole)
                return m_fullNames.at(row);
            //if (role == Qt::DecorationRole)
            //    return module.symbolsRead ? icon2 : icon;
            break;
    }
    return QVariant();
}

void SourceFilesHandler::setSourceFiles(const QMap<QString, QString> &sourceFiles)
{
    beginResetModel();
    m_shortNames.clear();
    m_fullNames.clear();
    QMap<QString, QString>::ConstIterator it = sourceFiles.begin();
    QMap<QString, QString>::ConstIterator et = sourceFiles.end();
    for (; it != et; ++it) {
        m_shortNames.append(it.key());
        m_fullNames.append(it.value());
    }
    endResetModel();
}

void SourceFilesHandler::removeAll()
{
    setSourceFiles(QMap<QString, QString>());
    //header()->setResizeMode(0, QHeaderView::ResizeToContents);
}

} // namespace Internal
} // namespace Debugger
