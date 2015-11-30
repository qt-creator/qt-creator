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

#include "runconfigurationmodel.h"
#include "target.h"
#include "runconfiguration.h"

#include <utils/algorithm.h>

using namespace ProjectExplorer;

/*!
    \class ProjectExplorer::RunConfigurationModel

    \brief The RunConfigurationModel class provides a model to represent the
    run configurations of a target.

    To be used in the dropdown lists of comboboxes.
    Automatically adjusts itself to added and removed run configurations.
*/

class RunConfigurationComparer
{
public:
    bool operator()(RunConfiguration *a, RunConfiguration *b)
    {
        return a->displayName() < b->displayName();
    }
};

RunConfigurationModel::RunConfigurationModel(Target *target, QObject *parent)
    : QAbstractListModel(parent),
      m_target(target)
{
    m_runConfigurations = m_target->runConfigurations();
    Utils::sort(m_runConfigurations, RunConfigurationComparer());

    connect(target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(addedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    connect(target, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(removedRunConfiguration(ProjectExplorer::RunConfiguration*)));

    foreach (RunConfiguration *rc, m_runConfigurations)
        connect(rc, SIGNAL(displayNameChanged()),
                this, SLOT(displayNameChanged()));
}

int RunConfigurationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_runConfigurations.size();
}

int RunConfigurationModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void RunConfigurationModel::displayNameChanged()
{
    RunConfiguration *rc = qobject_cast<RunConfiguration *>(sender());
    if (!rc)
        return;

    RunConfigurationComparer compare;
    // Find the old position
    int oldPos = m_runConfigurations.indexOf(rc);

    if (oldPos >= 1 && compare(m_runConfigurations.at(oldPos), m_runConfigurations.at(oldPos - 1))) {
        // We need to move up
        int newPos = oldPos - 1;
        while (newPos >= 0 && compare(m_runConfigurations.at(oldPos), m_runConfigurations.at(newPos))) {
            --newPos;
        }
        ++newPos;

        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_runConfigurations.insert(newPos, rc);
        m_runConfigurations.removeAt(oldPos + 1);
        endMoveRows();
        // Not only did we move, we also changed...
        emit dataChanged(index(newPos, 0), index(newPos,0));
    } else if  (oldPos < m_runConfigurations.size() - 1
                && compare(m_runConfigurations.at(oldPos + 1), m_runConfigurations.at(oldPos))) {
        // We need to move down
        int newPos = oldPos + 1;
        while (newPos < m_runConfigurations.size()
            && compare(m_runConfigurations.at(newPos), m_runConfigurations.at(oldPos))) {
            ++newPos;
        }
        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_runConfigurations.insert(newPos, rc);
        m_runConfigurations.removeAt(oldPos);
        endMoveRows();

        // We need to subtract one since removing at the old place moves the newIndex down
        emit dataChanged(index(newPos - 1, 0), index(newPos - 1, 0));
    } else {
        emit dataChanged(index(oldPos, 0), index(oldPos, 0));
    }
}

QVariant RunConfigurationModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int row = index.row();
        if (row < m_runConfigurations.size())
            return m_runConfigurations.at(row)->displayName();
    }

    return QVariant();
}

RunConfiguration *RunConfigurationModel::runConfigurationAt(int i)
{
    if (i > m_runConfigurations.size() || i < 0)
        return 0;
    return m_runConfigurations.at(i);
}

RunConfiguration *RunConfigurationModel::runConfigurationFor(const QModelIndex &idx)
{
    if (idx.row() > m_runConfigurations.size() || idx.row() < 0)
        return 0;
    return m_runConfigurations.at(idx.row());
}

QModelIndex RunConfigurationModel::indexFor(RunConfiguration *rc)
{
    int idx = m_runConfigurations.indexOf(rc);
    if (idx == -1)
        return QModelIndex();
    return index(idx, 0);
}

void RunConfigurationModel::addedRunConfiguration(RunConfiguration *rc)
{
    // Find the right place to insert
    RunConfigurationComparer compare;
    int i = 0;
    for (; i < m_runConfigurations.size(); ++i) {
        if (compare(rc, m_runConfigurations.at(i)))
            break;
    }

    beginInsertRows(QModelIndex(), i, i);
    m_runConfigurations.insert(i, rc);
    endInsertRows();


    connect(rc, SIGNAL(displayNameChanged()),
            this, SLOT(displayNameChanged()));
}

void RunConfigurationModel::removedRunConfiguration(RunConfiguration *rc)
{
    int i = m_runConfigurations.indexOf(rc);
    if (i < 0)
        return;

    beginRemoveRows(QModelIndex(), i, i);
    m_runConfigurations.removeAt(i);
    endRemoveRows();
}
