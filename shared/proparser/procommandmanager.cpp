/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "procommandmanager.h"

#include <utils/qtcassert.h>

using namespace Qt4ProjectManager::Internal;

ProCommandGroup::ProCommandGroup(const QString &name)
    : m_name(name)
{ }

ProCommandGroup::~ProCommandGroup()
{
    qDeleteAll(m_commands);
}

void ProCommandGroup::appendCommand(ProCommand *cmd)
{
    m_commands.append(cmd);
}

void ProCommandGroup::undo()
{
    for (int i = m_commands.count(); i > 0; --i)
        m_commands[i-1]->undo();
}

void ProCommandGroup::redo()
{
    for (int i = 0; i < m_commands.count(); ++i)
        m_commands[i]->redo();
}

ProCommandManager::ProCommandManager(QObject *parent)
    : QObject(parent)
{
    m_savepoint = 0;
    m_pos = 0;
    m_group = 0;
}

ProCommandManager::~ProCommandManager()
{
    qDeleteAll(m_groups);    
}

void ProCommandManager::beginGroup(const QString &name)
{
    QTC_ASSERT(!m_group, return);

    if (m_pos != m_groups.count()) {
        int removecount = m_groups.count() - m_pos;
        for (int i = 0; i < removecount; ++i)
            delete m_groups.takeLast();
        m_pos = m_groups.count();
    }
    
    m_group = new ProCommandGroup(name);
}

bool ProCommandManager::hasGroup() const
{
    return m_group != 0;
}

void ProCommandManager::endGroup()
{
    QTC_ASSERT(m_group, return);

    m_groups.append(m_group);
    m_pos = m_groups.count();
    m_group = 0;

    emit modified();
}

bool ProCommandManager::command(ProCommand *cmd)
{
    QTC_ASSERT(m_group, return false);

    if (cmd->redo()) {
        m_group->appendCommand(cmd);
        return true;
    }

    return false;
}

void ProCommandManager::undo()
{
    if (canUndo()) {
        --m_pos;
        m_groups[m_pos]->undo();
    }

    emit modified();
}

void ProCommandManager::redo()
{
    if (canRedo()) {
        m_groups[m_pos]->redo();
        ++m_pos;
    }

    emit modified();
}

bool ProCommandManager::isDirty() const
{
    if (m_groups.isEmpty())
        return false;

    if (m_pos != 0 && m_groups.at(m_pos - 1) == m_savepoint)
        return false;

    return true;
}

void ProCommandManager::notifySave()
{
    if (m_pos > 0)
        m_savepoint = m_groups.at(m_pos - 1);
}

bool ProCommandManager::canUndo() const
{
    return !m_groups.isEmpty() && m_pos > 0;
}

bool ProCommandManager::canRedo() const
{
    return m_groups.count() > m_pos;
}
