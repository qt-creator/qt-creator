/****************************************************************************
**
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

#include "deployconfigurationmodel.h"
#include "target.h"
#include "deployconfiguration.h"

using namespace ProjectExplorer;

/*!
    \class ProjectExplorer::DeployConfigurationModel

    \brief A model to represent the run configurations of a target.

    To be used in for the drop down of comboboxes.  Does automatically adjust
    itself to added and removed DeployConfigurations
*/

class DeployConfigurationComparer
{
public:
    bool operator()(DeployConfiguration *a, DeployConfiguration *b)
    {
        return a->displayName() < b->displayName();
    }
};

DeployConfigurationModel::DeployConfigurationModel(Target *target, QObject *parent)
    : QAbstractListModel(parent),
      m_target(target)
{
    m_deployConfigurations = m_target->deployConfigurations();
    qSort(m_deployConfigurations.begin(), m_deployConfigurations.end(), DeployConfigurationComparer());

    connect(target, SIGNAL(addedDeployConfiguration(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(addedDeployConfiguration(ProjectExplorer::DeployConfiguration*)));
    connect(target, SIGNAL(removedDeployConfiguration(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(removedDeployConfiguration(ProjectExplorer::DeployConfiguration*)));

    foreach (DeployConfiguration *dc, m_deployConfigurations)
        connect(dc, SIGNAL(displayNameChanged()),
                this, SLOT(displayNameChanged()));
}

int DeployConfigurationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_deployConfigurations.size();
}

int DeployConfigurationModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void DeployConfigurationModel::displayNameChanged()
{
    DeployConfiguration *dc = qobject_cast<DeployConfiguration *>(sender());
    if (!dc)
        return;

    DeployConfigurationComparer compare;
    // Find the old position
    int oldPos = m_deployConfigurations.indexOf(dc);

    if (oldPos >= 1 && compare(m_deployConfigurations.at(oldPos), m_deployConfigurations.at(oldPos - 1))) {
        // We need to move up
        int newPos = oldPos - 1;
        while (newPos >= 0 && compare(m_deployConfigurations.at(oldPos), m_deployConfigurations.at(newPos))) {
            --newPos;
        }
        ++newPos;

        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_deployConfigurations.insert(newPos, dc);
        m_deployConfigurations.removeAt(oldPos + 1);
        endMoveRows();
        // Not only did we move, we also changed...
        emit dataChanged(index(newPos, 0), index(newPos,0));
    } else if (oldPos < m_deployConfigurations.size() - 1
               && compare(m_deployConfigurations.at(oldPos + 1), m_deployConfigurations.at(oldPos))) {
        // We need to move down
        int newPos = oldPos + 1;
        while (newPos < m_deployConfigurations.size()
            && compare(m_deployConfigurations.at(newPos), m_deployConfigurations.at(oldPos))) {
            ++newPos;
        }
        beginMoveRows(QModelIndex(), oldPos, oldPos, QModelIndex(), newPos);
        m_deployConfigurations.insert(newPos, dc);
        m_deployConfigurations.removeAt(oldPos);
        endMoveRows();

        // We need to subtract one since removing at the old place moves the newIndex down
        emit dataChanged(index(newPos - 1, 0), index(newPos - 1, 0));
    } else {
        emit dataChanged(index(oldPos, 0), index(oldPos, 0));
    }
}

QVariant DeployConfigurationModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int row = index.row();
        if (row < m_deployConfigurations.size())
            return m_deployConfigurations.at(row)->displayName();
    }

    return QVariant();
}

DeployConfiguration *DeployConfigurationModel::deployConfigurationAt(int i)
{
    if (i > m_deployConfigurations.size() || i < 0)
        return 0;
    return m_deployConfigurations.at(i);
}

DeployConfiguration *DeployConfigurationModel::deployConfigurationFor(const QModelIndex &idx)
{
    if (idx.row() > m_deployConfigurations.size() || idx.row() < 0)
        return 0;
    return m_deployConfigurations.at(idx.row());
}

QModelIndex DeployConfigurationModel::indexFor(DeployConfiguration *rc)
{
    int idx = m_deployConfigurations.indexOf(rc);
    if (idx == -1)
        return QModelIndex();
    return index(idx, 0);
}

void DeployConfigurationModel::addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc)
{
    // Find the right place to insert
    DeployConfigurationComparer compare;
    int i = 0;
    for (; i < m_deployConfigurations.size(); ++i) {
        if (compare(dc, m_deployConfigurations.at(i)))
            break;
    }

    beginInsertRows(QModelIndex(), i, i);
    m_deployConfigurations.insert(i, dc);
    endInsertRows();

    connect(dc, SIGNAL(displayNameChanged()),
            this, SLOT(displayNameChanged()));
}

void DeployConfigurationModel::removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc)
{
    int i = m_deployConfigurations.indexOf(dc);
    if (i < 0)
        return;
    beginRemoveRows(QModelIndex(), i, i);
    m_deployConfigurations.removeAt(i);
    endRemoveRows();
}
