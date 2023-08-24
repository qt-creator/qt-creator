// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandbuilder.h"

#include "incredibuildtr.h"

using namespace Utils;

namespace IncrediBuild::Internal {

const char CUSTOMCOMMANDBUILDER_COMMAND[] = "IncrediBuild.BuildConsole.%1.Command";
const char CUSTOMCOMMANDBUILDER_ARGS[] = "IncrediBuild.BuildConsole.%1.Arguments";

static Key key(const QString &pattern, const QString &id)
{
    return keyFromString(pattern.arg(id));
}

QString CommandBuilder::displayName() const
{
    return Tr::tr("Custom Command");
}

void CommandBuilder::fromMap(const Store &map)
{
    m_command = FilePath::fromSettings(map.value(key(CUSTOMCOMMANDBUILDER_COMMAND, id())));
    m_args = map.value(key(CUSTOMCOMMANDBUILDER_ARGS, id())).toString();
}

void CommandBuilder::toMap(Store *map) const
{
    map->insert(key(CUSTOMCOMMANDBUILDER_COMMAND, id()), m_command.toSettings());
    map->insert(key(CUSTOMCOMMANDBUILDER_ARGS, id()), QVariant(m_args));
}

void CommandBuilder::setCommand(const FilePath &command)
{
    m_command = command;
}

void CommandBuilder::setArguments(const QString &arguments)
{
    if (arguments == defaultArguments())
        m_args.clear();
    else
        m_args = arguments;
}

} // IncrediBuild::Internal
