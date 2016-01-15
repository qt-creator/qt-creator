/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "undocommand.h"

#include "qmt/infrastructure/qmtassert.h"

namespace qmt {

UndoCommand::UndoCommand(const QString &text)
    : QUndoCommand(text),
      m_canRedo(false),
      m_doNotMerge(false)
{
}

UndoCommand::~UndoCommand()
{
}

int UndoCommand::id() const
{
    return 1;
}

void UndoCommand::setDoNotMerge(bool doNotMerge)
{
    m_doNotMerge = doNotMerge;
}

bool UndoCommand::mergeWith(const QUndoCommand *other)
{
    auto otherCommand = dynamic_cast<const UndoCommand *>(other);
    if (!otherCommand || otherCommand->m_doNotMerge)
        return false;
    return mergeWith(otherCommand);
}

bool UndoCommand::mergeWith(const UndoCommand *other)
{
    Q_UNUSED(other);

    return false;
}

void UndoCommand::undo()
{
    QMT_CHECK(!m_canRedo);
    m_canRedo = true;
}

void UndoCommand::redo()
{
    QMT_CHECK(m_canRedo);
    m_canRedo = false;
}

} // namespace qmt
