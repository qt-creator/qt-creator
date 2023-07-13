// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourcefileshandler.h"

#include "debuggeractions.h"
#include "debuggerengine.h"
#include "debuggertr.h"

#include <utils/basetreeview.h>

#include <QDebug>
#include <QFileInfo>
#include <QMenu>
#include <QSortFilterProxyModel>

using namespace Utils;

namespace Debugger::Internal {

SourceFilesHandler::SourceFilesHandler(DebuggerEngine *engine)
    : m_engine(engine)
{
    setObjectName("SourceFilesModel");
    auto proxy = new QSortFilterProxyModel(this);
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
            Tr::tr("Internal Name") + "        ",
            Tr::tr("Full Name") + "        ",
        };
        return headers[section];
    }
    return QVariant();
}

Qt::ItemFlags SourceFilesHandler::flags(const QModelIndex &index) const
{
    if (index.row() >= m_fullNames.size())
        return {};
    FilePath filePath = m_fullNames.at(index.row());
    return filePath.isReadableFile() ? QAbstractItemModel::flags(index) : Qt::ItemFlags({});
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
                return m_fullNames.at(row).toUserOutput();
            //if (role == Qt::DecorationRole)
            //    return module.symbolsRead ? icon2 : icon;
            break;
    }
    return QVariant();
}

bool SourceFilesHandler::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    if (role == BaseTreeView::ItemActivatedRole) {
        m_engine->gotoLocation(FilePath::fromString(idx.data().toString()));
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = data.value<ItemViewEvent>();
        if (ev.type() == QEvent::ContextMenu) {
            auto menu = new QMenu;
            QModelIndex index = idx.sibling(idx.row(), 0);
            QString name = index.data().toString();

            auto addAction =
                [menu](const QString &display, bool on, const std::function<void()> &onTriggered) {
                    QAction *act = menu->addAction(display);
                    act->setEnabled(on);
                    QObject::connect(act, &QAction::triggered, onTriggered);
                    return act;
                };

            addAction(Tr::tr("Reload Data"), m_engine->debuggerActionsEnabled(),
                      [this] { m_engine->reloadSourceFiles(); });

            if (name.isEmpty())
                addAction(Tr::tr("Open File"), false, {});
            else
                addAction(Tr::tr("Open File \"%1\"").arg(name), true,
                          [this, name] { m_engine->gotoLocation(FilePath::fromString(name)); });

            menu->addAction(settings().settingsDialog.action());
            menu->popup(ev.globalPos());
            return true;
        }
    }

    return false;
}

void SourceFilesHandler::setSourceFiles(const QMap<QString, FilePath> &sourceFiles)
{
    beginResetModel();
    m_shortNames.clear();
    m_fullNames.clear();
    auto it = sourceFiles.begin();
    const auto et = sourceFiles.end();
    for (; it != et; ++it) {
        m_shortNames.append(it.key());
        m_fullNames.append(it.value());
    }
    endResetModel();
}

void SourceFilesHandler::removeAll()
{
    setSourceFiles({});
    //header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

} // Debugger::Internal
