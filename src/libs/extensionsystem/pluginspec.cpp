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

#include "pluginspec.h"

#include "pluginspec_p.h"
#include "iplugin.h"
#include "iplugin_p.h"
#include "pluginmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QRegExp>
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_LINUX
// Using the patched version breaks on Fedora 10, KDE4.2.2/Qt4.5.
#   define USE_UNPATCHED_QPLUGINLOADER 1
#else
#   define USE_UNPATCHED_QPLUGINLOADER 1
#endif

#if USE_UNPATCHED_QPLUGINLOADER

#   include <QPluginLoader>
    typedef QT_PREPEND_NAMESPACE(QPluginLoader) PluginLoader;

#else

#   include "patchedpluginloader.cpp"
    typedef PatchedPluginLoader PluginLoader;

#endif

/*!
    \class ExtensionSystem::PluginDependency
    \brief Struct that contains the name and required compatible version number of a plugin's dependency.

    This reflects the data of a dependency tag in the plugin's xml description file.
    The name and version are used to resolve the dependency, i.e. a plugin with the given name and
    plugin \c {compatibility version <= dependency version <= plugin version} is searched for.

    See also ExtensionSystem::IPlugin for more information about plugin dependencies and
    version matching.
*/

/*!
    \variable ExtensionSystem::PluginDependency::name
    String identifier of the plugin.
*/

/*!
    \variable ExtensionSystem::PluginDependency::version
    Version string that a plugin must match to fill this dependency.
*/

/*!
    \variable ExtensionSystem::PluginDependency::type
    Defines whether the dependency is required or optional.
    \sa ExtensionSystem::PluginDependency::Type
*/

/*!
    \enum ExtensionSystem::PluginDependency::Type
    Whether the dependency is required or optional.
    \value Required
           Dependency needs to be there.
    \value Optional
           Dependency is not necessarily needed. You need to make sure that
           the plugin is able to load without this dependency installed, so
           for example you may not link to the dependency's library.
*/

/*!
    \class ExtensionSystem::PluginSpec
    \brief Contains the information of the plugins xml description file and
    information about the plugin's current state.

    The plugin spec is also filled with more information as the plugin
    goes through its loading process (see PluginSpec::State).
    If an error occurs, the plugin spec is the place to look for the
    error details.
*/

/*!
    \enum ExtensionSystem::PluginSpec::State

    The plugin goes through several steps while being loaded.
    The state gives a hint on what went wrong in case of an error.

    \value  Invalid
            Starting point: Even the xml description file was not read.
    \value  Read
            The xml description file has been successfully read, and its
            information is available via the PluginSpec.
    \value  Resolved
            The dependencies given in the description file have been
            successfully found, and are available via the dependencySpecs() method.
    \value  Loaded
            The plugin's library is loaded and the plugin instance created
            (available through plugin()).
    \value  Initialized
            The plugin instance's IPlugin::initialize() method has been called
            and returned a success value.
    \value  Running
            The plugin's dependencies are successfully initialized and
            extensionsInitialized has been called. The loading process is
            complete.
    \value Stopped
            The plugin has been shut down, i.e. the plugin's IPlugin::aboutToShutdown() method has been called.
    \value Deleted
            The plugin instance has been deleted.
*/

using namespace ExtensionSystem;
using namespace ExtensionSystem::Internal;

/*!
    \fn uint qHash(const ExtensionSystem::PluginDependency &value)
    \internal
*/
uint ExtensionSystem::qHash(const ExtensionSystem::PluginDependency &value)
{
    return qHash(value.name);
}

/*!
    \fn bool PluginDependency::operator==(const PluginDependency &other) const
    \internal
*/
bool PluginDependency::operator==(const PluginDependency &other) const
{
    return name == other.name && version == other.version && type == other.type;
}

/*!
    \fn PluginSpec::PluginSpec()
    \internal
*/
PluginSpec::PluginSpec()
    : d(new PluginSpecPrivate(this))
{
}

/*!
    \fn PluginSpec::~PluginSpec()
    \internal
*/
PluginSpec::~PluginSpec()
{
    delete d;
    d = 0;
}

/*!
    \fn QString PluginSpec::name() const
    The plugin name. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::name() const
{
    return d->name;
}

/*!
    \fn QString PluginSpec::version() const
    The plugin version. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::version() const
{
    return d->version;
}

/*!
    \fn QString PluginSpec::compatVersion() const
    The plugin compatibility version. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::compatVersion() const
{
    return d->compatVersion;
}

/*!
    \fn QString PluginSpec::vendor() const
    The plugin vendor. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::vendor() const
{
    return d->vendor;
}

/*!
    \fn QString PluginSpec::copyright() const
    The plugin copyright. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::copyright() const
{
    return d->copyright;
}

/*!
    \fn QString PluginSpec::license() const
    The plugin license. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::license() const
{
    return d->license;
}

/*!
    \fn QString PluginSpec::description() const
    The plugin description. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::description() const
{
    return d->description;
}

/*!
    \fn QString PluginSpec::url() const
    The plugin url where you can find more information about the plugin. This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::url() const
{
    return d->url;
}

/*!
    \fn QString PluginSpec::category() const
    The category that the plugin belongs to. Categories are groups of plugins which allow for keeping them together in the UI.
    Returns an empty string if the plugin does not belong to a category.
*/
QString PluginSpec::category() const
{
    return d->category;
}

/*!
    \fn bool PluginSpec::isExperimental() const
    Returns if the plugin has its experimental flag set.
*/
bool PluginSpec::isExperimental() const
{
    return d->experimental;
}

/*!
    Returns if the plugin is disabled by default.
    This might be because the plugin is experimental, or because
    the plugin manager's settings define it as disabled by default.
*/
bool PluginSpec::isDisabledByDefault() const
{
    return d->disabledByDefault;
}

/*!
    \fn bool PluginSpec::isEnabled() const
    Returns if the plugin is loaded at startup. True by default - the user can change it from the Plugin settings.
*/
bool PluginSpec::isEnabled() const
{
    return d->enabled;
}

/*!
    \fn bool PluginSpec::isDisabledIndirectly() const
    Returns true if loading was not done due to user unselecting this plugin or its dependencies,
    or if command-line parameter -noload was used.
*/
bool PluginSpec::isDisabledIndirectly() const
{
    return d->disabledIndirectly;
}

/*!
    \fn QList<PluginDependency> PluginSpec::dependencies() const
    The plugin dependencies. This is valid after the PluginSpec::Read state is reached.
*/
QList<PluginDependency> PluginSpec::dependencies() const
{
    return d->dependencies;
}

/*!
    \fn PluginSpec::PluginArgumentDescriptions PluginSpec::argumentDescriptions() const
    Returns a list of descriptions of command line arguments the plugin processes.
*/

PluginSpec::PluginArgumentDescriptions PluginSpec::argumentDescriptions() const
{
    return d->argumentDescriptions;
}

/*!
    \fn QString PluginSpec::location() const
    The absolute path to the directory containing the plugin xml description file
    this PluginSpec corresponds to.
*/
QString PluginSpec::location() const
{
    return d->location;
}

/*!
    \fn QString PluginSpec::filePath() const
    The absolute path to the plugin xml description file (including the file name)
    this PluginSpec corresponds to.
*/
QString PluginSpec::filePath() const
{
    return d->filePath;
}

/*!
    \fn QStringList PluginSpec::arguments() const
    Command line arguments specific to that plugin. Set at startup
*/

QStringList PluginSpec::arguments() const
{
    return d->arguments;
}

/*!
    \fn void PluginSpec::setArguments(const QStringList &arguments)
    Set the command line arguments specific to that plugin to \a arguments.
*/

void PluginSpec::setArguments(const QStringList &arguments)
{
    d->arguments = arguments;
}

/*!
    \fn PluginSpec::addArgument(const QString &argument)
    Adds \a argument to the command line arguments specific to that plugin.
*/

void PluginSpec::addArgument(const QString &argument)
{
    d->arguments.push_back(argument);
}


/*!
    \fn PluginSpec::State PluginSpec::state() const
    The state in which the plugin currently is.
    See the description of the PluginSpec::State enum for details.
*/
PluginSpec::State PluginSpec::state() const
{
    return d->state;
}

/*!
    \fn bool PluginSpec::hasError() const
    Returns whether an error occurred while reading/starting the plugin.
*/
bool PluginSpec::hasError() const
{
    return d->hasError;
}

/*!
    \fn QString PluginSpec::errorString() const
    Detailed, possibly multi-line, error description in case of an error.
*/
QString PluginSpec::errorString() const
{
    return d->errorString;
}

/*!
    \fn bool PluginSpec::provides(const QString &pluginName, const QString &version) const
    Returns if this plugin can be used to fill in a dependency of the given
    \a pluginName and \a version.

        \sa PluginSpec::dependencies()
*/
bool PluginSpec::provides(const QString &pluginName, const QString &version) const
{
    return d->provides(pluginName, version);
}

/*!
    \fn IPlugin *PluginSpec::plugin() const
    The corresponding IPlugin instance, if the plugin library has already been successfully loaded,
    i.e. the PluginSpec::Loaded state is reached.
*/
IPlugin *PluginSpec::plugin() const
{
    return d->plugin;
}

/*!
    \fn QList<PluginSpec *> PluginSpec::dependencySpecs() const
    Returns the list of dependencies, already resolved to existing plugin specs.
    Valid if PluginSpec::Resolved state is reached.

    \sa PluginSpec::dependencies()
*/
QHash<PluginDependency, PluginSpec *> PluginSpec::dependencySpecs() const
{
    return d->dependencySpecs;
}

//==========PluginSpecPrivate==================

namespace {
    const char PLUGIN[] = "plugin";
    const char PLUGIN_NAME[] = "name";
    const char PLUGIN_VERSION[] = "version";
    const char PLUGIN_COMPATVERSION[] = "compatVersion";
    const char PLUGIN_EXPERIMENTAL[] = "experimental";
    const char PLUGIN_DISABLED_BY_DEFAULT[] = "disabledByDefault";
    const char VENDOR[] = "vendor";
    const char COPYRIGHT[] = "copyright";
    const char LICENSE[] = "license";
    const char DESCRIPTION[] = "description";
    const char URL[] = "url";
    const char CATEGORY[] = "category";
    const char DEPENDENCYLIST[] = "dependencyList";
    const char DEPENDENCY[] = "dependency";
    const char DEPENDENCY_NAME[] = "name";
    const char DEPENDENCY_VERSION[] = "version";
    const char DEPENDENCY_TYPE[] = "type";
    const char DEPENDENCY_TYPE_SOFT[] = "optional";
    const char DEPENDENCY_TYPE_HARD[] = "required";
    const char ARGUMENTLIST[] = "argumentList";
    const char ARGUMENT[] = "argument";
    const char ARGUMENT_NAME[] = "name";
    const char ARGUMENT_PARAMETER[] = "parameter";
}
/*!
    \fn PluginSpecPrivate::PluginSpecPrivate(PluginSpec *spec)
    \internal
*/
PluginSpecPrivate::PluginSpecPrivate(PluginSpec *spec)
    :
    experimental(false),
    disabledByDefault(false),
    enabled(true),
    disabledIndirectly(false),
    plugin(0),
    state(PluginSpec::Invalid),
    hasError(false),
    q(spec)
{
}

/*!
    \fn bool PluginSpecPrivate::read(const QString &fileName)
    \internal
*/
bool PluginSpecPrivate::read(const QString &fileName)
{
    name
        = version
        = compatVersion
        = vendor
        = copyright
        = license
        = description
        = url
        = category
        = location
        = QString();
    state = PluginSpec::Invalid;
    hasError = false;
    errorString.clear();
    dependencies.clear();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return reportError(tr("Cannot open file %1 for reading: %2")
                           .arg(QDir::toNativeSeparators(file.fileName()), file.errorString()));
    QFileInfo fileInfo(file);
    location = fileInfo.absolutePath();
    filePath = fileInfo.absoluteFilePath();
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            readPluginSpec(reader);
            break;
        default:
            break;
        }
    }
    if (reader.hasError())
        return reportError(tr("Error parsing file %1: %2, at line %3, column %4")
                .arg(QDir::toNativeSeparators(file.fileName()))
                .arg(reader.errorString())
                .arg(reader.lineNumber())
                .arg(reader.columnNumber()));
    state = PluginSpec::Read;
    return true;
}

void PluginSpec::setEnabled(bool value)
{
    d->enabled = value;
}

void PluginSpec::setDisabledByDefault(bool value)
{
    d->disabledByDefault = value;
}

void PluginSpec::setDisabledIndirectly(bool value)
{
    d->disabledIndirectly = value;
}

/*!
    \fn bool PluginSpecPrivate::reportError(const QString &err)
    \internal
*/
bool PluginSpecPrivate::reportError(const QString &err)
{
    errorString = err;
    hasError = true;
    return false;
}

static inline QString msgAttributeMissing(const char *elt, const char *attribute)
{
    return QCoreApplication::translate("PluginSpec", "'%1' misses attribute '%2'").arg(QLatin1String(elt), QLatin1String(attribute));
}

static inline QString msgInvalidFormat(const char *content)
{
    return QCoreApplication::translate("PluginSpec", "'%1' has invalid format").arg(QLatin1String(content));
}

static inline QString msgInvalidElement(const QString &name)
{
    return QCoreApplication::translate("PluginSpec", "Invalid element '%1'").arg(name);
}

static inline QString msgUnexpectedClosing(const QString &name)
{
    return QCoreApplication::translate("PluginSpec", "Unexpected closing element '%1'").arg(name);
}

static inline QString msgUnexpectedToken()
{
    return QCoreApplication::translate("PluginSpec", "Unexpected token");
}

/*!
    \fn void PluginSpecPrivate::readPluginSpec(QXmlStreamReader &reader)
    \internal
*/
void PluginSpecPrivate::readPluginSpec(QXmlStreamReader &reader)
{
    QString element = reader.name().toString();
    if (element != QLatin1String(PLUGIN)) {
        reader.raiseError(QCoreApplication::translate("PluginSpec", "Expected element '%1' as top level element")
                          .arg(QLatin1String(PLUGIN)));
        return;
    }
    name = reader.attributes().value(QLatin1String(PLUGIN_NAME)).toString();
    if (name.isEmpty()) {
        reader.raiseError(msgAttributeMissing(PLUGIN, PLUGIN_NAME));
        return;
    }
    version = reader.attributes().value(QLatin1String(PLUGIN_VERSION)).toString();
    if (version.isEmpty()) {
        reader.raiseError(msgAttributeMissing(PLUGIN, PLUGIN_VERSION));
        return;
    }
    if (!isValidVersion(version)) {
        reader.raiseError(msgInvalidFormat(PLUGIN_VERSION));
        return;
    }
    compatVersion = reader.attributes().value(QLatin1String(PLUGIN_COMPATVERSION)).toString();
    if (!compatVersion.isEmpty() && !isValidVersion(compatVersion)) {
        reader.raiseError(msgInvalidFormat(PLUGIN_COMPATVERSION));
        return;
    } else if (compatVersion.isEmpty()) {
        compatVersion = version;
    }
    disabledByDefault = readBooleanValue(reader, PLUGIN_DISABLED_BY_DEFAULT);
    experimental = readBooleanValue(reader, PLUGIN_EXPERIMENTAL);
    if (reader.hasError())
        return;
    if (experimental)
        disabledByDefault = true;
    enabled = !disabledByDefault;
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            element = reader.name().toString();
            if (element == QLatin1String(VENDOR))
                vendor = reader.readElementText().trimmed();
            else if (element == QLatin1String(COPYRIGHT))
                copyright = reader.readElementText().trimmed();
            else if (element == QLatin1String(LICENSE))
                license = reader.readElementText().trimmed();
            else if (element == QLatin1String(DESCRIPTION))
                description = reader.readElementText().trimmed();
            else if (element == QLatin1String(URL))
                url = reader.readElementText().trimmed();
            else if (element == QLatin1String(CATEGORY))
                category = reader.readElementText().trimmed();
            else if (element == QLatin1String(DEPENDENCYLIST))
                readDependencies(reader);
            else if (element == QLatin1String(ARGUMENTLIST))
                readArgumentDescriptions(reader);
            else
                reader.raiseError(msgInvalidElement(name));
            break;
        case QXmlStreamReader::EndDocument:
        case QXmlStreamReader::Comment:
        case QXmlStreamReader::EndElement:
        case QXmlStreamReader::Characters:
            break;
        default:
            reader.raiseError(msgUnexpectedToken());
            break;
        }
    }
}

/*!
    \fn void PluginSpecPrivate::readArgumentDescriptions(QXmlStreamReader &reader)
    \internal
*/

void PluginSpecPrivate::readArgumentDescriptions(QXmlStreamReader &reader)
{
    QString element;
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            element = reader.name().toString();
            if (element == QLatin1String(ARGUMENT))
                readArgumentDescription(reader);
            else
                reader.raiseError(msgInvalidElement(name));
            break;
        case QXmlStreamReader::Comment:
        case QXmlStreamReader::Characters:
            break;
        case QXmlStreamReader::EndElement:
            element = reader.name().toString();
            if (element == QLatin1String(ARGUMENTLIST))
                return;
            reader.raiseError(msgUnexpectedClosing(element));
            break;
        default:
            reader.raiseError(msgUnexpectedToken());
            break;
        }
    }
}

/*!
    \fn void PluginSpecPrivate::readArgumentDescription(QXmlStreamReader &reader)
    \internal
*/
void PluginSpecPrivate::readArgumentDescription(QXmlStreamReader &reader)
{
    PluginArgumentDescription arg;
    arg.name = reader.attributes().value(QLatin1String(ARGUMENT_NAME)).toString();
    if (arg.name.isEmpty()) {
        reader.raiseError(msgAttributeMissing(ARGUMENT, ARGUMENT_NAME));
        return;
    }
    arg.parameter = reader.attributes().value(QLatin1String(ARGUMENT_PARAMETER)).toString();
    arg.description = reader.readElementText();
    if (reader.tokenType() != QXmlStreamReader::EndElement)
        reader.raiseError(msgUnexpectedToken());
    argumentDescriptions.push_back(arg);
}

bool PluginSpecPrivate::readBooleanValue(QXmlStreamReader &reader, const char *key)
{
    const QString valueString = reader.attributes().value(QLatin1String(key)).toString();
    const bool isOn = valueString.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    if (!valueString.isEmpty() && !isOn
            && valueString.compare(QLatin1String("false"), Qt::CaseInsensitive) != 0) {
        reader.raiseError(msgInvalidFormat(key));
    }
    return isOn;
}

/*!
    \fn void PluginSpecPrivate::readDependencies(QXmlStreamReader &reader)
    \internal
*/
void PluginSpecPrivate::readDependencies(QXmlStreamReader &reader)
{
    QString element;
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            element = reader.name().toString();
            if (element == QLatin1String(DEPENDENCY))
                readDependencyEntry(reader);
            else
                reader.raiseError(msgInvalidElement(name));
            break;
        case QXmlStreamReader::Comment:
        case QXmlStreamReader::Characters:
            break;
        case QXmlStreamReader::EndElement:
            element = reader.name().toString();
            if (element == QLatin1String(DEPENDENCYLIST))
                return;
            reader.raiseError(msgUnexpectedClosing(element));
            break;
        default:
            reader.raiseError(msgUnexpectedToken());
            break;
        }
    }
}

/*!
    \fn void PluginSpecPrivate::readDependencyEntry(QXmlStreamReader &reader)
    \internal
*/
void PluginSpecPrivate::readDependencyEntry(QXmlStreamReader &reader)
{
    PluginDependency dep;
    dep.name = reader.attributes().value(QLatin1String(DEPENDENCY_NAME)).toString();
    if (dep.name.isEmpty()) {
        reader.raiseError(msgAttributeMissing(DEPENDENCY, DEPENDENCY_NAME));
        return;
    }
    dep.version = reader.attributes().value(QLatin1String(DEPENDENCY_VERSION)).toString();
    if (!dep.version.isEmpty() && !isValidVersion(dep.version)) {
        reader.raiseError(msgInvalidFormat(DEPENDENCY_VERSION));
        return;
    }
    dep.type = PluginDependency::Required;
    if (reader.attributes().hasAttribute(QLatin1String(DEPENDENCY_TYPE))) {
        QString typeValue = reader.attributes().value(QLatin1String(DEPENDENCY_TYPE)).toString();
        if (typeValue == QLatin1String(DEPENDENCY_TYPE_HARD)) {
            dep.type = PluginDependency::Required;
        } else if (typeValue == QLatin1String(DEPENDENCY_TYPE_SOFT)) {
            dep.type = PluginDependency::Optional;
        } else {
            reader.raiseError(msgInvalidFormat(DEPENDENCY_TYPE));
            return;
        }
    }
    dependencies.append(dep);
    reader.readNext();
    if (reader.tokenType() != QXmlStreamReader::EndElement)
        reader.raiseError(msgUnexpectedToken());
}

/*!
    \fn bool PluginSpecPrivate::provides(const QString &pluginName, const QString &pluginVersion) const
    \internal
*/
bool PluginSpecPrivate::provides(const QString &pluginName, const QString &pluginVersion) const
{
    if (QString::compare(pluginName, name, Qt::CaseInsensitive) != 0)
        return false;
    return (versionCompare(version, pluginVersion) >= 0) && (versionCompare(compatVersion, pluginVersion) <= 0);
}

/*!
    \fn QRegExp &PluginSpecPrivate::versionRegExp()
    \internal
*/
QRegExp &PluginSpecPrivate::versionRegExp()
{
    static QRegExp reg(QLatin1String("([0-9]+)(?:[.]([0-9]+))?(?:[.]([0-9]+))?(?:_([0-9]+))?"));
    return reg;
}
/*!
    \fn bool PluginSpecPrivate::isValidVersion(const QString &version)
    \internal
*/
bool PluginSpecPrivate::isValidVersion(const QString &version)
{
    return versionRegExp().exactMatch(version);
}

/*!
    \fn int PluginSpecPrivate::versionCompare(const QString &version1, const QString &version2)
    \internal
*/
int PluginSpecPrivate::versionCompare(const QString &version1, const QString &version2)
{
    QRegExp reg1 = versionRegExp();
    QRegExp reg2 = versionRegExp();
    if (!reg1.exactMatch(version1))
        return 0;
    if (!reg2.exactMatch(version2))
        return 0;
    int number1;
    int number2;
    for (int i = 0; i < 4; ++i) {
        number1 = reg1.cap(i+1).toInt();
        number2 = reg2.cap(i+1).toInt();
        if (number1 < number2)
            return -1;
        if (number1 > number2)
            return 1;
    }
    return 0;
}

/*!
    \fn bool PluginSpecPrivate::resolveDependencies(const QList<PluginSpec *> &specs)
    \internal
*/
bool PluginSpecPrivate::resolveDependencies(const QList<PluginSpec *> &specs)
{
    if (hasError)
        return false;
    if (state == PluginSpec::Resolved)
        state = PluginSpec::Read; // Go back, so we just re-resolve the dependencies.
    if (state != PluginSpec::Read) {
        errorString = QCoreApplication::translate("PluginSpec", "Resolving dependencies failed because state != Read");
        hasError = true;
        return false;
    }
    QHash<PluginDependency, PluginSpec *> resolvedDependencies;
    foreach (const PluginDependency &dependency, dependencies) {
        PluginSpec *found = 0;

        foreach (PluginSpec *spec, specs) {
            if (spec->provides(dependency.name, dependency.version)) {
                found = spec;
                break;
            }
        }
        if (!found) {
            if (dependency.type == PluginDependency::Required) {
                hasError = true;
                if (!errorString.isEmpty())
                    errorString.append(QLatin1Char('\n'));
                errorString.append(QCoreApplication::translate("PluginSpec", "Could not resolve dependency '%1(%2)'")
                    .arg(dependency.name).arg(dependency.version));
            }
            continue;
        }
        resolvedDependencies.insert(dependency, found);
    }
    if (hasError)
        return false;

    dependencySpecs = resolvedDependencies;

    state = PluginSpec::Resolved;

    return true;
}

void PluginSpecPrivate::disableIndirectlyIfDependencyDisabled()
{
    if (!enabled)
        return;

    if (disabledIndirectly)
        return;

    QHashIterator<PluginDependency, PluginSpec *> it(dependencySpecs);
    while (it.hasNext()) {
        it.next();
        if (it.key().type == PluginDependency::Optional)
            continue;
        PluginSpec *dependencySpec = it.value();
        if (dependencySpec->isDisabledIndirectly() || !dependencySpec->isEnabled()) {
            disabledIndirectly = true;
            break;
        }
    }
}

/*!
    \fn bool PluginSpecPrivate::loadLibrary()
    \internal
*/
bool PluginSpecPrivate::loadLibrary()
{
    if (hasError)
        return false;
    if (state != PluginSpec::Resolved) {
        if (state == PluginSpec::Loaded)
            return true;
        errorString = QCoreApplication::translate("PluginSpec", "Loading the library failed because state != Resolved");
        hasError = true;
        return false;
    }
#ifdef QT_NO_DEBUG

#ifdef Q_OS_WIN
    QString libName = QString::fromLatin1("%1/%2.dll").arg(location).arg(name);
#elif defined(Q_OS_MAC)
    QString libName = QString::fromLatin1("%1/lib%2.dylib").arg(location).arg(name);
#else
    QString libName = QString::fromLatin1("%1/lib%2.so").arg(location).arg(name);
#endif

#else //Q_NO_DEBUG

#ifdef Q_OS_WIN
    QString libName = QString::fromLatin1("%1/%2d.dll").arg(location).arg(name);
#elif defined(Q_OS_MAC)
    QString libName = QString::fromLatin1("%1/lib%2_debug.dylib").arg(location).arg(name);
#else
    QString libName = QString::fromLatin1("%1/lib%2.so").arg(location).arg(name);
#endif

#endif

    PluginLoader loader(libName);
    if (!loader.load()) {
        hasError = true;
        errorString = QDir::toNativeSeparators(libName)
            + QString::fromLatin1(": ") + loader.errorString();
        return false;
    }
    IPlugin *pluginObject = qobject_cast<IPlugin*>(loader.instance());
    if (!pluginObject) {
        hasError = true;
        errorString = QCoreApplication::translate("PluginSpec", "Plugin is not valid (does not derive from IPlugin)");
        loader.unload();
        return false;
    }
    state = PluginSpec::Loaded;
    plugin = pluginObject;
    plugin->d->pluginSpec = q;
    return true;
}

/*!
    \fn bool PluginSpecPrivate::initializePlugin()
    \internal
*/
bool PluginSpecPrivate::initializePlugin()
{
    if (hasError)
        return false;
    if (state != PluginSpec::Loaded) {
        if (state == PluginSpec::Initialized)
            return true;
        errorString = QCoreApplication::translate("PluginSpec", "Initializing the plugin failed because state != Loaded");
        hasError = true;
        return false;
    }
    if (!plugin) {
        errorString = QCoreApplication::translate("PluginSpec", "Internal error: have no plugin instance to initialize");
        hasError = true;
        return false;
    }
    QString err;
    if (!plugin->initialize(arguments, &err)) {
        errorString = QCoreApplication::translate("PluginSpec", "Plugin initialization failed: %1").arg(err);
        hasError = true;
        return false;
    }
    state = PluginSpec::Initialized;
    return true;
}

/*!
    \fn bool PluginSpecPrivate::initializeExtensions()
    \internal
*/
bool PluginSpecPrivate::initializeExtensions()
{
    if (hasError)
        return false;
    if (state != PluginSpec::Initialized) {
        if (state == PluginSpec::Running)
            return true;
        errorString = QCoreApplication::translate("PluginSpec", "Cannot perform extensionsInitialized because state != Initialized");
        hasError = true;
        return false;
    }
    if (!plugin) {
        errorString = QCoreApplication::translate("PluginSpec", "Internal error: have no plugin instance to perform extensionsInitialized");
        hasError = true;
        return false;
    }
    plugin->extensionsInitialized();
    state = PluginSpec::Running;
    return true;
}

/*!
    \fn bool PluginSpecPrivate::delayedInitialize()
    \internal
*/
bool PluginSpecPrivate::delayedInitialize()
{
    if (hasError)
        return false;
    if (state != PluginSpec::Running)
        return false;
    if (!plugin) {
        errorString = QCoreApplication::translate("PluginSpec", "Internal error: have no plugin instance to perform delayedInitialize");
        hasError = true;
        return false;
    }
    return plugin->delayedInitialize();
}

/*!
    \fn bool PluginSpecPrivate::stop()
    \internal
*/
IPlugin::ShutdownFlag PluginSpecPrivate::stop()
{
    if (!plugin)
        return IPlugin::SynchronousShutdown;
    state = PluginSpec::Stopped;
    return plugin->aboutToShutdown();
}

/*!
    \fn bool PluginSpecPrivate::kill()
    \internal
*/
void PluginSpecPrivate::kill()
{
    if (!plugin)
        return;
    delete plugin;
    plugin = 0;
    state = PluginSpec::Deleted;
}
