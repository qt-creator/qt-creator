/****************************************************************************
**
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

#include "configtaskhandler.h"

#include "task.h"
#include "taskhub.h"

#include <coreplugin/icore.h>

#include <QAction>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ConfigTaskHandler::ConfigTaskHandler(const Task &pattern, Core::Id group, Core::Id page) :
    m_pattern(pattern),
    m_targetGroup(group),
    m_targetPage(page)
{ }

bool ConfigTaskHandler::canHandle(const ProjectExplorer::Task &task) const
{
    return task.description == m_pattern.description
            && task.category == m_pattern.category;
}

void ConfigTaskHandler::handle(const ProjectExplorer::Task &task)
{
    Q_UNUSED(task);
    Core::ICore::showOptionsDialog(m_targetGroup, m_targetPage);
}

QAction *ConfigTaskHandler::createAction(QObject *parent) const
{
    QAction *action = new QAction(Core::ICore::msgShowOptionsDialog(), parent);
    action->setToolTip(Core::ICore::msgShowOptionsDialogToolTip());
    return action;
}
