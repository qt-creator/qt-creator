// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandbuilder.h"

#include "incredibuildtr.h"

using namespace Utils;

namespace IncrediBuild::Internal {

const char CUSTOMCOMMANDBUILDER_COMMAND[] = "IncrediBuild.BuildConsole.%1.Command";
const char CUSTOMCOMMANDBUILDER_ARGS[] = "IncrediBuild.BuildConsole.%1.Arguments";

QString CommandBuilder::displayName() const
{
    return Tr::tr("Custom Command");
}

void CommandBuilder::fromMap(const QVariantMap &map)
{
    m_command = FilePath::fromSettings(map.value(QString(CUSTOMCOMMANDBUILDER_COMMAND).arg(id())));
    m_args = map.value(QString(CUSTOMCOMMANDBUILDER_ARGS).arg(id())).toString();
}

void CommandBuilder::toMap(QVariantMap *map) const
{
    (*map)[QString(CUSTOMCOMMANDBUILDER_COMMAND).arg(id())] = m_command.toSettings();
    (*map)[QString(CUSTOMCOMMANDBUILDER_ARGS).arg(id())] = QVariant(m_args);
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
