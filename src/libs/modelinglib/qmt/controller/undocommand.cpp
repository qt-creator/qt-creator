// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "undocommand.h"

#include "qmt/infrastructure/qmtassert.h"

namespace qmt {

UndoCommand::UndoCommand(const QString &text)
    : QUndoCommand(text)
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
    Q_UNUSED(other)

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
