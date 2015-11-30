/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
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

#include "command.h"

namespace Beautifier {
namespace Internal {

Command::Command()
    : m_processing(FileProcessing)
    , m_pipeAddsNewline(false)
    , m_returnsCRLF(false)
{
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
