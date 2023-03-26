// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "configtaskhandler.h"

#include <coreplugin/icore.h>

#include <QAction>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ConfigTaskHandler::ConfigTaskHandler(const Task &pattern, Utils::Id page) :
    m_pattern(pattern),
    m_targetPage(page)
{ }

bool ConfigTaskHandler::canHandle(const Task &task) const
{
    return task.description() == m_pattern.description()
            && task.category == m_pattern.category;
}

void ConfigTaskHandler::handle(const Task &task)
{
    Q_UNUSED(task)
    Core::ICore::showOptionsDialog(m_targetPage);
}

QAction *ConfigTaskHandler::createAction(QObject *parent) const
{
    auto action = new QAction(Core::ICore::msgShowOptionsDialog(), parent);
    action->setToolTip(Core::ICore::msgShowOptionsDialogToolTip());
    return action;
}
