/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "sourcefileshandler.h"

#include "debuggerconstants.h"
#include "debuggerengine.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

#include <QtGui/QSortFilterProxyModel>

namespace Debugger {
namespace Internal {

SourceFilesHandler::SourceFilesHandler(DebuggerEngine *engine)
  : m_engine(engine)
{
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(this);
    m_proxyModel = proxy;
}

void SourceFilesHandler::clearModel()
{
    if (m_shortNames.isEmpty())
        return;
    m_shortNames.clear();
    m_fullNames.clear();
    reset();
}

QVariant SourceFilesHandler::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            tr("Internal name") + "        ",
            tr("Full name") + "        ",
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
    switch (role) {
        case EngineActionsEnabledRole:
            return m_engine->debuggerActionsEnabled();
    }

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

bool SourceFilesHandler::setData
    (const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    switch (role) {
        case RequestReloadSourceFilesRole:
            m_engine->reloadSourceFiles();
            return true;

        case RequestOpenFileRole:
            m_engine->openFile(value.toString());
            return true;
    }
    return false;
}

void SourceFilesHandler::setSourceFiles(const QMap<QString, QString> &sourceFiles)
{
    m_shortNames.clear();
    m_fullNames.clear();
    QMap<QString, QString>::ConstIterator it = sourceFiles.begin();
    QMap<QString, QString>::ConstIterator et = sourceFiles.end();
    for (; it != et; ++it) {
        m_shortNames.append(it.key());
        m_fullNames.append(it.value());
    }
    reset();
}

void SourceFilesHandler::removeAll()
{
    setSourceFiles(QMap<QString, QString>());
    //header()->setResizeMode(0, QHeaderView::ResizeToContents);
}

} // namespace Internal
} // namespace Debugger
