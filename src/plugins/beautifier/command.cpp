/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "command.h"

namespace Beautifier {
namespace Internal {

bool Command::isValid() const
{
    return !m_executable.isEmpty();
}

QString Command::executable() const
{
    return m_executable;
}

void Command::setExecutable(const QString &executable)
{
    m_executable = executable;
}

QStringList Command::options() const
{
    return m_options;
}

void Command::addOption(const QString &option)
{
    m_options << option;
}

Command::Processing Command::processing() const
{
    return m_processing;
}

void Command::setProcessing(const Processing &processing)
{
    m_processing = processing;
}

bool Command::pipeAddsNewline() const
{
    return m_pipeAddsNewline;
}

void Command::setPipeAddsNewline(bool pipeAddsNewline)
{
    m_pipeAddsNewline = pipeAddsNewline;
}

bool Command::returnsCRLF() const
{
    return m_returnsCRLF;
}

void Command::setReturnsCRLF(bool returnsCRLF)
{
    m_returnsCRLF = returnsCRLF;
}

} // namespace Internal
} // namespace Beautifier
