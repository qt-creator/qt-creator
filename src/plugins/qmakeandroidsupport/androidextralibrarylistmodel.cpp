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
#include "qmakeandroidrunconfiguration.h"

#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <proparser/prowriter.h>


using namespace QmakeAndroidSupport;
using namespace Internal;
using QmakeProjectManager::QmakeProject;

AndroidExtraLibraryListModel::AndroidExtraLibraryListModel(ProjectExplorer::Target *target,
                                                           QObject *parent)
    : QAbstractItemModel(parent),
      m_target(target)
{

    activeRunConfigurationChanged();

    auto project = static_cast<QmakeProject *>(target->project());
    connect(project, &QmakeProject::proFileUpdated,
            this, &AndroidExtraLibraryListModel::proFileUpdated);

    connect(target, &ProjectExplorer::Target::activeRunConfigurationChanged,
            this, &AndroidExtraLibraryListModel::activeRunConfigurationChanged);
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

void AndroidExtraLibraryListModel::activeRunConfigurationChanged()
{
    const QmakeProjectManager::QmakeProFileNode *node = activeNode();
    if (!node || node->parseInProgress()) {
        emit enabledChanged(false);
        return;
    }

    m_scope = QLatin1String("contains(ANDROID_TARGET_ARCH,")
            + node->singleVariableValue(QmakeProjectManager::Variable::AndroidArch)
            + QLatin1Char(')');

    bool enabled;
    beginResetModel();
    if (node->validParse() && node->projectType() == QmakeProjectManager::ProjectType::ApplicationTemplate) {
        m_entries = node->variableValue(QmakeProjectManager::Variable::AndroidExtraLibs);
        enabled = true;
    } else {
        // parsing error or not a application template
        m_entries.clear();
        enabled = false;
    }
    endResetModel();

    emit enabledChanged(enabled);
}

QmakeProjectManager::QmakeProFileNode *AndroidExtraLibraryListModel::activeNode() const
{
    ProjectExplorer::RunConfiguration *rc = m_target->activeRunConfiguration();
    QmakeAndroidRunConfiguration *qarc = qobject_cast<QmakeAndroidRunConfiguration *>(rc);
    if (!qarc)
        return 0;
    auto project = static_cast<QmakeProject *>(m_target->project());
    return project->rootProjectNode()->findProFileFor(qarc->proFilePath());
}

void AndroidExtraLibraryListModel::proFileUpdated(QmakeProjectManager::QmakeProFileNode *node)
{
    if (node != activeNode())
        return;
    activeRunConfigurationChanged();
}

bool AndroidExtraLibraryListModel::isEnabled() const
{
    QmakeProjectManager::QmakeProFileNode *node = activeNode();
    if (!node)
        return false;
    if (node->parseInProgress())
        return false;
    if (node->projectType() != QmakeProjectManager::ProjectType::ApplicationTemplate)
        return false;
    return true;
}

void AndroidExtraLibraryListModel::addEntries(const QStringList &list)
{
    QmakeProjectManager::QmakeProFileNode *node = activeNode();
    if (!node || node->projectType() != QmakeProjectManager::ProjectType::ApplicationTemplate)
        return;

    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size() + list.size());

    foreach (const QString &path, list)
        m_entries +=  QLatin1String("$$PWD/")
                + node->filePath().toFileInfo().absoluteDir().relativeFilePath(path);

    node->setProVariable(QLatin1String("ANDROID_EXTRA_LIBS"), m_entries, m_scope,
                         QmakeProjectManager::Internal::ProWriter::ReplaceValues
                         | QmakeProjectManager::Internal::ProWriter::MultiLine);

    endInsertRows();
}

bool greaterModelIndexByRow(const QModelIndex &a, const QModelIndex &b)
{
    return a.row() > b.row();
}

void AndroidExtraLibraryListModel::removeEntries(QModelIndexList list)
{
    QmakeProjectManager::QmakeProFileNode *node = activeNode();
    if (list.isEmpty() || !node || node->projectType() != QmakeProjectManager::ProjectType::ApplicationTemplate)
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

    node->setProVariable(QLatin1String("ANDROID_EXTRA_LIBS"), m_entries, m_scope);
}
