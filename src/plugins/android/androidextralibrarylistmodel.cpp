/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "androidextralibrarylistmodel.h"
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>

using namespace Android;
using namespace Internal;

AndroidExtraLibraryListModel::AndroidExtraLibraryListModel(Qt4ProjectManager::Qt4Project *project,
                                                           QObject *parent)
    : QAbstractItemModel(parent)
    , m_project(project)
{
    reset();

    connect(m_project, SIGNAL(proFilesEvaluated()), this, SLOT(reset()));
}

QModelIndex AndroidExtraLibraryListModel::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row, column);
}

QModelIndex AndroidExtraLibraryListModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int AndroidExtraLibraryListModel::rowCount(const QModelIndex &) const
{
    return m_entries.size();
}

int AndroidExtraLibraryListModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant AndroidExtraLibraryListModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.row() >= 0 && index.row() < m_entries.size());
    const QString &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole: return entry;
    default: return QVariant();
    };
}

void AndroidExtraLibraryListModel::reset()
{
    if (m_project->rootQt4ProjectNode()->projectType() != Qt4ProjectManager::ApplicationTemplate)
        return;

    beginResetModel();
    Qt4ProjectManager::Qt4ProFileNode *node = m_project->rootQt4ProjectNode();
    m_entries = node->variableValue(Qt4ProjectManager::AndroidExtraLibs);
    endResetModel();
}

void AndroidExtraLibraryListModel::addEntries(const QStringList &list)
{
    if (m_project->rootQt4ProjectNode()->projectType() != Qt4ProjectManager::ApplicationTemplate)
        return;

    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size() + list.size());

    foreach (QString path, list)
        m_entries += QDir(m_project->projectDirectory()).relativeFilePath(path);

    Qt4ProjectManager::Qt4ProFileNode *node = m_project->rootQt4ProjectNode();
    node->setProVariable(QLatin1String("ANDROID_EXTRA_LIBS"), m_entries.join(QLatin1Char(' ')));

    endInsertRows();
}

void AndroidExtraLibraryListModel::removeEntries(const QModelIndexList &list)
{
    if (list.isEmpty() || m_project->rootQt4ProjectNode()->projectType() != Qt4ProjectManager::ApplicationTemplate)
        return;

    QStringList oldList = m_entries;
    int i = 0;
    while (i < list.size()) {
        int firstRow = list.at(i++).row();
        int lastRow = firstRow;
        while (i < list.size() && list.at(i).row() - lastRow <= 1 && list.at(i).row() > firstRow)
            lastRow = list.at(i++).row();

        int first = m_entries.indexOf(oldList.at(firstRow));
        int count = lastRow - firstRow + 1;
        Q_ASSERT(count > 0);
        Q_ASSERT(oldList.at(lastRow) == m_entries.at(first + count - 1));

        beginRemoveRows(QModelIndex(), first, first + count - 1);
        while (count-- > 0)
            m_entries.removeAt(first);
        endRemoveRows();
    }

    Qt4ProjectManager::Qt4ProFileNode *node = m_project->rootQt4ProjectNode();
    node->setProVariable(QLatin1String("ANDROID_EXTRA_LIBS"), m_entries.join(QLatin1Char(' ')));
}
