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
const QLatin1String CUSTOMCOMMANDBUILDER_COMMAND("IncrediBuild.BuildConsole.CustomCommandBuilder.Command");
const QLatin1String CUSTOMCOMMANDBUILDER_ARGS("IncrediBuild.BuildConsole.CustomCommandBuilder.Arguments");
const QLatin1String CUSTOMCOMMANDBUILDER_ARGSSET("IncrediBuild.BuildConsole.CustomCommandBuilder.ArgumentsSet");
} // namespace Constants

QString CommandBuilder::fullCommandFlag()
{
    QString argsLine;
    for (const QString &a : arguments())
        argsLine += "\"" + a + "\" ";

    if (!keepJobNum())
        argsLine = setMultiProcessArg(argsLine);

    QString fullCommand("\"%0\" %1");
    fullCommand = fullCommand.arg(command(), argsLine);

    return fullCommand;
}

void CommandBuilder::reset()
{
    m_command.clear();
    m_args.clear();
    m_argsSet = false;
}

bool CommandBuilder::fromMap(const QVariantMap &map)
{
    m_command = map.value(Constants::CUSTOMCOMMANDBUILDER_COMMAND, QVariant(QString())).toString();
    m_argsSet = map.value(Constants::CUSTOMCOMMANDBUILDER_ARGSSET, QVariant(false)).toBool();
    if (m_argsSet)
        arguments(map.value(Constants::CUSTOMCOMMANDBUILDER_ARGS, QVariant(QString())).toString());

    // Not loading m_keepJobNum as it is managed by the build step.
    return true;
}

void CommandBuilder::toMap(QVariantMap *map) const
{
    (*map)[Constants::CUSTOMCOMMANDBUILDER_COMMAND] = QVariant(m_command);
    (*map)[Constants::CUSTOMCOMMANDBUILDER_ARGSSET] = QVariant(m_argsSet);
    if (m_argsSet)
        (*map)[Constants::CUSTOMCOMMANDBUILDER_ARGS] = QVariant(m_args);

    // Not saving m_keepJobNum as it is managed by the build step.
}

// Split args to quoted or spaced parts
void CommandBuilder::arguments(const QString &argsLine)
{
    QStringList args;
    QString currArg;
    bool inQuote = false;
    bool inArg = false;
    for (int i = 0; i < argsLine.length(); ++i) {
        QChar c = argsLine[i];
        if (c.isSpace()) {
            if (!inArg) // Spaces between arguments
                continue;

            if (!inQuote) { // Arg termination
                if (currArg.length() > 0) {
                    args.append(currArg);
                    currArg.clear();
                }
                inArg = false;
                continue;
            } else { // Space within argument
                currArg += c;
                continue;
            }
        }

        inArg = true;
        if (c == '"') {
            if ((i < (argsLine.length() - 1))
                && (argsLine[i + 1] == '"')) { // Convert '""' to double-quote within arg
                currArg += '"';
                ++i;
                continue;
            }

            if (inQuote) { // Arg termination
                if (currArg.length() > 0) {
                    args.append(currArg);
                    currArg.clear();
                }
                inArg = false;
                inQuote = false;
                continue;
            }

            // Arg start
            inArg = true;
            inQuote = true;
            continue;
        }

        // Simple character
        inArg = true;
        currArg += c;
    }

    if (currArg.length() > 0)
        args.append(currArg);

    arguments(args);
}

} // namespace Internal
} // namespace IncrediBuild
