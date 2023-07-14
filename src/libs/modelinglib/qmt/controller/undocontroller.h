// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QUndoStack;
QT_END_NAMESPACE

namespace qmt {

class UndoCommand;

class QMT_EXPORT UndoController : public QObject
{
public:
    explicit UndoController(QObject *parent = nullptr);
    ~UndoController() override;

    QUndoStack *undoStack() const { return m_undoStack; }

    void push(UndoCommand *cmd);
    void beginMergeSequence(const QString &text);
    void endMergeSequence();

    void reset();
    void doNotMerge();

private:
    QUndoStack *m_undoStack = nullptr;
    bool m_doNotMerge = false;
};

} // namespace qmt
