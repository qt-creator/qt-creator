// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contextmenuaction.h"

namespace qmt {

ContextMenuAction::ContextMenuAction(const QString &label, const QString &id, QObject *parent)
    : QAction(label, parent),
      m_id(id)
{
}

ContextMenuAction::ContextMenuAction(const QString &label, const QString &id, const QKeySequence &shortcut,
                                     QObject *parent)
    : QAction(label, parent),
      m_id(id)
{
    setShortcut(shortcut);
}

ContextMenuAction::~ContextMenuAction()
{
}

} // namespace qmt
