// Copyright (C) 2019 Orgad Shaneh <orgads@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globalfilechangeblocker.h"

#include <QGuiApplication>

namespace Utils {

GlobalFileChangeBlocker::GlobalFileChangeBlocker()
{
    m_blockedState = QGuiApplication::applicationState() != Qt::ApplicationActive;
    connect(qApp, &QGuiApplication::applicationStateChanged,
            this, &GlobalFileChangeBlocker::applicationStateChanged);
}

GlobalFileChangeBlocker *GlobalFileChangeBlocker::instance()
{
    static GlobalFileChangeBlocker blocker;
    return &blocker;
}

void GlobalFileChangeBlocker::forceBlocked(bool blocked)
{
    blocked ? m_ignoreChanges.lock() : m_ignoreChanges.unlock();
    applicationStateChanged(QGuiApplication::applicationState());
}

void GlobalFileChangeBlocker::applicationStateChanged(Qt::ApplicationState state)
{
    const bool blocked = m_ignoreChanges.isLocked() || (state != Qt::ApplicationActive);
    if (blocked != m_blockedState) {
        emit stateChanged(blocked);
        m_blockedState = blocked;
    }
}

} // namespace Utils
