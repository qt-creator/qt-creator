// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QAbstractTableModel>
#include <QList>
#include <QVariant>

namespace Git::Internal {

class RemoteModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit RemoteModel(QObject *parent = nullptr);

    void clear();
    bool refresh(const Utils::FilePath &workingDirectory, QString *errorMessage);

    QStringList allRemoteNames() const;
    QString remoteName(int row) const;
    QString remoteUrl(int row) const;

    bool removeRemote(int row);
    bool addRemote(const QString &name, const QString &url);
    bool renameRemote(const QString &oldName, const QString &newName);
    bool updateUrl(const QString &name, const QString &newUrl);

    // QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int remoteCount() const;

    Utils::FilePath workingDirectory() const;
    int findRemoteByName(const QString &name) const;

signals:
    void refreshed();

protected:
    class Remote {
    public:
        QString name;
        QString url;
    };
    using RemoteList = QList<Remote>;

private:
    const Qt::ItemFlags m_flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;

    Utils::FilePath m_workingDirectory;
    RemoteList m_remotes;
};

} // Git::Internal
