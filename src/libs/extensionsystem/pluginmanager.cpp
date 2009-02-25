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

#include "pluginmanager.h"
#include "pluginmanager_p.h"
#include "pluginspec.h"
#include "pluginspec_p.h"
#include "optionsparser.h"
#include "iplugin.h"

#include <QtCore/QMetaProperty>
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtCore/QWriteLocker>
#include <QtDebug>
#ifdef WITH_TESTS
#include <QTest>
#endif

typedef QList<ExtensionSystem::PluginSpec *> PluginSpecSet;

enum { debugLeaks = 0 };

/*!
    \namespace ExtensionSystem
    \brief The ExtensionSystem namespace provides classes that belong to the core plugin system.

    The basic extension system contains of the plugin manager and its supporting classes,
    and the IPlugin interface that must be implemented by plugin providers.
*/

/*!
    \namespace ExtensionSystem::Internal
    \internal
*/

/*!
    \class ExtensionSystem::PluginManager
    \mainclass

    \brief Core plugin system that manages the plugins, their life cycle and their registered objects.

    The plugin manager is used for the following tasks:
    \list
    \o Manage plugins and their state
    \o Manipulate a 'common object pool'
    \endlist

    \section1 Plugins
    Plugins consist of an xml descriptor file, and of a library that contains a Qt plugin
    (declared via Q_EXPORT_PLUGIN) that must derive from the IPlugin class.
    The plugin manager is used to set a list of file system directories to search for
    plugins, retrieve information about the state of these plugins, and to load them.

    Usually the application creates a PluginManager instance and initiates the loading.
    \code
        ExtensionSystem::PluginManager *manager = new ExtensionSystem::PluginManager();
        manager->setPluginPaths(QStringList() << "plugins"); // 'plugins' and subdirs will be searched for plugins
        manager->loadPlugins(); // try to load all the plugins
    \endcode
    Additionally it is possible to directly access to the plugin specifications
    (the information in the descriptor file), and the plugin instances (via PluginSpec),
    and their state.

    \section1 Object Pool
    Plugins (and everybody else) can add objects to a common 'pool' that is located in
    the plugin manager. Objects in the pool must derive from QObject, there are no other
    prerequisites. All objects of a specified type can be retrieved from the object pool
    via the getObjects() and getObject() methods. They are aware of Aggregation::Aggregate, i.e.
    these methods use the Aggregation::query methods instead of a qobject_cast to determine
    the matching objects.

    Whenever the state of the object pool changes a corresponding signal is emitted by the plugin manager.

    A common usecase for the object pool is that a plugin (or the application) provides
    an "extension point" for other plugins, which is a class / interface that can
    be implemented and added to the object pool. The plugin that provides the
    extension point looks for implementations of the class / interface in the object pool.
    \code
        // plugin A provides a "MimeTypeHandler" extension point
        // in plugin B:
        MyMimeTypeHandler *handler = new MyMimeTypeHandler();
        ExtensionSystem::PluginManager::instance()->addObject(handler);
        // in plugin A:
        QList<MimeTypeHandler *> mimeHandlers =
            ExtensionSystem::PluginManager::instance()->getObjects<MimeTypeHandler>();
    \endcode

    \bold Note: The object pool manipulating functions are thread-safe.
*/

/*!
    \fn void PluginManager::objectAdded(QObject *obj)
    Signal that \a obj has been added to the object pool.
*/

/*!
    \fn void PluginManager::aboutToRemoveObject(QObject *obj)
    Signal that \a obj will be removed from the object pool.
*/

/*!
    \fn void PluginManager::pluginsChanged()
    Signal that the list of available plugins has changed.

    \sa plugins()
*/

/*!
    \fn T *PluginManager::getObject() const
    Retrieve the object of a given type from the object pool.
    This method is aware of Aggregation::Aggregate, i.e. it uses
    the Aggregation::query methods instead of qobject_cast to
    determine the type of an object.
    If there are more than one object of the given type in
    the object pool, this method will choose an arbitrary one of them.

    \sa addObject()
*/

/*!
    \fn QList<T *> PluginManager::getObjects() const
    Retrieve all objects of a given type from the object pool.
    This method is aware of Aggregation::Aggregate, i.e. it uses
    the Aggregation::query methods instead of qobject_cast to
    determine the type of an object.

    \sa addObject()
*/

using namespace ExtensionSystem;
using namespace ExtensionSystem::Internal;

static bool lessThanByPluginName(const PluginSpec *one, const PluginSpec *two)
{
    return one->name() < two->name();
}

PluginManager *PluginManager::m_instance = 0;

/*!
    \fn PluginManager *PluginManager::instance()
    Get the unique plugin manager instance.
*/
PluginManager *PluginManager::instance()
{
    return m_instance;
}

/*!
    \fn PluginManager::PluginManager()
    Create a plugin manager. Should be done only once per application.
*/
PluginManager::PluginManager()
    : d(new PluginManagerPrivate(this))
{
    m_instance = this;
}

/*!
    \fn PluginManager::~PluginManager()
    \internal
*/
PluginManager::~PluginManager()
{
    delete d;
    d = 0;
}

/*!
    \fn void PluginManager::addObject(QObject *obj)
    Add the given object \a obj to the object pool, so it can be retrieved again from the pool by type.
    The plugin manager does not do any memory management - added objects
    must be removed from the pool and deleted manually by whoever is responsible for the object.

    Emits the objectAdded() signal.

    \sa PluginManager::removeObject()
    \sa PluginManager::getObject()
    \sa PluginManager::getObjects()
*/
void PluginManager::addObject(QObject *obj)
{
    d->addObject(obj);
}

/*!
    \fn void PluginManager::removeObject(QObject *obj)
    Emits aboutToRemoveObject() and removes the object \a obj from the object pool.
    \sa PluginManager::addObject()
*/
void PluginManager::removeObject(QObject *obj)
{
    d->removeObject(obj);
}

/*!
    \fn QList<QObject *> PluginManager::allObjects() const
    Retrieve the list of all objects in the pool, unfiltered.
    Usually clients do not need to call this.
    \sa PluginManager::getObject()
    \sa PluginManager::getObjects()
*/
QList<QObject *> PluginManager::allObjects() const
{
    return d->allObjects;
}

/*!
    \fn void PluginManager::loadPlugins()
    Tries to load all the plugins that were previously found when
    setting the plugin search paths. The plugin specs of the plugins
    can be used to retrieve error and state information about individual plugins.

    \sa setPluginPaths()
    \sa plugins()
*/
void PluginManager::loadPlugins()
{
    return d->loadPlugins();
}

/*!
    \fn QStringList PluginManager::pluginPaths() const
    The list of paths were the plugin manager searches for plugins.

    \sa setPluginPaths()
*/
QStringList PluginManager::pluginPaths() const
{
    return d->pluginPaths;
}

/*!
    \fn void PluginManager::setPluginPaths(const QStringList &paths)
    Sets the plugin search paths, i.e. the file system paths where the plugin manager
    looks for plugin descriptions. All given \a paths and their sub directory trees
    are searched for plugin xml description files.

    \sa pluginPaths()
    \sa loadPlugins()
*/
void PluginManager::setPluginPaths(const QStringList &paths)
{
    d->setPluginPaths(paths);
}

/*!
    \fn QString PluginManager::fileExtension() const
    The file extension of plugin description files.
    The default is "xml".

    \sa setFileExtension()
*/
QString PluginManager::fileExtension() const
{
    return d->extension;
}

/*!
    \fn void PluginManager::setFileExtension(const QString &extension)
    Sets the file extension of plugin description files.
    The default is "xml".
    At the moment this must be called before setPluginPaths() is called.
    // ### TODO let this + setPluginPaths read the plugin specs lazyly whenever loadPlugins() or plugins() is called.
*/
void PluginManager::setFileExtension(const QString &extension)
{
    d->extension = extension;
}

/*!
    \fn QStringList PluginManager::arguments() const
    The arguments left over after parsing (Neither startup nor plugin
    arguments). Typically, this will be the list of files to open.
*/
QStringList PluginManager::arguments() const
{
    return d->arguments;
}

/*!
    \fn QList<PluginSpec *> PluginManager::plugins() const
    List of all plugin specifications that have been found in the plugin search paths.
    This list is valid directly after the setPluginPaths() call.
    The plugin specifications contain the information from the plugins' xml description files
    and the current state of the plugins. If a plugin's library has been already successfully loaded,
    the plugin specification has a reference to the created plugin instance as well.

    \sa setPluginPaths()
*/
QList<PluginSpec *> PluginManager::plugins() const
{
    return d->pluginSpecs;
}

/*!
    \fn bool PluginManager::parseOptions(const QStringList &args, const QMap<QString, bool> &appOptions, QMap<QString, QString> *foundAppOptions, QString *errorString)
    Takes the list of command line options in \a args and parses them.
    The plugin manager itself might process some options itself directly (-noload <plugin>), and
    adds options that are registered by plugins to their plugin specs.
    The caller (the application) may register itself for options via the \a appOptions list, containing pairs
    of "option string" and a bool that indicates if the option requires an argument.
    Application options always override any plugin's options.
    
    \a foundAppOptions is set to pairs of ("option string", "argument") for any application options that were found.
    The command line options that were not processed can be retrieved via the arguments() method.
    If an error occurred (like missing argument for an option that requires one), \a errorString contains
    a descriptive message of the error.
    
    Returns if there was an error.
 */
bool PluginManager::parseOptions(const QStringList &args,
    const QMap<QString, bool> &appOptions,
    QMap<QString, QString> *foundAppOptions,
    QString *errorString)
{
    OptionsParser options(args, appOptions, foundAppOptions, errorString, d);
    return options.parse();
}



static inline void indent(QTextStream &str, int indent)
{
    const QChar blank = QLatin1Char(' ');
    for (int i = 0 ; i < indent; i++)
        str << blank;
}

static inline void formatOption(QTextStream &str,
                                const QString &opt, const QString &parm, const QString &description,
                                int optionIndentation, int descriptionIndentation)
{
    int remainingIndent = descriptionIndentation - optionIndentation - opt.size();
    indent(str, optionIndentation);
    str << opt;
    if (!parm.isEmpty()) {
        str << " <" << parm << '>';
        remainingIndent -= 3 + parm.size();
    }
    indent(str, qMax(0, remainingIndent));
    str << description << '\n';
}

/*!
    \fn static PluginManager::formatOptions(QTextStream &str, int optionIndentation, int descriptionIndentation)

    Format the startup options of the plugin manager for command line help.
*/

void PluginManager::formatOptions(QTextStream &str, int optionIndentation, int descriptionIndentation)
{
    formatOption(str, QLatin1String(OptionsParser::NO_LOAD_OPTION),
                 QLatin1String("plugin"), QLatin1String("Do not load <plugin>"),
                 optionIndentation, descriptionIndentation);
}

/*!
    \fn PluginManager::formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation) const

    Format the plugin  options of the plugin specs for command line help.
*/

void PluginManager::formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation) const
{
    typedef PluginSpec::PluginArgumentDescriptions PluginArgumentDescriptions;
    // Check plugins for options
    const PluginSpecSet::const_iterator pcend = d->pluginSpecs.constEnd();
    for (PluginSpecSet::const_iterator pit = d->pluginSpecs.constBegin(); pit != pcend; ++pit) {
        const PluginArgumentDescriptions pargs = (*pit)->argumentDescriptions();
        if (!pargs.empty()) {
            str << "\nPlugin: " <<  (*pit)->name() << '\n';
            const PluginArgumentDescriptions::const_iterator acend = pargs.constEnd();
            for (PluginArgumentDescriptions::const_iterator ait =pargs.constBegin(); ait != acend; ++ait)
                formatOption(str, ait->name, ait->parameter, ait->description, optionIndentation, descriptionIndentation);
        }
    }
}

/*!
    \fn PluginManager::formatPluginVersions(QTextStream &str) const

    Format the version of the plugin specs for command line help.
*/

void PluginManager::formatPluginVersions(QTextStream &str) const
{
    const PluginSpecSet::const_iterator cend = d->pluginSpecs.constEnd();
    for (PluginSpecSet::const_iterator it = d->pluginSpecs.constBegin(); it != cend; ++it) {
        const PluginSpec *ps = *it;
        str << "  " << ps->name() << ' ' << ps->version() << ' ' << ps->description() <<  '\n';
    }
}

void PluginManager::startTests()
{
#ifdef WITH_TESTS
    foreach (PluginSpec *pluginSpec, d->testSpecs) {
        const QMetaObject *mo = pluginSpec->plugin()->metaObject();
        QStringList methods;
        methods.append("arg0");
        // We only want slots starting with "test"
        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
            if (QByteArray(mo->method(i).signature()).startsWith("test")) {
                QString method = QString::fromLatin1(mo->method(i).signature());
                methods.append(method.left(method.size()-2));
            }
        }
        QTest::qExec(pluginSpec->plugin(), methods);

    }
#endif
}

/*!
 * \fn bool PluginManager::runningTests() const
 * \internal
 */
bool PluginManager::runningTests() const
{
    return !d->testSpecs.isEmpty();
}

/*!
 * \fn QString PluginManager::testDataDirectory() const
 * \internal
 */
QString PluginManager::testDataDirectory() const
{
    QString s = QString::fromLocal8Bit(qgetenv("IDETESTDIR"));
    if (s.isEmpty()) {
        s = IDE_TEST_DIR;
        s.append("/tests");
    }
    s = QDir::cleanPath(s);
    return s;
}

//============PluginManagerPrivate===========

/*!
    \fn PluginSpec *PluginManagerPrivate::createSpec()
    \internal
*/
PluginSpec *PluginManagerPrivate::createSpec()
{
    return new PluginSpec();
}

/*!
    \fn PluginSpecPrivate *PluginManagerPrivate::privateSpec(PluginSpec *spec)
    \internal
*/
PluginSpecPrivate *PluginManagerPrivate::privateSpec(PluginSpec *spec)
{
    return spec->d;
}

/*!
    \fn PluginManagerPrivate::PluginManagerPrivate(PluginManager *pluginManager)
    \internal
*/
PluginManagerPrivate::PluginManagerPrivate(PluginManager *pluginManager)
    : extension("xml"), q(pluginManager)
{
}

/*!
    \fn PluginManagerPrivate::~PluginManagerPrivate()
    \internal
*/
PluginManagerPrivate::~PluginManagerPrivate()
{
    stopAll();
    qDeleteAll(pluginSpecs);
    if (!allObjects.isEmpty()) {
        qDebug() << "There are" << allObjects.size() << "objects left in the plugin manager pool: " << allObjects;
    }
}

void PluginManagerPrivate::stopAll()
{
    QList<PluginSpec *> queue = loadQueue();
    foreach (PluginSpec *spec, queue) {
        loadPlugin(spec, PluginSpec::Stopped);
    }
    QListIterator<PluginSpec *> it(queue);
    it.toBack();
    while (it.hasPrevious()) {
        loadPlugin(it.previous(), PluginSpec::Deleted);
    }
}

/*!
    \fn void PluginManagerPrivate::addObject(QObject *obj)
    \internal
*/
void PluginManagerPrivate::addObject(QObject *obj)
{
    {
        QWriteLocker lock(&(q->m_lock));
        if (obj == 0) {
            qWarning() << "PluginManagerPrivate::addObject(): trying to add null object";
            return;
        }
        if (allObjects.contains(obj)) {
            qWarning() << "PluginManagerPrivate::addObject(): trying to add duplicate object";
            return;
        }

        if (debugLeaks)
            qDebug() << "PluginManagerPrivate::addObject" << obj << obj->objectName();

        allObjects.append(obj);
    }
    emit q->objectAdded(obj);
}

/*!
    \fn void PluginManagerPrivate::removeObject(QObject *obj)
    \internal
*/
void PluginManagerPrivate::removeObject(QObject *obj)
{
    if (obj == 0) {
        qWarning() << "PluginManagerPrivate::removeObject(): trying to remove null object";
        return;
    }

    if (!allObjects.contains(obj)) {
        qWarning() << "PluginManagerPrivate::removeObject(): object not in list:"
            << obj << obj->objectName();
        return;
    }
    if (debugLeaks)
        qDebug() << "PluginManagerPrivate::removeObject" << obj << obj->objectName();

    emit q->aboutToRemoveObject(obj);
    QWriteLocker lock(&(q->m_lock));
    allObjects.removeAll(obj);
}

/*!
    \fn void PluginManagerPrivate::loadPlugins()
    \internal
*/
void PluginManagerPrivate::loadPlugins()
{
    QList<PluginSpec *> queue = loadQueue();
    foreach (PluginSpec *spec, queue) {
        loadPlugin(spec, PluginSpec::Loaded);
    }
    foreach (PluginSpec *spec, queue) {
        loadPlugin(spec, PluginSpec::Initialized);
    }
    QListIterator<PluginSpec *> it(queue);
    it.toBack();
    while (it.hasPrevious()) {
        loadPlugin(it.previous(), PluginSpec::Running);
    }
    emit q->pluginsChanged();
}

/*!
    \fn void PluginManagerPrivate::loadQueue()
    \internal
*/
QList<PluginSpec *> PluginManagerPrivate::loadQueue()
{
    QList<PluginSpec *> queue;
    foreach (PluginSpec *spec, pluginSpecs) {
        QList<PluginSpec *> circularityCheckQueue;
        loadQueue(spec, queue, circularityCheckQueue);
    }
    return queue;
}

/*!
    \fn bool PluginManagerPrivate::loadQueue(PluginSpec *spec, QList<PluginSpec *> &queue, QList<PluginSpec *> &circularityCheckQueue)
    \internal
*/
bool PluginManagerPrivate::loadQueue(PluginSpec *spec, QList<PluginSpec *> &queue,
        QList<PluginSpec *> &circularityCheckQueue)
{
    if (queue.contains(spec))
        return true;
    // check for circular dependencies
    if (circularityCheckQueue.contains(spec)) {
        spec->d->hasError = true;
        spec->d->errorString = q->tr("Circular dependency detected:\n");
        int index = circularityCheckQueue.indexOf(spec);
        for (int i = index; i < circularityCheckQueue.size(); ++i) {
            spec->d->errorString.append(q->tr("%1(%2) depends on\n")
                .arg(circularityCheckQueue.at(i)->name()).arg(circularityCheckQueue.at(i)->version()));
        }
        spec->d->errorString.append(q->tr("%1(%2)").arg(spec->name()).arg(spec->version()));
        return false;
    }
    circularityCheckQueue.append(spec);
    // check if we have the dependencies
    if (spec->state() == PluginSpec::Invalid || spec->state() == PluginSpec::Read) {
        spec->d->hasError = true;
        spec->d->errorString += "\n";
        spec->d->errorString += q->tr("Cannot load plugin because dependencies are not resolved");
        return false;
    }
    // add dependencies
    foreach (PluginSpec *depSpec, spec->dependencySpecs()) {
        if (!loadQueue(depSpec, queue, circularityCheckQueue)) {
            spec->d->hasError = true;
            spec->d->errorString =
                q->tr("Cannot load plugin because dependency failed to load: %1(%2)\nReason: %3")
                    .arg(depSpec->name()).arg(depSpec->version()).arg(depSpec->errorString());
            return false;
        }
    }
    // add self
    queue.append(spec);
    return true;
}

/*!
    \fn void PluginManagerPrivate::loadPlugin(PluginSpec *spec, PluginSpec::State destState)
    \internal
*/
void PluginManagerPrivate::loadPlugin(PluginSpec *spec, PluginSpec::State destState)
{
    if (spec->hasError())
        return;
    if (destState == PluginSpec::Running) {
        spec->d->initializeExtensions();
        return;
    } else if (destState == PluginSpec::Deleted) {
        spec->d->kill();
        return;
    }
    foreach (PluginSpec *depSpec, spec->dependencySpecs()) {
        if (depSpec->state() != destState) {
            spec->d->hasError = true;
            spec->d->errorString =
                q->tr("Cannot load plugin because dependency failed to load: %1(%2)\nReason: %3")
                    .arg(depSpec->name()).arg(depSpec->version()).arg(depSpec->errorString());
            return;
        }
    }
    if (destState == PluginSpec::Loaded)
        spec->d->loadLibrary();
    else if (destState == PluginSpec::Initialized)
        spec->d->initializePlugin();
    else if (destState == PluginSpec::Stopped)
        spec->d->stop();
}

/*!
    \fn void PluginManagerPrivate::setPluginPaths(const QStringList &paths)
    \internal
*/
void PluginManagerPrivate::setPluginPaths(const QStringList &paths)
{
    pluginPaths = paths;
    readPluginPaths();
}

/*!
    \fn void PluginManagerPrivate::readPluginPaths()
    \internal
*/
void PluginManagerPrivate::readPluginPaths()
{
    qDeleteAll(pluginSpecs);
    pluginSpecs.clear();

    QStringList specFiles;
    QStringList searchPaths = pluginPaths;
    while (!searchPaths.isEmpty()) {
        const QDir dir(searchPaths.takeFirst());
        const QFileInfoList files = dir.entryInfoList(QStringList() << QString("*.%1").arg(extension), QDir::Files);
        foreach (const QFileInfo &file, files)
            specFiles << file.absoluteFilePath();
        const QFileInfoList dirs = dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
        foreach (const QFileInfo &subdir, dirs)
            searchPaths << subdir.absoluteFilePath();
    }
    foreach (const QString &specFile, specFiles) {
        PluginSpec *spec = new PluginSpec;
        spec->d->read(specFile);
        pluginSpecs.append(spec);
    }
    resolveDependencies();
    // ensure deterministic plugin load order by sorting
    qSort(pluginSpecs.begin(), pluginSpecs.end(), lessThanByPluginName);
    emit q->pluginsChanged();
}

void PluginManagerPrivate::resolveDependencies()
{
    foreach (PluginSpec *spec, pluginSpecs) {
        spec->d->resolveDependencies(pluginSpecs);
    }
}

 // Look in argument descriptions of the specs for the option.
PluginSpec *PluginManagerPrivate::pluginForOption(const QString &option, bool *requiresArgument) const
{
    // Look in the plugins for an option
    typedef PluginSpec::PluginArgumentDescriptions PluginArgumentDescriptions;

    *requiresArgument = false;
    const PluginSpecSet::const_iterator pcend = pluginSpecs.constEnd();
    for (PluginSpecSet::const_iterator pit = pluginSpecs.constBegin(); pit != pcend; ++pit) {
        PluginSpec *ps = *pit;
        const PluginArgumentDescriptions pargs = ps->argumentDescriptions();
        if (!pargs.empty()) {
            const PluginArgumentDescriptions::const_iterator acend = pargs.constEnd();
            for (PluginArgumentDescriptions::const_iterator ait = pargs.constBegin(); ait != acend; ++ait) {
                if (ait->name == option) {
                    *requiresArgument = !ait->parameter.isEmpty();
                    return ps;
                }
            }
        }
    }
    return 0;
}

PluginSpec *PluginManagerPrivate::pluginByName(const QString &name) const
{
    foreach (PluginSpec *spec, pluginSpecs)
        if (spec->name() == name)
            return spec;
    return 0;
}

