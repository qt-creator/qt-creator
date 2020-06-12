/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "iplugin.h"
#include "iplugin_p.h"
#include "pluginmanager.h"
#include "pluginspec.h"

/*!
    \class ExtensionSystem::IPlugin
    \inheaderfile extensionsystem/iplugin.h
    \inmodule QtCreator
    \ingroup mainclasses

    \brief The IPlugin class is an abstract base class that must be implemented
    once for each plugin.

    A plugin needs to provide meta data in addition to the actual plugin
    library, so the plugin manager can find the plugin, resolve its
    dependencies, and load it. For more information, see \l{Plugin Meta Data}.

    Plugins must provide one implementation of the IPlugin class, located
    in a library that matches the \c name attribute given in their
    meta data. The IPlugin implementation must be exported and
    made known to Qt's plugin system, using the \c Q_PLUGIN_METADATA macro with
    an IID set to \c "org.qt-project.Qt.QtCreatorPlugin".

    For more information, see \l{Plugin Life Cycle}.
*/

/*!
    \enum IPlugin::ShutdownFlag

    This enum type holds whether the plugin is shut down synchronously or
    asynchronously.

    \value SynchronousShutdown
           The plugin is shut down synchronously.
    \value AsynchronousShutdown
           The plugin needs to perform asynchronous
           actions before it shuts down.
*/

/*!
    \fn bool ExtensionSystem::IPlugin::initialize(const QStringList &arguments, QString *errorString)
    Called after the plugin has been loaded and the IPlugin instance
    has been created.

    The initialize functions of plugins that depend on this plugin are called
    after the initialize function of this plugin has been called with
    \a arguments. Plugins should initialize their internal state in this
    function.

    Returns whether initialization succeeds. If it does not, \a errorString
    should be set to a user-readable message describing the reason.

    \sa extensionsInitialized()
    \sa delayedInitialize()
*/

/*!
    \fn void ExtensionSystem::IPlugin::extensionsInitialized()
    Called after the initialize() function has been called,
    and after both the initialize() and \c extensionsInitialized()
    functions of plugins that depend on this plugin have been called.

    In this function, the plugin can assume that plugins that depend on
    this plugin are fully \e {up and running}. It is a good place to
    look in the global object pool for objects that have been provided
    by weakly dependent plugins.

    \sa initialize()
    \sa delayedInitialize()
*/

/*!
    \fn bool ExtensionSystem::IPlugin::delayedInitialize()
    Called after all plugins' extensionsInitialized() function has been called,
    and after the \c delayedInitialize() function of plugins that depend on this
    plugin have been called.

    The plugins' \c delayedInitialize() functions are called after the
    application is already running, with a few milliseconds delay to
    application startup, and between individual \c delayedInitialize()
    function calls. To avoid unnecessary delays, a plugin should return
    \c true from the function if it actually implements it, to indicate
    that the next plugins' \c delayedInitialize() call should be delayed
    a few milliseconds to give input and paint events a chance to be processed.

    This function can be used if a plugin needs to do non-trivial setup that doesn't
    necessarily need to be done directly at startup, but still should be done within a
    short time afterwards. This can decrease the perceived plugin or application startup
    time a lot, with very little effort.

    \sa initialize()
    \sa extensionsInitialized()
*/

/*!
    \fn ExtensionSystem::IPlugin::ShutdownFlag ExtensionSystem::IPlugin::aboutToShutdown()
    Called during a shutdown sequence in the same order as initialization
    before the plugins get deleted in reverse order.

    This function should be used to disconnect from other plugins,
    hide all UI, and optimize shutdown in general.
    If a plugin needs to delay the real shutdown for a while, for example if
    it needs to wait for external processes to finish for a clean shutdown,
    the plugin can return IPlugin::AsynchronousShutdown from this function. This
    will keep the main event loop running after the aboutToShutdown() sequence
    has finished, until all plugins requesting asynchronous shutdown have sent
    the asynchronousShutdownFinished() signal.

    The default implementation of this function does nothing and returns
    IPlugin::SynchronousShutdown.

    Returns IPlugin::AsynchronousShutdown if the plugin needs to perform
    asynchronous actions before shutdown.

    \sa asynchronousShutdownFinished()
*/

/*!
    \fn QObject *ExtensionSystem::IPlugin::remoteCommand(const QStringList &options,
                                           const QString &workingDirectory,
                                           const QStringList &arguments)

    When \QC is executed with the \c -client argument while another \QC instance
    is running, this function of the plugin is called in the running instance.

    The \a workingDirectory argument specifies the working directory of the
    calling process. For example, if you're in a directory, and you execute
    \c { qtcreator -client file.cpp}, the working directory of the calling
    process is passed to the running instance and \c {file.cpp} is transformed
    into an absolute path starting from this directory.

    Plugin-specific arguments are passed in \a options, while the rest of the
    arguments are passed in \a arguments.

    Returns a QObject that blocks the command until it is destroyed, if \c -block
    is used.
*/

/*!
    \fn void ExtensionSystem::IPlugin::asynchronousShutdownFinished()
    Sent by the plugin implementation after an asynchronous shutdown
    is ready to proceed with the shutdown sequence.

    \sa aboutToShutdown()
*/

using namespace ExtensionSystem;

/*!
    \internal
*/
IPlugin::IPlugin()
    : d(new Internal::IPluginPrivate())
{
}

/*!
    \internal
*/
IPlugin::~IPlugin()
{
    delete d;
    d = nullptr;
}

/*!
    Returns objects that are meant to be passed on to \l QTest::qExec().

    This function will be called if the user starts \QC with
    \c {-test PluginName} or \c {-test all}.

    The ownership of returned objects is transferred to caller.
*/
QVector<QObject *> IPlugin::createTestObjects() const
{
    return {};
}

/*!
    Returns the PluginSpec corresponding to this plugin.
    This is not available in the constructor.
*/
PluginSpec *IPlugin::pluginSpec() const
{
    return d->pluginSpec;
}
