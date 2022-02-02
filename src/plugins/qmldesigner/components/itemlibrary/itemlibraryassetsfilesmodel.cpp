/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "itemlibraryassetsfilesmodel.h"

#include <QDebug>

namespace QmlDesigner {

ItemLibraryAssetsFilesModel::ItemLibraryAssetsFilesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // add roles
    m_roleNames.insert(FileNameRole, "fileName");
    m_roleNames.insert(FilePathRole, "filePath");
    m_roleNames.insert(FileDirRole, "fileDir");
}

QVariant ItemLibraryAssetsFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid index requested: " << QString::number(index.row());
        return {};
    }

    if (role == FileNameRole)
        return m_files[index.row()].split('/').last();

    if (role == FilePathRole)
        return m_files[index.row()];

    if (role == FileDirRole)
        return QVariant::fromValue(parent());

    qWarning() << Q_FUNC_INFO << "Invalid role requested: " << QString::number(role);
    return {};
}

int ItemLibraryAssetsFilesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_files.size();
}

QHash<int, QByteArray> ItemLibraryAssetsFilesModel::roleNames() const
{
    return m_roleNames;
}

void ItemLibraryAssetsFilesModel::addFile(const QString &filePath)
{
    m_files.append(filePath);
}

} // namespace QmlDesigner
