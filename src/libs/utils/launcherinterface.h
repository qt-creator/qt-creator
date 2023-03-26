// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "processreaper.h"
#include "singleton.h"

#include <QThread>

namespace Utils {
namespace Internal {
class CallerHandle;
class LauncherHandle;
class LauncherInterfacePrivate;
class ProcessLauncherImpl;
}

class QTCREATOR_UTILS_EXPORT LauncherInterface final
        : public SingletonWithOptionalDependencies<LauncherInterface, ProcessReaper>
{
public:
    static void setPathToLauncher(const QString &pathToLauncher);

private:
    friend class Internal::CallerHandle;
    friend class Internal::LauncherHandle;
    friend class Internal::ProcessLauncherImpl;

    static bool isStarted();
    static void sendData(const QByteArray &data);
    static Internal::CallerHandle *registerHandle(QObject *parent, quintptr token);
    static void unregisterHandle(quintptr token);

    LauncherInterface();
    ~LauncherInterface();

    QThread m_thread;
    Internal::LauncherInterfacePrivate *m_private;
    friend class SingletonWithOptionalDependencies<LauncherInterface, ProcessReaper>;
};

} // namespace Utils
