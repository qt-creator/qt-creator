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

#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QVariant>

namespace Git {
namespace Internal {

class RemoteModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit RemoteModel(QObject *parent = 0);

    void clear();
    bool refresh(const QString &workingDirectory, QString *errorMessage);

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

    QString workingDirectory() const;
    int findRemoteByName(const QString &name) const;

protected:
    class Remote {
    public:
        QString name;
        QString url;
    };
    typedef QList<Remote> RemoteList;

private:
    const Qt::ItemFlags m_flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;

    QString m_workingDirectory;
    RemoteList m_remotes;
};

} // namespace Internal
} // namespace Git
