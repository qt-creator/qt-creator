// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "singleton.h"

#include <QThread>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Utils {
namespace Internal { class ProcessReaperPrivate; }

class QTCREATOR_UTILS_EXPORT ProcessReaper final
        : public SingletonWithOptionalDependencies<ProcessReaper>
{
public:
    static void reap(QProcess *process, int timeoutMs = 500);

private:
    ProcessReaper();
    ~ProcessReaper();

    QThread m_thread;
    Internal::ProcessReaperPrivate *m_private;
    friend class SingletonWithOptionalDependencies<ProcessReaper>;
};

} // namespace Utils
