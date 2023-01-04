// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "undocontroller.h"

#include "undocommand.h"

#include <QUndoStack>

namespace qmt {

UndoController::UndoController(QObject *parent) :
    QObject(parent),
    m_undoStack(new QUndoStack(this))
{
}

UndoController::~UndoController()
{
}

void UndoController::push(UndoCommand *cmd)
{
    cmd->setDoNotMerge(m_doNotMerge);
    m_doNotMerge = false;
    m_undoStack->push(cmd);
}

void UndoController::beginMergeSequence(const QString &text)
{
    m_undoStack->beginMacro(text);
}

void UndoController::endMergeSequence()
{
    m_undoStack->endMacro();
}

void UndoController::reset()
{
    m_undoStack->clear();
}

void UndoController::doNotMerge()
{
    m_doNotMerge = true;
}

} // namespace qmt
