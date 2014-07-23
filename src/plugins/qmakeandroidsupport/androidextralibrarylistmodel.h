/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDEXTRALIBRARYLISTMODEL_H
#define ANDROIDEXTRALIBRARYLISTMODEL_H

#include <QAbstractItemModel>
#include <QStringList>

namespace QmakeProjectManager {
class QmakeProject;
class QmakeProFileNode;
}

namespace QmakeAndroidSupport {

namespace Internal {
class AndroidExtraLibraryListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AndroidExtraLibraryListModel(QmakeProjectManager::QmakeProject *project,
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

private slots:
    void proFileUpdated(QmakeProjectManager::QmakeProFileNode *node, bool success, bool parseInProgress);

private:
    QmakeProjectManager::QmakeProject *m_project;
    QStringList m_entries;
    QString m_scope;
};

} // namespace Internal
} // namespace QmakeAndroidSupport

#endif // ANDROIDEXTRALIBRARYLISTMODEL_H
