/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef REMOTEMODEL_H
#define REMOTEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QVariant>

namespace Git {
namespace Internal {

class GitClient;

class RemoteModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit RemoteModel(GitClient *client, QObject *parent = 0);

    void clear();
    bool refresh(const QString &workingDirectory, QString *errorMessage);

    QString remoteName(int row) const;
    QString remoteUrl(int row) const;

    bool removeRemote(int row);
    bool addRemote(const QString &name, const QString &url);
    bool renameRemote(const QString &oldName, const QString &newName);
    bool updateUrl(const QString &name, const QString &newUrl);

    // QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    int remoteCount() const;

    QString workingDirectory() const;
    int findRemoteByName(const QString &name) const;
    GitClient *client() const;

protected:
    struct Remote {
        bool parse(const QString &line);

        QString name;
        QString url;
    };
    typedef QList<Remote> RemoteList;

private:
    const Qt::ItemFlags m_flags;

    GitClient *m_client;
    QString m_workingDirectory;
    RemoteList m_remotes;
};

} // namespace Internal
} // namespace Git

#endif // REMOTEMODEL_H
