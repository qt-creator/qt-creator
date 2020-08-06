/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "commandbuilder.h"

namespace IncrediBuild {
namespace Internal {

namespace Constants {
const QLatin1String CUSTOMCOMMANDBUILDER_COMMAND("IncrediBuild.BuildConsole.%1.Command");
const QLatin1String CUSTOMCOMMANDBUILDER_ARGS("IncrediBuild.BuildConsole.%1.Arguments");
const QLatin1String CUSTOMCOMMANDBUILDER_ARGSSET("IncrediBuild.BuildConsole.%1.ArgumentsSet");
} // namespace Constants

void CommandBuilder::fromMap(const QVariantMap &map)
{
    m_command = map.value(QString(Constants::CUSTOMCOMMANDBUILDER_COMMAND).arg(id())).toString();
    m_args = map.value(QString(Constants::CUSTOMCOMMANDBUILDER_ARGS).arg(id())).toString();
}

void CommandBuilder::toMap(QVariantMap *map) const
{
    (*map)[QString(Constants::CUSTOMCOMMANDBUILDER_COMMAND).arg(id())] = QVariant(m_command);
    (*map)[QString(Constants::CUSTOMCOMMANDBUILDER_ARGS).arg(id())] = QVariant(m_args);
}

void CommandBuilder::setCommand(const QString &command)
{
    if (command == defaultCommand())
        m_command.clear();
    else
        m_command = command;
}

void CommandBuilder::setArguments(const QString &arguments)
{
    if (arguments == defaultArguments())
        m_args.clear();
    else
        m_args = arguments;
}

} // namespace Internal
} // namespace IncrediBuild
