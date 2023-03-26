// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QUndoCommand>
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT UndoCommand : public QUndoCommand
{
public:
    explicit UndoCommand(const QString &text);
    ~UndoCommand() override;

    bool canRedo() const { return m_canRedo; }
    int id() const override;
    void setDoNotMerge(bool doNotMerge);

    bool mergeWith(const QUndoCommand *other) override;
    virtual bool mergeWith(const UndoCommand *other);

    void undo() override;
    void redo() override;

private:
    bool m_canRedo = false;
    bool m_doNotMerge = false;
};

} // namespace qmt
