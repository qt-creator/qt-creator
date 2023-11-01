// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotemodel.h"

#include "gitclient.h"
#include "gittr.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace Git::Internal {

RemoteModel::RemoteModel(QObject *parent) : QAbstractTableModel(parent)
{ }

QStringList RemoteModel::allRemoteNames() const
{
    return Utils::transform(m_remotes, &Remote::name);
}

QString RemoteModel::remoteName(int row) const
{
    return m_remotes.at(row).name;
}

QString RemoteModel::remoteUrl(int row) const
{
    return m_remotes.at(row).url;
}

bool RemoteModel::removeRemote(int row)
{
    QString output;
    QString error;
    bool success = gitClient().synchronousRemoteCmd(
                m_workingDirectory, {"rm", remoteName(row)}, &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

bool RemoteModel::addRemote(const QString &name, const QString &url)
{
    QString output;
    QString error;
    if (name.isEmpty() || url.isEmpty())
        return false;

    bool success = gitClient().synchronousRemoteCmd(
                m_workingDirectory, {"add", name, url}, &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

bool RemoteModel::renameRemote(const QString &oldName, const QString &newName)
{
    QString output;
    QString error;
    bool success = gitClient().synchronousRemoteCmd(
                m_workingDirectory, {"rename", oldName, newName}, &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

bool RemoteModel::updateUrl(const QString &name, const QString &newUrl)
{
    QString output;
    QString error;
    bool success = gitClient().synchronousRemoteCmd(
                m_workingDirectory, {"set-url", name, newUrl}, &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

FilePath RemoteModel::workingDirectory() const
{
    return m_workingDirectory;
}

int RemoteModel::remoteCount() const
{
    return m_remotes.size();
}

int RemoteModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return remoteCount();
}

int RemoteModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

QVariant RemoteModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if (index.column() == 0)
            return remoteName(row);
        else
            return remoteUrl(row);
    default:
        break;
    }
    return {};
}

QVariant RemoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};
    return (section == 0) ? Tr::tr("Name") : Tr::tr("URL");
}

bool RemoteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    const QString name = remoteName(index.row());
    const QString url = remoteUrl(index.row());
    switch (index.column()) {
    case 0:
        if (name == value.toString())
            return true;
        return renameRemote(name, value.toString());
    case 1:
        if (url == value.toString())
            return true;
        return updateUrl(name, value.toString());
    default:
        return false;
    }
}

Qt::ItemFlags RemoteModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return m_flags;
}

void RemoteModel::clear()
{
    if (m_remotes.isEmpty())
        return;
    beginResetModel();
    m_remotes.clear();
    endResetModel();
}

bool RemoteModel::refresh(const FilePath &workingDirectory, QString *errorMessage)
{
    m_workingDirectory = workingDirectory;

    // get list of remotes.
    QMap<QString,QString> remotesList
            = gitClient().synchronousRemotesList(workingDirectory, errorMessage);

    beginResetModel();
    m_remotes.clear();
    const QList<QString> remotes = remotesList.keys();
    for (const QString &remoteName : remotes) {
        Remote newRemote;
        newRemote.name = remoteName;
        newRemote.url = remotesList.value(remoteName);
        m_remotes.push_back(newRemote);
    }
    endResetModel();
    emit refreshed();
    return true;
}

int RemoteModel::findRemoteByName(const QString &name) const
{
    const int count = remoteCount();
    for (int i = 0; i < count; i++)
        if (remoteName(i) == name)
            return i;
    return -1;
}

} // Git::Internal

