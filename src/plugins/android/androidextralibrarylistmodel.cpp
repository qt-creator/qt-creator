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

#include "androidextralibrarylistmodel.h"
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <proparser/prowriter.h>

using namespace Android;
using namespace Internal;

AndroidExtraLibraryListModel::AndroidExtraLibraryListModel(QmakeProjectManager::QmakeProject *project,
                                                           QObject *parent)
    : QAbstractItemModel(parent)
    , m_project(project)
{
    QmakeProjectManager::QmakeProFileNode *node = m_project->rootQmakeProjectNode();
    proFileUpdated(node, node->validParse(), node->parseInProgress());

    connect(m_project, SIGNAL(proFileUpdated(QmakeProjectManager::QmakeProFileNode*,bool,bool)),
            this, SLOT(proFileUpdated(QmakeProjectManager::QmakeProFileNode*,bool,bool)));
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

void AndroidExtraLibraryListModel::proFileUpdated(QmakeProjectManager::QmakeProFileNode *node, bool success, bool parseInProgress)
{
    QmakeProjectManager::QmakeProFileNode *root = m_project->rootQmakeProjectNode();
    if (node != root)
        return;

    m_scope = QLatin1String("contains(ANDROID_TARGET_ARCH,")
            + node->singleVariableValue(QmakeProjectManager::AndroidArchVar)
            + QLatin1Char(')');

    if (parseInProgress) {
        emit enabledChanged(false);
        return;
    }

    bool enabled;
    beginResetModel();
    if (success && root->projectType() == QmakeProjectManager::ApplicationTemplate) {
        m_entries = node->variableValue(QmakeProjectManager::AndroidExtraLibs);
        enabled = true;
    } else {
        // parsing error or not a application template
        m_entries.clear();
        enabled = false;
    }
    endResetModel();

    emit enabledChanged(enabled);
}

bool AndroidExtraLibraryListModel::isEnabled() const
{
    QmakeProjectManager::QmakeProFileNode *root = m_project->rootQmakeProjectNode();
    if (root->parseInProgress())
        return false;
    if (root->projectType() != QmakeProjectManager::ApplicationTemplate)
        return false;
    return true;
}

void AndroidExtraLibraryListModel::addEntries(const QStringList &list)
{
    if (m_project->rootQmakeProjectNode()->projectType() != QmakeProjectManager::ApplicationTemplate)
        return;

    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size() + list.size());

    foreach (QString path, list)
        m_entries += QDir(m_project->projectDirectory()).relativeFilePath(path);

    QmakeProjectManager::QmakeProFileNode *node = m_project->rootQmakeProjectNode();
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
    if (list.isEmpty() || m_project->rootQmakeProjectNode()->projectType() != QmakeProjectManager::ApplicationTemplate)
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

    QmakeProjectManager::QmakeProFileNode *node = m_project->rootQmakeProjectNode();
    node->setProVariable(QLatin1String("ANDROID_EXTRA_LIBS"), m_entries, m_scope);
}
