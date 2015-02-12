/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotemodel.h"
#include "gitclient.h"

namespace Git {
namespace Internal {

// ------ RemoteModel
RemoteModel::RemoteModel(GitClient *client, QObject *parent) :
    QAbstractTableModel(parent),
    m_flags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable),
    m_client(client)
{ }

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
    bool success = m_client->synchronousRemoteCmd(m_workingDirectory,
                                                  QStringList() << QLatin1String("rm") << remoteName(row),
                                                  &output, &error);
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

    bool success = m_client->synchronousRemoteCmd(m_workingDirectory,
                                                  QStringList() << QLatin1String("add") << name << url,
                                                  &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

bool RemoteModel::renameRemote(const QString &oldName, const QString &newName)
{
    QString output;
    QString error;
    bool success = m_client->synchronousRemoteCmd(m_workingDirectory,
                                                  QStringList() << QLatin1String("rename") << oldName << newName,
                                                  &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

bool RemoteModel::updateUrl(const QString &name, const QString &newUrl)
{
    QString output;
    QString error;
    bool success = m_client->synchronousRemoteCmd(m_workingDirectory,
                                                  QStringList() << QLatin1String("set-url") << name << newUrl,
                                                  &output, &error);
    if (success)
        success = refresh(m_workingDirectory, &error);
    return success;
}

QString RemoteModel::workingDirectory() const
{
    return m_workingDirectory;
}

int RemoteModel::remoteCount() const
{
    return m_remotes.size();
}

int RemoteModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return remoteCount();
}

int RemoteModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
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
    return QVariant();
}

QVariant RemoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    return (section == 0) ? tr("Name") : tr("URL");
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
    Q_UNUSED(index);
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

bool RemoteModel::refresh(const QString &workingDirectory, QString *errorMessage)
{
    m_workingDirectory = workingDirectory;

    // get list of remotes.
    QMap<QString,QString> remotesList =
            m_client->synchronousRemotesList(workingDirectory, errorMessage);

    if (remotesList.isEmpty())
        return false;

    beginResetModel();
    m_remotes.clear();
    foreach (const QString &remoteName, remotesList.keys()) {
        Remote newRemote;
        newRemote.name = remoteName;
        newRemote.url = remotesList.value(remoteName);
        m_remotes.push_back(newRemote);
    }
    endResetModel();
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

GitClient *RemoteModel::client() const
{
    return m_client;
}

} // namespace Internal
} // namespace Git

