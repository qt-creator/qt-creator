/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sourcefileshandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/basetreeview.h>
#include <utils/savedaction.h>

#include <QDebug>
#include <QFileInfo>
#include <QMenu>
#include <QSortFilterProxyModel>

using namespace Utils;

namespace Debugger {
namespace Internal {

SourceFilesHandler::SourceFilesHandler(DebuggerEngine *engine)
    : m_engine(engine)
{
    setObjectName("SourceFilesModel");
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setObjectName("SourceFilesProxyModel");
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

bool SourceFilesHandler::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    if (role == BaseTreeView::ItemActivatedRole) {
        m_engine->gotoLocation(idx.data().toString());
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = data.value<ItemViewEvent>();
        if (ev.type() == QEvent::ContextMenu) {
            auto menu = new QMenu;
            QModelIndex index = idx.sibling(idx.row(), 0);
            QString name = index.data().toString();

            auto addAction = [this, menu](const QString &display, bool on, const std::function<void()> &onTriggered) {
                QAction *act = menu->addAction(display);
                act->setEnabled(on);
                QObject::connect(act, &QAction::triggered, onTriggered);
                return act;
            };

            addAction(tr("Reload Data"), m_engine->debuggerActionsEnabled(),
                      [this] { m_engine->reloadSourceFiles(); });

            if (name.isEmpty())
                addAction(tr("Open File"), false, {});
            else
                addAction(tr("Open File \"%1\"").arg(name), true,
                          [this, name] { m_engine->gotoLocation(name); });

            menu->addSeparator();
            menu->addAction(action(SettingsDialog));
            menu->popup(ev.globalPos());
            return true;
        }
    }

    return false;
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
    //header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

} // namespace Internal
} // namespace Debugger
