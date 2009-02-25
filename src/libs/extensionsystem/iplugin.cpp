/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "iplugin.h"
#include "iplugin_p.h"
#include "pluginmanager.h"
#include "pluginspec.h"

/*!
    \class ExtensionSystem::IPlugin
    \mainclass
    \brief Base class for all plugins.

    The IPlugin class is an abstract class that must be implemented
    once for each plugin.
    A plugin consists of two parts: A description file, and a library
    that at least contains the IPlugin implementation.

    \tableofcontents

    \section1 Plugin Specification
    The plugin specification file is an xml file that contains all
    information that are necessary for loading the plugin's library,
    plus some textual descriptions. The file must be located in
    (a subdir of) one of the plugin manager's plugin search paths,
    and must have the \c .xml extension.

    \section2 Main Tag
    The root tag is \c plugin. It has mandatory attributes \c name
    and \c version, and an optional \c compatVersion.
    \table
    \header
        \o Tag
        \o Meaning
    \row
        \o plugin
        \o Root element in a plugin's xml file.
    \endtable
    \table
    \header
        \o Attribute
        \o Meaning
    \row
        \o name
        \o This is used as an identifier for the plugin and can e.g.
           be referenced in other plugin's dependencies. It is
           also used to construct the name of the plugin library
           as \c lib[name].[dll|.so|.dylib].
    \row
        \o version
        \o Version string in the form \c {"x.y.z_n"}, used for identifying
           the plugin.
    \row
        \o compatVersion
        \o Compatibility version. Optional. If not given, it is implicitly
           set to the same value as \c version. The compatibility version
           is used to resolve dependencies on this plugin. See
           \l {Dependencies}{Dependencies} for details.
    \endtable

    \section2 Plugin-describing Tags
    These are direct children of the \c plugin tag, and are solely used
    for more detailed (user centric) description of the plugin. All of these
    are optional.
    \table
    \header
        \o Tag
        \o Meaning
    \row
        \o vendor
        \o String that describes the plugin creator/vendor,
           like \c {MyCompany}.
    \row
        \o copyright
        \o A short copyright notice, like \c {(C) 2007-2008 MyCompany}.
    \row
        \o license
        \o Possibly multi-line license information about the plugin.
    \row
        \o description
        \o Possibly multi-line description of what the plugin is supposed
           to provide.
    \row
        \o url
        \o Link to further information about the plugin, like
           \c {http://www.mycompany-online.com/products/greatplugin}.
    \endtable

    \section2 Dependencies
    A plugin can have dependencies on other plugins. These are
    specified in the plugin's xml file as well, to ensure that
    these other plugins are loaded before this plugin.
    Dependency information consists of the name of the required plugin
    (lets denote that as \c {dependencyName}),
    and the required version of the plugin (\c {dependencyVersion}).
    A plugin with given \c name, \c version and \c compatVersion matches
    the dependency if
    \list
        \o it's \c name matches \c dependencyName, and
        \o \c {compatVersion <= dependencyVersion <= version}.
    \endlist

    The xml element that describes dependencies is the \c dependency tag,
    with required attributes \c name and \c version. It is an
    optional direct child of the \c plugin tag and can appear multiple times.
    \table
    \header
        \o Tag
        \o Meaning
    \row
        \o dependency
        \o Describes a dependency on another plugin.
    \endtable
    \table
    \header
        \o Attribute
        \o Meaning
    \row
        \o name
        \o The name of the plugin, on which this plugin relies.
    \row
        \o version
        \o The version to which the plugin must be compatible to
           fill the dependency, in the form \c {"x.y.z_n"}.
    \endtable

    \section2 Example \c plugin.xml
    \code
        <plugin name="test" version="1.0.1" compatVersion="1.0.0">
            <vendor>MyCompany</vendor>
            <copyright>(C) 2007 MyCompany</copyright>
            <license>
        This is a default license bla
        blubbblubb
        end of terms
            </license>
            <description>
        This plugin is just a test.
            it demonstrates the great use of the plugin spec.
            </description>
            <url>http://www.mycompany-online.com/products/greatplugin</url>
            <dependencyList>
                <dependency name="SomeOtherPlugin" version="2.3.0_2"/>
                <dependency name="EvenOther" version="1.0.0"/>
            </dependencyList>
        </plugin>
    \endcode
    The first dependency could for example be matched by a plugin with
    \code
        <plugin name="SomeOtherPlugin" version="3.1.0" compatVersion="2.2.0">
        </plugin>
    \endcode
    since the name matches, and the version \c "2.3.0_2" given in the dependency tag
    lies in the range of \c "2.2.0" and \c "3.1.0".

    \section2 A Note on Plugin Versions
    Plugin versions are in the form \c "x.y.z_n" where, x, y, z and n are
    non-negative integer numbers. You don't have to specify the version
    in this full form - any left-out part will implicitly be set to zero.
    So, \c "2.10_2" is equal to \c "2.10.0_2", and "1" is the same as "1.0.0_0".

    \section1 Plugin Implementation
    Plugins must provide one implementation of the IPlugin class, located
    in a library that matches the \c name attribute given in their
    xml description. The IPlugin implementation must be exported and
    made known to Qt's plugin system via the Q_EXPORT_PLUGIN macro, see the
    Qt documentation for details on that.

    After the plugins' xml files have been read, and dependencies have been
    found, the plugin loading is done in three phases:
    \list 1
    \o All plugin libraries are loaded in 'root-to-leaf' order of the
       dependency tree.
    \o All plugins' initialize methods are called in 'root-to-leaf' order
       of the dependency tree. This is a good place to put
       objects in the plugin manager's object pool.
    \o All plugins' extensionsInitialized methods are called in 'leaf-to-root'
       order of the dependency tree. At this point, plugins can
       be sure that all plugins that depend on this plugin have
       been initialized completely (implying that they have put
       objects in the object pool, if they want that during the
       initialization sequence).
    \endlist
    If library loading or initialization of a plugin fails, all plugins
    that depend on that plugin also fail.

    Plugins have access to the plugin manager
    (and it's object pool) via the PluginManager::instance()
    method.
*/

/*!
    \fn bool IPlugin::initialize(const QStringList &arguments, QString *errorString)
    Called after the plugin has been loaded and the IPlugin instance
    has been created. The initialize methods of plugins that depend
    on this plugin are called after the initialize method of this plugin
    has been called. Plugins should initialize their internal state in this
    method. Returns if initialization of successful. If it wasn't successful,
    the \a errorString should be set to a user-readable message
    describing the reason.
    \sa extensionsInitialized()
*/

/*!
    \fn void IPlugin::extensionsInitialized()
    Called after the IPlugin::initialize() method has been called,
    and after both the IPlugin::initialize() and IPlugin::extensionsInitialized()
    methods of plugins that depend on this plugin have been called.
    In this method, the plugin can assume that plugins that depend on
    this plugin are fully 'up and running'. It is a good place to
    look in the plugin manager's object pool for objects that have
    been provided by dependent plugins.
    \sa initialize()
*/

/*!
    \fn void IPlugin::shutdown()
    Called during a shutdown sequence in the same order as initialization
    before the plugins get deleted in reverse order.
    This method can be used to optimize the shutdown down, e.g. to
    disconnect from the PluginManager::aboutToRemoveObject() signal
    if getting the signal (and probably doing lots of stuff to update
    the internal and visible state) doesn't make sense during shutdown.
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
    PluginManager *pm = PluginManager::instance();
    foreach (QObject *obj, d->addedObjectsInReverseOrder)
        pm->removeObject(obj);
    qDeleteAll(d->addedObjectsInReverseOrder);
    d->addedObjectsInReverseOrder.clear();
    delete d;
    d = 0;
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
    Convenience method that registers \a obj in the plugin manager's
    plugin pool by just calling PluginManager::addObject().
*/
void IPlugin::addObject(QObject *obj)
{
    PluginManager::instance()->addObject(obj);
}

/*!
    \fn void IPlugin::addAutoReleasedObject(QObject *obj)
    Convenience method for registering \a obj in the plugin manager's
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
    PluginManager::instance()->addObject(obj);
}

/*!
    \fn void IPlugin::removeObject(QObject *obj)
    Convenience method that unregisters \a obj from the plugin manager's
    plugin pool by just calling PluginManager::removeObject().
*/
void IPlugin::removeObject(QObject *obj)
{
    PluginManager::instance()->removeObject(obj);
}

