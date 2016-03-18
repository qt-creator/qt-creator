/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include <QAbstractItemModel>
#include <QStringList>

namespace ProjectExplorer {
class RunConfiguration;
class Target;
}

namespace QmakeProjectManager { class QmakeProFileNode; }

namespace QmakeAndroidSupport {

namespace Internal {
class AndroidExtraLibraryListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AndroidExtraLibraryListModel(ProjectExplorer::Target *target,
                                          QObject *parent = 0);

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    void removeEntries(QModelIndexList list);
    void addEntries(const QStringList &list);

    bool isEnabled() const;

signals:
    void enabledChanged(bool);

private:
    void proFileUpdated(QmakeProjectManager::QmakeProFileNode *node);
    void activeRunConfigurationChanged();
    QmakeProjectManager::QmakeProFileNode *activeNode() const;

    ProjectExplorer::Target *m_target;
    QStringList m_entries;
    QString m_scope;
};

} // namespace Internal
} // namespace QmakeAndroidSupport
