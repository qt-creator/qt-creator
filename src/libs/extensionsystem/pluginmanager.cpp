/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pluginmanager.h"
#include "pluginmanager_p.h"
#include "pluginspec.h"
#include "pluginspec_p.h"
#include "optionsparser.h"
#include "iplugin.h"
#include "plugincollection.h"

#include <QEventLoop>
#include <QDateTime>
#include <QDir>
#include <QMetaProperty>
#include <QSettings>
#include <QTextStream>
#include <QTime>
#include <QWriteLocker>
#include <QDebug>
#include <QTimer>

#ifdef WITH_TESTS
#include <QTest>
#endif

static const char C_IGNORED_PLUGINS[] = "Plugins/Ignored";
static const char C_FORCEENABLED_PLUGINS[] = "Plugins/ForceEnabled";
static const int DELAYED_INITIALIZE_INTERVAL = 20; // ms

typedef QList<ExtensionSystem::PluginSpec *> PluginSpecSet;

enum { debugLeaks = 0 };

/*!
    \namespace ExtensionSystem
    \brief The ExtensionSystem namespace provides classes that belong to the
           core plugin system.

    The basic extension system contains the plugin manager and its supporting classes,
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
        // Plugin A provides a "MimeTypeHandler" extension point
        // in plugin B:
        MyMimeTypeHandler *handler = new MyMimeTypeHandler();
        ExtensionSystem::PluginManager::instance()->addObject(handler);
        // In plugin A:
        QList<MimeTypeHandler *> mimeHandlers =
            ExtensionSystem::PluginManager::instance()->getObjects<MimeTypeHandler>();
    \endcode


    The \c{ExtensionSystem::Invoker} class template provides "syntactic sugar"
    for using "soft" extension points that may or may not be provided by an
    object in the pool. This approach does neither require the "user" plugin being
    linked against the "provider" plugin nor a common shared
    header file. The exposed interface is implicitly given by the
    invokable methods of the "provider" object in the object pool.

    The \c{ExtensionSystem::invoke} function template encapsulates
    {ExtensionSystem::Invoker} construction for the common case where
    the success of the call is not checked.

    \code
        // In the "provide" plugin A:
        namespace PluginA {
        class SomeProvider : public QObject
        {
            Q_OBJECT

        public:
            Q_INVOKABLE QString doit(const QString &msg, int n) {
            {
                qDebug() << "I AM DOING IT " << msg;
                return QString::number(n);
            }
        };
        } // namespace PluginA


        // In the "user" plugin B:
        int someFuntionUsingPluginA()
        {
            using namespace ExtensionSystem;

            QObject *target = PluginManager::instance()
                ->getObjectByClassName("PluginA::SomeProvider");

            if (target) {
                // Some random argument.
                QString msg = "REALLY.";

                // Plain function call, no return value.
                invoke<void>(target, "doit", msg, 2);

                // Plain function with no return value.
                qDebug() << "Result: " << invoke<QString>(target, "doit", msg, 21);

                // Record success of function call with return value.
                Invoker<QString> in1(target, "doit", msg, 21);
                qDebug() << "Success: (expected)" << in1.wasSuccessful();

                // Try to invoke a non-existing function.
                Invoker<QString> in2(target, "doitWrong", msg, 22);
                qDebug() << "Success (not expected):" << in2.wasSuccessful();

            } else {

                // We have to cope with plugin A's absence.
            }
        };
    \endcode

    \bold Note: The type of the parameters passed to the \c{invoke()} calls
    is deduced from the parameters themselves and must match the type of
    the arguments of the called functions \e{exactly}. No conversion or even
    integer promotions are applicable, so to invoke a function with a \c{long}
    parameter explicitly use \c{long(43)} or such.

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
    m_instance->d->addObject(obj);
}

/*!
    \fn void PluginManager::removeObject(QObject *obj)
    Emits aboutToRemoveObject() and removes the object \a obj from the object pool.
    \sa PluginManager::addObject()
*/
void PluginManager::removeObject(QObject *obj)
{
    m_instance->d->removeObject(obj);
}

/*!
    \fn QList<QObject *> PluginManager::allObjects() const
    Retrieve the list of all objects in the pool, unfiltered.
    Usually clients do not need to call this.
    \sa PluginManager::getObject()
    \sa PluginManager::getObjects()
*/
QList<QObject *> PluginManager::allObjects()
{
    return m_instance->d->allObjects;
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
    return m_instance->d->loadPlugins();
}

/*!
    \fn bool PluginManager::hasError() const
    Returns true if any plugin has errors even though it is enabled.
    Most useful to call after loadPlugins().
*/
bool PluginManager::hasError()
{
    foreach (PluginSpec *spec, plugins()) {
        // only show errors on startup if plugin is enabled.
        if (spec->hasError() && spec->isEnabled() && !spec->isDisabledIndirectly())
            return true;
    }
    return false;
}

/*!
    \fn void PluginManager::shutdown()
    Shuts down and deletes all plugins.
*/
void PluginManager::shutdown()
{
    d->shutdown();
}

/*!
    \fn QStringList PluginManager::pluginPaths() const
    The list of paths were the plugin manager searches for plugins.

    \sa setPluginPaths()
*/
QStringList PluginManager::pluginPaths()
{
    return m_instance->d->pluginPaths;
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
    m_instance->d->setPluginPaths(paths);
}

/*!
    \fn QString PluginManager::fileExtension() const
    The file extension of plugin description files.
    The default is "xml".

    \sa setFileExtension()
*/
QString PluginManager::fileExtension()
{
    return m_instance->d->extension;
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
    m_instance->d->extension = extension;
}

/*!
    Define the user specific settings to use for information about enabled/disabled plugins.
    Needs to be set before the plugin search path is set with setPluginPaths().
*/
void PluginManager::setSettings(QSettings *settings)
{
    m_instance->d->setSettings(settings);
}

/*!
    Define the global (user-independent) settings to use for information about default disabled plugins.
    Needs to be set before the plugin search path is set with setPluginPaths().
*/
void PluginManager::setGlobalSettings(QSettings *settings)
{
    m_instance->d->setGlobalSettings(settings);
}

/*!
    Returns the user specific settings used for information about enabled/disabled plugins.
*/
QSettings *PluginManager::settings()
{
    return m_instance->d->settings;
}

/*!
    Returns the global (user-independent) settings used for information about default disabled plugins.
*/
QSettings *PluginManager::globalSettings()
{
    return m_instance->d->globalSettings;
}

void PluginManager::writeSettings()
{
    m_instance->d->writeSettings();
}

/*!
    \fn QStringList PluginManager::arguments() const
    The arguments left over after parsing (Neither startup nor plugin
    arguments). Typically, this will be the list of files to open.
*/
QStringList PluginManager::arguments()
{
    return m_instance->d->arguments;
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
QList<PluginSpec *> PluginManager::plugins()
{
    return m_instance->d->pluginSpecs;
}

QHash<QString, PluginCollection *> PluginManager::pluginCollections()
{
    return m_instance->d->pluginCategories;
}

/*!
    \fn QString PluginManager::serializedArguments() const

    Serialize plugin options and arguments for sending in a single string
    via QtSingleApplication:
    ":myplugin|-option1|-option2|:arguments|argument1|argument2",
    as a list of lists started by a keyword with a colon. Arguments are last.

    \sa setPluginPaths()
*/

static const char argumentKeywordC[] = ":arguments";

QString PluginManager::serializedArguments()
{
    const QChar separator = QLatin1Char('|');
    QString rc;
    foreach (const PluginSpec *ps, plugins()) {
        if (!ps->arguments().isEmpty()) {
            if (!rc.isEmpty())
                rc += separator;
            rc += QLatin1Char(':');
            rc += ps->name();
            rc += separator;
            rc +=  ps->arguments().join(QString(separator));
        }
    }
    if (!m_instance->d->arguments.isEmpty()) {
        if (!rc.isEmpty())
            rc += separator;
        rc += QLatin1String(argumentKeywordC);
        // If the argument appears to be a file, make it absolute
        // when sending to another instance.
        foreach (const QString &argument, m_instance->d->arguments) {
            rc += separator;
            const QFileInfo fi(argument);
            if (fi.exists() && fi.isRelative())
                rc += fi.absoluteFilePath();
            else
                rc += argument;
        }
    }
    return rc;
}

/* Extract a sublist from the serialized arguments
 * indicated by a keyword starting with a colon indicator:
 * ":a,i1,i2,:b:i3,i4" with ":a" -> "i1,i2"
 */
static QStringList subList(const QStringList &in, const QString &key)
{
    QStringList rc;
    // Find keyword and copy arguments until end or next keyword
    const QStringList::const_iterator inEnd = in.constEnd();
    QStringList::const_iterator it = qFind(in.constBegin(), inEnd, key);
    if (it != inEnd) {
        const QChar nextIndicator = QLatin1Char(':');
        for (++it; it != inEnd && !it->startsWith(nextIndicator); ++it)
            rc.append(*it);
    }
    return rc;
}

/*!
    \fn PluginManager::remoteArguments(const QString &argument)

    Parses the options encoded by serializedArguments() const
    and passes them on to the respective plugins along with the arguments.
*/

void PluginManager::remoteArguments(const QString &serializedArgument)
{
    if (serializedArgument.isEmpty())
        return;
    QStringList serializedArguments = serializedArgument.split(QLatin1Char('|'));
    const QStringList arguments = subList(serializedArguments, QLatin1String(argumentKeywordC));
    foreach (const PluginSpec *ps, plugins()) {
        if (ps->state() == PluginSpec::Running) {
            const QStringList pluginOptions = subList(serializedArguments, QLatin1Char(':') + ps->name());
            ps->plugin()->remoteCommand(pluginOptions, arguments);
        }
    }
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
    OptionsParser options(args, appOptions, foundAppOptions, errorString, m_instance->d);
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
    indent(str, qMax(1, remainingIndent));
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
    formatOption(str, QLatin1String(OptionsParser::PROFILE_OPTION),
                 QString(), QLatin1String("Profile plugin loading"),
                 optionIndentation, descriptionIndentation);
#ifdef WITH_TESTS
    formatOption(str, QString::fromLatin1(OptionsParser::TEST_OPTION)
                 + QLatin1String(" <plugin> [testfunction[:testdata]]..."), QString(),
                 QLatin1String("Run plugin's tests"), optionIndentation, descriptionIndentation);
    formatOption(str, QString::fromLatin1(OptionsParser::TEST_OPTION) + QLatin1String(" all"),
                 QString(), QLatin1String("Run tests from all plugins"),
                 optionIndentation, descriptionIndentation);
#endif
}

/*!
    \fn PluginManager::formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation) const

    Format the plugin  options of the plugin specs for command line help.
*/

void PluginManager::formatPluginOptions(QTextStream &str, int optionIndentation, int descriptionIndentation)
{
    typedef PluginSpec::PluginArgumentDescriptions PluginArgumentDescriptions;
    // Check plugins for options
    const PluginSpecSet::const_iterator pcend = m_instance->d->pluginSpecs.constEnd();
    for (PluginSpecSet::const_iterator pit = m_instance->d->pluginSpecs.constBegin(); pit != pcend; ++pit) {
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

void PluginManager::formatPluginVersions(QTextStream &str)
{
    const PluginSpecSet::const_iterator cend = m_instance->d->pluginSpecs.constEnd();
    for (PluginSpecSet::const_iterator it = m_instance->d->pluginSpecs.constBegin(); it != cend; ++it) {
        const PluginSpec *ps = *it;
        str << "  " << ps->name() << ' ' << ps->version() << ' ' << ps->description() <<  '\n';
    }
}

void PluginManager::startTests()
{
#ifdef WITH_TESTS
    foreach (const PluginManagerPrivate::TestSpec &testSpec, d->testSpecs) {
        const PluginSpec * const pluginSpec = testSpec.pluginSpec;
        if (!pluginSpec->plugin())
            continue;

        // Collect all test functions/methods of the plugin.
        QStringList allTestFunctions;
        const QMetaObject *metaObject = pluginSpec->plugin()->metaObject();

        for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
#if QT_VERSION >= 0x050000
            const QByteArray signature = metaObject->method(i).methodSignature();
#else
            const QByteArray signature = metaObject->method(i).signature();
#endif
            if (signature.startsWith("test") && !signature.endsWith("_data()")) {
                const QString method = QString::fromLatin1(signature);
                const QString methodName = method.left(method.size() - 2);
                allTestFunctions.append(methodName);
            }
        }

        QStringList testFunctionsToExecute;

        // User did not specify any test functions, so add every test function.
        if (testSpec.testFunctions.isEmpty()) {
            testFunctionsToExecute = allTestFunctions;

        // User specified test functions. Add them if they are valid.
        } else {
            foreach (const QString &userTestFunction, testSpec.testFunctions) {
                // There might be a test data suffix like in "testfunction:testdata1".
                QString testFunctionName = userTestFunction;
                const int index = testFunctionName.indexOf(QLatin1Char(':'));
                if (index != -1)
                    testFunctionName = testFunctionName.left(index);

                if (allTestFunctions.contains(testFunctionName)) {
                    // If the specified test data is invalid, the QTest framework will
                    // print a reasonable error message for us.
                    testFunctionsToExecute.append(userTestFunction);
                } else {
                    QTextStream out(stdout);
                    out << "Unknown test function \"" << testFunctionName
                        << "\" for plugin \"" << pluginSpec->name() << "\"." << endl
                        << "  Available test functions for plugin \"" << pluginSpec->name()
                        << "\" are:" << endl;
                    foreach (const QString &testFunction, allTestFunctions)
                        out << "    " << testFunction << endl;
                }
            }
        }

        // QTest::qExec() expects basically QCoreApplication::arguments(),
        // so prepend a fake argument for the application name.
        testFunctionsToExecute.prepend(QLatin1String("arg0"));

        // Don't run QTest::qExec with only one argument, that'd run
        // *all* slots as tests.
        if (testFunctionsToExecute.size() > 1)
            QTest::qExec(pluginSpec->plugin(), testFunctionsToExecute);
    }
    if (!d->testSpecs.isEmpty())
        QTimer::singleShot(1, QCoreApplication::instance(), SLOT(quit()));
#endif
}

/*!
 * \fn bool PluginManager::runningTests() const
 * \internal
 */
bool PluginManager::runningTests()
{
    return !m_instance->d->testSpecs.isEmpty();
}

/*!
 * \fn QString PluginManager::testDataDirectory() const
 * \internal
 */
QString PluginManager::testDataDirectory()
{
    QByteArray ba = qgetenv("QTCREATOR_TEST_DIR");
    QString s = QString::fromLocal8Bit(ba.constData(), ba.size());
    if (s.isEmpty()) {
        s = QLatin1String(IDE_TEST_DIR);
        s.append(QLatin1String("/tests"));
    }
    s = QDir::cleanPath(s);
    return s;
}

/*!
    \fn void PluginManager::profilingReport(const char *what, const PluginSpec *spec = 0)

    Create a profiling entry showing the elapsed time if profiling is activated.
*/

void PluginManager::profilingReport(const char *what, const PluginSpec *spec)
{
    m_instance->d->profilingReport(what, spec);
}


/*!
    \fn void PluginManager::loadQueue()

    Returns a list of plugins in load order.
*/
QList<PluginSpec *> PluginManager::loadQueue()
{
    return m_instance->d->loadQueue();
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
    \fn void PluginManagerPrivate::setSettings(QSettings *settings)
    \internal
*/
void PluginManagerPrivate::setSettings(QSettings *s)
{
    if (settings)
        delete settings;
    settings = s;
    if (settings)
        settings->setParent(this);
}

/*!
    \internal
*/
void PluginManagerPrivate::setGlobalSettings(QSettings *s)
{
    if (globalSettings)
        delete globalSettings;
    globalSettings = s;
    if (globalSettings)
        globalSettings->setParent(this);
}

/*!
    \fn PluginSpecPrivate *PluginManagerPrivate::privateSpec(PluginSpec *spec)
    \internal
*/
PluginSpecPrivate *PluginManagerPrivate::privateSpec(PluginSpec *spec)
{
    return spec->d;
}

void PluginManagerPrivate::nextDelayedInitialize()
{
    while (!delayedInitializeQueue.isEmpty()) {
        PluginSpec *spec = delayedInitializeQueue.takeFirst();
        profilingReport(">delayedInitialize", spec);
        bool delay = spec->d->delayedInitialize();
        profilingReport("<delayedInitialize", spec);
        if (delay)
            break; // do next delayedInitialize after a delay
    }
    if (delayedInitializeQueue.isEmpty()) {
        delete delayedInitializeTimer;
        delayedInitializeTimer = 0;
        emit q->initializationDone();
    } else {
        delayedInitializeTimer->start();
    }
}

/*!
    \fn PluginManagerPrivate::PluginManagerPrivate(PluginManager *pluginManager)
    \internal
*/
PluginManagerPrivate::PluginManagerPrivate(PluginManager *pluginManager) :
    extension(QLatin1String("xml")),
    delayedInitializeTimer(0),
    shutdownEventLoop(0),
    m_profileElapsedMS(0),
    m_profilingVerbosity(0),
    settings(0),
    globalSettings(0),
    q(pluginManager)
{
}


/*!
    \fn PluginManagerPrivate::~PluginManagerPrivate()
    \internal
*/
PluginManagerPrivate::~PluginManagerPrivate()
{
    qDeleteAll(pluginSpecs);
    qDeleteAll(pluginCategories);
}

/*!
    \fn void PluginManagerPrivate::writeSettings()
    \internal
*/
void PluginManagerPrivate::writeSettings()
{
    if (!settings)
        return;
    QStringList tempDisabledPlugins;
    QStringList tempForceEnabledPlugins;
    foreach (PluginSpec *spec, pluginSpecs) {
        if (!spec->isDisabledByDefault() && !spec->isEnabled())
            tempDisabledPlugins.append(spec->name());
        if (spec->isDisabledByDefault() && spec->isEnabled())
            tempForceEnabledPlugins.append(spec->name());
    }

    settings->setValue(QLatin1String(C_IGNORED_PLUGINS), tempDisabledPlugins);
    settings->setValue(QLatin1String(C_FORCEENABLED_PLUGINS), tempForceEnabledPlugins);
}

/*!
    \fn void PluginManagerPrivate::readSettings()
    \internal
*/
void PluginManagerPrivate::readSettings()
{
    if (globalSettings)
        defaultDisabledPlugins = globalSettings->value(QLatin1String(C_IGNORED_PLUGINS)).toStringList();
    if (settings) {
        disabledPlugins = settings->value(QLatin1String(C_IGNORED_PLUGINS)).toStringList();
        forceEnabledPlugins = settings->value(QLatin1String(C_FORCEENABLED_PLUGINS)).toStringList();
    }
}

/*!
    \fn void PluginManagerPrivate::stopAll()
    \internal
*/
void PluginManagerPrivate::stopAll()
{
    if (delayedInitializeTimer && delayedInitializeTimer->isActive()) {
        delayedInitializeTimer->stop();
        delete delayedInitializeTimer;
        delayedInitializeTimer = 0;
    }
    QList<PluginSpec *> queue = loadQueue();
    foreach (PluginSpec *spec, queue) {
        loadPlugin(spec, PluginSpec::Stopped);
    }
}

/*!
    \fn void PluginManagerPrivate::deleteAll()
    \internal
*/
void PluginManagerPrivate::deleteAll()
{
    QList<PluginSpec *> queue = loadQueue();
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

        if (m_profilingVerbosity && !m_profileTimer.isNull()) {
            // Report a timestamp when adding an object. Useful for profiling
            // its initialization time.
            const int absoluteElapsedMS = m_profileTimer->elapsed();
            qDebug("  %-43s %8dms", obj->metaObject()->className(), absoluteElapsedMS);
        }

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
        PluginSpec *spec = it.previous();
        loadPlugin(spec, PluginSpec::Running);
        if (spec->state() == PluginSpec::Running)
            delayedInitializeQueue.append(spec);
    }
    emit q->pluginsChanged();

    delayedInitializeTimer = new QTimer;
    delayedInitializeTimer->setInterval(DELAYED_INITIALIZE_INTERVAL);
    delayedInitializeTimer->setSingleShot(true);
    connect(delayedInitializeTimer, SIGNAL(timeout()),
            this, SLOT(nextDelayedInitialize()));
    delayedInitializeTimer->start();
}

/*!
    \fn void PluginManagerPrivate::shutdown()
    \internal
*/
void PluginManagerPrivate::shutdown()
{
    stopAll();
    if (!asynchronousPlugins.isEmpty()) {
        shutdownEventLoop = new QEventLoop;
        shutdownEventLoop->exec();
    }
    deleteAll();
    if (!allObjects.isEmpty())
        qDebug() << "There are" << allObjects.size() << "objects left in the plugin manager pool: " << allObjects;
}

/*!
    \fn void PluginManagerPrivate::asyncShutdownFinished()
    \internal
*/
void PluginManagerPrivate::asyncShutdownFinished()
{
    IPlugin *plugin = qobject_cast<IPlugin *>(sender());
    Q_ASSERT(plugin);
    asynchronousPlugins.removeAll(plugin->pluginSpec());
    if (asynchronousPlugins.isEmpty())
        shutdownEventLoop->exit();
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
        spec->d->errorString = PluginManager::tr("Circular dependency detected:\n");
        int index = circularityCheckQueue.indexOf(spec);
        for (int i = index; i < circularityCheckQueue.size(); ++i) {
            spec->d->errorString.append(PluginManager::tr("%1(%2) depends on\n")
                .arg(circularityCheckQueue.at(i)->name()).arg(circularityCheckQueue.at(i)->version()));
        }
        spec->d->errorString.append(PluginManager::tr("%1(%2)").arg(spec->name()).arg(spec->version()));
        return false;
    }
    circularityCheckQueue.append(spec);
    // check if we have the dependencies
    if (spec->state() == PluginSpec::Invalid || spec->state() == PluginSpec::Read) {
        queue.append(spec);
        return false;
    }

    // add dependencies
    foreach (PluginSpec *depSpec, spec->dependencySpecs()) {
        if (!loadQueue(depSpec, queue, circularityCheckQueue)) {
            spec->d->hasError = true;
            spec->d->errorString =
                PluginManager::tr("Cannot load plugin because dependency failed to load: %1(%2)\nReason: %3")
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
    if (spec->hasError() || spec->state() != destState-1)
        return;

    // don't load disabled plugins.
    if ((spec->isDisabledIndirectly() || !spec->isEnabled()) && destState == PluginSpec::Loaded)
        return;

    switch (destState) {
    case PluginSpec::Running:
        profilingReport(">initializeExtensions", spec);
        spec->d->initializeExtensions();
        profilingReport("<initializeExtensions", spec);
        return;
    case PluginSpec::Deleted:
        profilingReport(">delete", spec);
        spec->d->kill();
        profilingReport("<delete", spec);
        return;
    default:
        break;
    }
    // check if dependencies have loaded without error
    QHashIterator<PluginDependency, PluginSpec *> it(spec->dependencySpecs());
    while (it.hasNext()) {
        it.next();
        if (it.key().type == PluginDependency::Optional)
            continue;
        PluginSpec *depSpec = it.value();
        if (depSpec->state() != destState) {
            spec->d->hasError = true;
            spec->d->errorString =
                PluginManager::tr("Cannot load plugin because dependency failed to load: %1(%2)\nReason: %3")
                    .arg(depSpec->name()).arg(depSpec->version()).arg(depSpec->errorString());
            return;
        }
    }
    switch (destState) {
    case PluginSpec::Loaded:
        profilingReport(">loadLibrary", spec);
        spec->d->loadLibrary();
        profilingReport("<loadLibrary", spec);
        break;
    case PluginSpec::Initialized:
        profilingReport(">initializePlugin", spec);
        spec->d->initializePlugin();
        profilingReport("<initializePlugin", spec);
        break;
    case PluginSpec::Stopped:
        profilingReport(">stop", spec);
        if (spec->d->stop() == IPlugin::AsynchronousShutdown) {
            asynchronousPlugins << spec;
            connect(spec->plugin(), SIGNAL(asynchronousShutdownFinished()),
                    this, SLOT(asyncShutdownFinished()));
        }
        profilingReport("<stop", spec);
        break;
    default:
        break;
    }
}

/*!
    \fn void PluginManagerPrivate::setPluginPaths(const QStringList &paths)
    \internal
*/
void PluginManagerPrivate::setPluginPaths(const QStringList &paths)
{
    pluginPaths = paths;
    readSettings();
    readPluginPaths();
}

/*!
    \fn void PluginManagerPrivate::readPluginPaths()
    \internal
*/
void PluginManagerPrivate::readPluginPaths()
{
    qDeleteAll(pluginCategories);
    qDeleteAll(pluginSpecs);
    pluginSpecs.clear();
    pluginCategories.clear();

    QStringList specFiles;
    QStringList searchPaths = pluginPaths;
    while (!searchPaths.isEmpty()) {
        const QDir dir(searchPaths.takeFirst());
        const QString pattern = QLatin1String("*.") + extension;
        const QFileInfoList files = dir.entryInfoList(QStringList(pattern), QDir::Files);
        foreach (const QFileInfo &file, files)
            specFiles << file.absoluteFilePath();
        const QFileInfoList dirs = dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
        foreach (const QFileInfo &subdir, dirs)
            searchPaths << subdir.absoluteFilePath();
    }
    defaultCollection = new PluginCollection(QString());
    pluginCategories.insert(QString(), defaultCollection);

    foreach (const QString &specFile, specFiles) {
        PluginSpec *spec = new PluginSpec;
        spec->d->read(specFile);

        PluginCollection *collection = 0;
        // find correct plugin collection or create a new one
        if (pluginCategories.contains(spec->category()))
            collection = pluginCategories.value(spec->category());
        else {
            collection = new PluginCollection(spec->category());
            pluginCategories.insert(spec->category(), collection);
        }
        if (defaultDisabledPlugins.contains(spec->name())) {
            spec->setDisabledByDefault(true);
            spec->setEnabled(false);
        }
        if (spec->isDisabledByDefault() && forceEnabledPlugins.contains(spec->name()))
            spec->setEnabled(true);
        if (!spec->isDisabledByDefault() && disabledPlugins.contains(spec->name()))
            spec->setEnabled(false);

        collection->addPlugin(spec);
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
    foreach (PluginSpec *spec, loadQueue()) {
        spec->d->disableIndirectlyIfDependencyDisabled();
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

void PluginManagerPrivate::disablePluginIndirectly(PluginSpec *spec)
{
    spec->d->disabledIndirectly = true;
}

PluginSpec *PluginManagerPrivate::pluginByName(const QString &name) const
{
    foreach (PluginSpec *spec, pluginSpecs)
        if (spec->name() == name)
            return spec;
    return 0;
}

void PluginManagerPrivate::initProfiling()
{
    if (m_profileTimer.isNull()) {
        m_profileTimer.reset(new QTime);
        m_profileTimer->start();
        m_profileElapsedMS = 0;
        qDebug("Profiling started");
    } else {
        m_profilingVerbosity++;
    }
}

void PluginManagerPrivate::profilingReport(const char *what, const PluginSpec *spec /* = 0 */)
{
    if (!m_profileTimer.isNull()) {
        const int absoluteElapsedMS = m_profileTimer->elapsed();
        const int elapsedMS = absoluteElapsedMS - m_profileElapsedMS;
        m_profileElapsedMS = absoluteElapsedMS;
        if (spec)
            qDebug("%-22s %-22s %8dms (%8dms)", what, qPrintable(spec->name()), absoluteElapsedMS, elapsedMS);
        else
            qDebug("%-45s %8dms (%8dms)", what, absoluteElapsedMS, elapsedMS);
    }
}

/*!
    \fn void PluginManager::getObjectByName(const QString &name) const
    \brief Retrieves one object with a given name from the object pool.
    \sa addObject()
*/

QObject *PluginManager::getObjectByName(const QString &name)
{
    QReadLocker lock(&m_instance->m_lock);
    QList<QObject *> all = allObjects();
    foreach (QObject *obj, all) {
        if (obj->objectName() == name)
            return obj;
    }
    return 0;
}

/*!
    \fn void PluginManager::getObjectByClassName(const QString &className) const
    Retrieves one object inheriting a class with a given name from the object pool.
    \sa addObject()
*/

QObject *PluginManager::getObjectByClassName(const QString &className)
{
    const QByteArray ba = className.toUtf8();
    QReadLocker lock(&m_instance->m_lock);
    QList<QObject *> all = allObjects();
    foreach (QObject *obj, all) {
        if (obj->inherits(ba.constData()))
            return obj;
    }
    return 0;
}
