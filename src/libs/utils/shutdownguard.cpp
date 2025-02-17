// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <shutdownguard.h>

#include <threadutils.h>
#include <qtcassert.h>

namespace Utils {

class ShutdownGuardHolder final
{
public:
    ~ShutdownGuardHolder()
    {
        if (!m_alreadyGone)
            triggerShutdownGuard();
    }

    QObject *shutdownGuard()
    {
        QTC_CHECK(!m_alreadyGone);
        if (!m_shutdownGuard) {
            QTC_CHECK(Utils::isMainThread());
            m_shutdownGuard = new QObject;
        }
        return m_shutdownGuard;
    }

    void triggerShutdownGuard()
    {
        m_alreadyGone = true;
        delete m_shutdownGuard;
        m_shutdownGuard = nullptr;
    }

private:
    QObject *m_shutdownGuard = nullptr;
    bool m_alreadyGone = false;
};

static ShutdownGuardHolder theShutdownGuardHolder;

/*!
    Destroys the shutdown guard object and consequently all
    objects guarded by it.

    In a normal run of the application this function is called
    automatically at the appropriate time.
*/

void triggerShutdownGuard()
{
    return theShutdownGuardHolder.triggerShutdownGuard();
}

/*!
    Returns an object that can be used as the parent for objects that should be
    destroyed just at the end of the applications lifetime.
    The object is destroyed after all plugins' aboutToShutdown methods
    have finished, just before the plugins are deleted.

    Only use this from the application's main thread.

    \sa ExtensionSystem::IPlugin::aboutToShutdown()
*/

QObject *shutdownGuard()
{
    return theShutdownGuardHolder.shutdownGuard();
}

} // Utils
