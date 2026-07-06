// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <shutdownguard.h>

#include <qtcassert.h>

#include <QThread>

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
            QTC_CHECK(QThread::isMainThread());
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

    To tie a single object's lifetime to the guard, prefer the GuardedObject
    wrapper over connecting to this object's \c destroyed() signal by hand.

    \sa Utils::GuardedObject
    \sa ExtensionSystem::IPlugin::aboutToShutdown()
*/

QObject *shutdownGuard()
{
    return theShutdownGuardHolder.shutdownGuard();
}

/*!
    \class Utils::GuardedObject
    \inmodule QtCreator
    \brief The GuardedObject class owns a heap-allocated object and deletes it
    when the shutdown guard is destroyed.

    This is the preferred way to hold a process-lifetime singleton, as it avoids
    a manual shutdownGuard() connection. Typically used as a function-local
    static:

    \code
    MyThing &myThing()
    {
        static GuardedObject<MyThing> theMyThing; // created on first use,
        return *theMyThing;                       // deleted at shutdown
    }
    \endcode

    Construct it from constructor arguments (as above) or from an existing
    object pointer (\c {GuardedObject<MyThing>{new MyThing(...)}}).

    \sa Utils::shutdownGuard()
*/

} // Utils
