// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt_global.h"
#include <QAction>

namespace qmt {

class QMT_EXPORT ContextMenuAction : public QAction
{
public:
    ContextMenuAction(const QString &label, const QString &id, QObject *parent = nullptr);
    ContextMenuAction(const QString &label, const QString &id, const QKeySequence &shortcut,
                      QObject *parent = nullptr);
    ~ContextMenuAction() override;

    QString id() const { return m_id; }

private:
    QString m_id;
};

} // namespace qmt
