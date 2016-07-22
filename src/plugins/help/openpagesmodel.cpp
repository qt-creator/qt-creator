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

#include "openpagesmodel.h"
#include "helpplugin.h"
#include "helpviewer.h"

#include <QUrl>

using namespace Help::Internal;

OpenPagesModel::OpenPagesModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int OpenPagesModel::rowCount(const QModelIndex &parent) const
{
    return  parent.isValid() ? 0 : m_pages.count();
}

int OpenPagesModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 2;
}

QVariant OpenPagesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()
        || index.column() >= columnCount() - 1)
        return QVariant();

    switch (role) {
        case Qt::ToolTipRole:
            return m_pages.at(index.row())->source().toString();
        case Qt::DisplayRole: {
            QString title = m_pages.at(index.row())->title();
            title.replace('&', "&&");
            return title.isEmpty() ? tr("(Untitled)") : title;
        }
        default:
            break;
    }
    return QVariant();
}

void OpenPagesModel::addPage(const QUrl &url, qreal zoom)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    HelpViewer *page = HelpPlugin::createHelpViewer(zoom);
    connect(page, &HelpViewer::titleChanged,
            this, &OpenPagesModel::handleTitleChanged);
    m_pages << page;
    endInsertRows();
    if (url.isValid())
        page->setSource(url);
}

void OpenPagesModel::removePage(int index)
{
    Q_ASSERT(index >= 0 && index < rowCount());
    beginRemoveRows(QModelIndex(), index, index);
    HelpViewer *page = m_pages.at(index);
    page->stop();
    m_pages.removeAt(index);
    endRemoveRows();
    page->deleteLater();
}

HelpViewer *OpenPagesModel::pageAt(int index) const
{
    Q_ASSERT(index >= 0 && index < rowCount());
    return m_pages.at(index);
}

void OpenPagesModel::handleTitleChanged()
{
    HelpViewer *page = static_cast<HelpViewer *>(sender());
    const int row = m_pages.indexOf(page);
    Q_ASSERT(row != -1 );
    const QModelIndex &item = index(row, 0);
    emit dataChanged(item, item);
}
