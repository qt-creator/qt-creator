/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iplugin.h"
#include "iplugin_p.h"
#include "pluginmanager.h"
#include "pluginspec.h"

/*!
    \class ExtensionSystem::IPlugin
    \mainclass
    \brief The IPlugin class is the base class for all plugins.

    The IPlugin class is an abstract class that must be implemented
    once for each plugin.
    A plugin consists of two parts: A description file, and a library
    that at least contains the IPlugin implementation.

    \tableofcontents

    \section1 Plugin Specification

    A plugin needs to provide a plugin specification file in addition
    to the actual plugin library, so the plugin manager can find the plugin,
    resolve its dependencies, and load it. For more information,
    see \l{Plugin Specifications}.

    \section1 Plugin Implementation
    Plugins must provide one implementation of the IPlugin class, located
    in a library that matches the \c name attribute given in their
    XML description. The IPlugin implementation must be exported and
    made known to Qt's plugin system, using the \c Q_PLUGIN_METADATA macro with
    an IID set to \c "org.qt-project.Qt.QtCreatorPlugin".


    After the plugins' XML files have been read, and dependencies have been
    found, the plugin loading is done in three phases:
    \list 1
    \li All plugin libraries are loaded in \e{root-to-leaf} order of the
       dependency tree.
    \li All plugins' initialize functions are called in \e{root-to-leaf} order
       of the dependency tree. This is a good place to put
       objects in the plugin manager's object pool.
    \li All plugins' extensionsInitialized functions are called in \e{leaf-to-root}
       order of the dependency tree. At this point, plugins can
       be sure that all plugins that depend on this plugin have
       been initialized completely (implying that they have put
       objects in the object pool, if they want that during the
       initialization sequence).
    \endlist
    If library loading or initialization of a plugin fails, all plugins
    that depend on that plugin also fail.

    Plugins have access to the plugin manager
    (and its object pool) via the PluginManager::instance()
    function.
*/

/*!
    \fn bool IPlugin::initialize(const QStringList &arguments, QString *errorString)
    \brief Called after the plugin has been loaded and the IPlugin instance
    has been created.

    The initialize functions of plugins that depend
    on this plugin are called after the initialize function of this plugin
    has been called. Plugins should initialize their internal state in this
    function. Returns if initialization of successful. If it wasn't successful,
    the \a errorString should be set to a user-readable message
    describing the reason.

    \sa extensionsInitialized()
    \sa delayedInitialize()
*/

/*!
    \fn void IPlugin::extensionsInitialized()
    \brief Called after the IPlugin::initialize() function has been called,
    and after both the IPlugin::initialize() and IPlugin::extensionsInitialized()
    functions of plugins that depend on this plugin have been called.

    In this function, the plugin can assume that plugins that depend on
    this plugin are fully 'up and running'. It is a good place to
    look in the plugin manager's object pool for objects that have
    been provided by dependent plugins.

    \sa initialize()
    \sa delayedInitialize()
*/

/*!
    \fn bool IPlugin::delayedInitialize()
    \brief Called after all plugins' IPlugin::extensionsInitialized() function has been called,
    and after the IPlugin::delayedInitialize() function of plugins that depend on this plugin
    have been called.

    The plugins' delayedInitialize() functions are called after the application is already running,
    with a few milliseconds delay to application startup, and between individual delayedInitialize
    function calls. To avoid unnecessary delays, a plugin should return true from the function if it
    actually implements it, to indicate that the next plugins' delayedInitialize() call should
    be delayed a few milliseconds to give input and paint events a chance to be processed.

    This function can be used if a plugin needs to do non-trivial setup that doesn't
    necessarily needs to be done directly at startup, but still should be done within a
    short time afterwards. This can increase the felt plugin/application startup
    time a lot, with very little effort.

    \sa initialize()
    \sa extensionsInitialized()
*/

/*!
    \fn IPlugin::ShutdownFlag IPlugin::aboutToShutdown()
    \brief Called during a shutdown sequence in the same order as initialization
    before the plugins get deleted in reverse order.

    This function should be used to disconnect from other plugins,
    hide all UI, and optimize shutdown in general.
    If a plugin needs to delay the real shutdown for a while, for example if
    it needs to wait for external processes to finish for a clean shutdown,
    the plugin can return IPlugin::AsynchronousShutdown from this function. This
    will keep the main event loop running after the aboutToShutdown() sequence
    has finished, until all plugins requesting AsynchronousShutdown have sent
    the asynchronousShutdownFinished() signal.

    The default implementation of this function does nothing and returns
    IPlugin::SynchronousShutdown.

    Returns IPlugin::AsynchronousShutdown if the plugin needs to perform
    asynchronous actions before performing the shutdown.

    \sa asynchronousShutdownFinished()
*/

/*!
    \fn QObject *IPlugin::remoteCommand(const QStringList &options, const QStringList &arguments)
    \brief When \QC is executed with the -client argument while already another instance of \QC
           is running, this function of plugins is called in the running instance.

    Plugin-specific arguments are passed in \a options, while the rest of the
    arguments are passed in \a arguments.

    \returns a QObject that blocks the command until it is destroyed, if -block is used.

    \sa PluginManager::serializedArguments()
*/

/*!
    \fn void IPlugin::asynchronousShutdownFinished()
    Sent by the plugin implementation after a asynchronous shutdown
    is ready to proceed with the shutdown sequence.

    \sa aboutToShutdown()
*/

using namespace ExtensionSystem;

/*!
    \fn IPlugin::IPlugin()
    \internal
*/
IPlugin::IPlugin()
    : d(new Internal::IPluginPrivate())
{
}

/*!
    \fn IPlugin::~IPlugin()
    \internal
*/
IPlugin::~IPlugin()
{
    foreach (QObject *obj, d->addedObjectsInReverseOrder)
        PluginManager::removeObject(obj);
    qDeleteAll(d->addedObjectsInReverseOrder);
    d->addedObjectsInReverseOrder.clear();
    delete d;
    d = 0;
}

/*!
    \fn QList<QObject *> IPlugin::createTestObjects() const

    Returns objects that are meant to be passed on to QTest::qExec().

    This function will be called if the user starts \QC with '-test PluginName' or '-test all'.

    The ownership of returned objects is transferred to caller.
*/
QList<QObject *> IPlugin::createTestObjects() const
{
    return QList<QObject *>();
}

/*!
    \fn PluginSpec *IPlugin::pluginSpec() const
    Returns the PluginSpec corresponding to this plugin.
    This is not available in the constructor.
*/
PluginSpec *IPlugin::pluginSpec() const
{
    return d->pluginSpec;
}

/*!
    \fn void IPlugin::addObject(QObject *obj)
    Convenience function that registers \a obj in the plugin manager's
    plugin pool by just calling PluginManager::addObject().
*/
void IPlugin::addObject(QObject *obj)
{
    PluginManager::addObject(obj);
}

/*!
    \fn void IPlugin::addAutoReleasedObject(QObject *obj)
    Convenience function for registering \a obj in the plugin manager's
    plugin pool. Usually, registered objects must be removed from
    the object pool and deleted by hand.
    Objects added to the pool via addAutoReleasedObject are automatically
    removed and deleted in reverse order of registration when
    the IPlugin instance is destroyed.
    \sa PluginManager::addObject()
*/
void IPlugin::addAutoReleasedObject(QObject *obj)
{
    d->addedObjectsInReverseOrder.prepend(obj);
    PluginManager::addObject(obj);
}

/*!
    \fn void IPlugin::removeObject(QObject *obj)
    Convenience function that unregisters \a obj from the plugin manager's
    plugin pool by just calling PluginManager::removeObject().
*/
void IPlugin::removeObject(QObject *obj)
{
    PluginManager::removeObject(obj);
}

