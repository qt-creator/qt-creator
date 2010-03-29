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

#include "openpagesmodel.h"
#include "helpviewer.h"

#include <QtCore/QUrl>

using namespace Help::Internal;

OpenPagesModel::OpenPagesModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int OpenPagesModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 2;
}

int OpenPagesModel::rowCount(const QModelIndex &parent) const
{
    return  parent.isValid() ? 0 : m_pages.count();
}

QVariant OpenPagesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || role != Qt::DisplayRole)
        return QVariant();
    QString title = m_pages.at(index.row())->title();
    title.replace(QLatin1Char('&'), QLatin1String("&&"));
    return title.isEmpty() ? tr("(Untitled)") : title;
}

void OpenPagesModel::addPage(const QUrl &url, qreal zoom)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    HelpViewer *page = new HelpViewer(zoom);
    connect(page, SIGNAL(titleChanged()), this, SLOT(handleTitleChanged()));
    m_pages << page;
    endInsertRows();
    page->setSource(url);
}

void OpenPagesModel::removePage(int index)
{
    Q_ASSERT(index >= 0 && index < rowCount());
    beginRemoveRows(QModelIndex(), index, index);
    HelpViewer *page = m_pages.at(index);
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
