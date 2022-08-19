// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "assetslibraryfilesmodel.h"

#include <QDebug>

namespace QmlDesigner {

AssetsLibraryFilesModel::AssetsLibraryFilesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // add roles
    m_roleNames.insert(FileNameRole, "fileName");
    m_roleNames.insert(FilePathRole, "filePath");
    m_roleNames.insert(FileDirRole, "fileDir");
}

QVariant AssetsLibraryFilesModel::data(const QModelIndex &index, int role) const
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

int AssetsLibraryFilesModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_files.size();
}

QHash<int, QByteArray> AssetsLibraryFilesModel::roleNames() const
{
    return m_roleNames;
}

void AssetsLibraryFilesModel::addFile(const QString &filePath)
{
    m_files.append(filePath);
}

} // namespace QmlDesigner
