/**************************************************************************
**
** Copyright (c) 2014 Lorenz Haas
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

#include "command.h"

namespace Beautifier {
namespace Internal {

Command::Command()
    : m_processing(FileProcessing)
    , m_pipeAddsNewline(false)
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

} // namespace Internal
} // namespace Beautifier
