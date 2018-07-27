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

#include "androidextralibrarylistmodel.h"

#include <android/androidqtsupport.h>
#include <android/androidmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

namespace Android {

AndroidExtraLibraryListModel::AndroidExtraLibraryListModel(ProjectExplorer::Target *target,
                                                           QObject *parent)
    : QAbstractItemModel(parent),
      m_target(target)
{
    updateModel();

    connect(target->project(), &ProjectExplorer::Project::parsingStarted,
            this, &AndroidExtraLibraryListModel::updateModel);
    connect(target->project(), &ProjectExplorer::Project::parsingFinished,
            this, &AndroidExtraLibraryListModel::updateModel);
    connect(target, &ProjectExplorer::Target::activeRunConfigurationChanged,
            this, &AndroidExtraLibraryListModel::updateModel);
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
    const QString &entry = QDir::cleanPath(m_entries.at(index.row()));
    switch (role) {
    case Qt::DisplayRole: return entry;
    default: return QVariant();
    };
}

void AndroidExtraLibraryListModel::updateModel()
{
    AndroidQtSupport *qtSupport = Android::AndroidManager::androidQtSupport(m_target);
    QTC_ASSERT(qtSupport, return);

    if (qtSupport->parseInProgress(m_target)) {
        emit enabledChanged(false);
        return;
    }

    bool enabled;
    beginResetModel();
    if (qtSupport->validParse(m_target)) {
        m_entries = qtSupport->targetData(Constants::AndroidExtraLibs, m_target).toStringList();
        enabled = true;
    } else {
        // parsing error or not a application template
        m_entries.clear();
        enabled = false;
    }
    endResetModel();

    emit enabledChanged(enabled);
}

void AndroidExtraLibraryListModel::addEntries(const QStringList &list)
{
    AndroidQtSupport *qtSupport = Android::AndroidManager::androidQtSupport(m_target);
    QTC_ASSERT(qtSupport, return);
    Utils::FileName projectFilePath = qtSupport->projectFilePath(m_target);

    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size() + list.size());

    const QDir dir = qtSupport->projectFilePath(m_target).toFileInfo().absoluteDir();
    for (const QString &path : list)
        m_entries += "$$PWD/" + dir.relativeFilePath(path);

    qtSupport->setTargetData(Constants::AndroidExtraLibs, m_entries, m_target);
    endInsertRows();
}

bool greaterModelIndexByRow(const QModelIndex &a, const QModelIndex &b)
{
    return a.row() > b.row();
}

void AndroidExtraLibraryListModel::removeEntries(QModelIndexList list)
{
    if (list.isEmpty())
        return;

    std::sort(list.begin(), list.end(), greaterModelIndexByRow);

    int i = 0;
    while (i < list.size()) {
        int lastRow = list.at(i++).row();
        int firstRow = lastRow;
        while (i < list.size() && firstRow - list.at(i).row()  <= 1)
            firstRow = list.at(i++).row();

        beginRemoveRows(QModelIndex(), firstRow, lastRow);
        int count = lastRow - firstRow + 1;
        while (count-- > 0)
            m_entries.removeAt(firstRow);
        endRemoveRows();
    }

    AndroidQtSupport *qtSupport = AndroidManager::androidQtSupport(m_target);
    QTC_ASSERT(qtSupport, return);
    qtSupport->setTargetData(Constants::AndroidExtraLibs, m_entries, m_target);
}

} // Android
