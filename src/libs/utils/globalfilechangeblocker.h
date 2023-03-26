// Copyright (C) 2019 Orgad Shaneh <orgads@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "guard.h"

#include <QObject>

namespace Utils {

class QTCREATOR_UTILS_EXPORT GlobalFileChangeBlocker : public QObject
{
    Q_OBJECT

public:
    static GlobalFileChangeBlocker *instance();
    void forceBlocked(bool blocked);
    bool isBlocked() const { return m_blockedState; }

signals:
    void stateChanged(bool blocked);

private:
    GlobalFileChangeBlocker();
    void applicationStateChanged(Qt::ApplicationState state);

    Guard m_ignoreChanges;
    bool m_blockedState = false;
};

} // namespace Utils
