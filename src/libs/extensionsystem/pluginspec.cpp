// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pluginspec.h"

#include "extensionsystemtr.h"
#include "iplugin.h"
#include "pluginmanager.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPluginLoader>

Q_LOGGING_CATEGORY(pluginSpecLog, "qtc.extensionsystem.plugin", QtWarningMsg)

using namespace ExtensionSystem::Internal;
using namespace Utils;

namespace ExtensionSystem {

/*!
    \class ExtensionSystem::PluginDependency
    \inheaderfile extensionsystem/pluginspec.h
    \inmodule QtCreator

    \brief The PluginDependency class contains the name and required compatible
    version number of a plugin's dependency.

    This reflects the data of a dependency object in the plugin's meta data.
    The name and version are used to resolve the dependency. That is,
    a plugin with the given name and
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
    \value Test
           Dependency needs to be force-loaded for running tests of the plugin.
*/

/*!
    \class ExtensionSystem::PluginSpec
    \inheaderfile extensionsystem/pluginspec.h
    \inmodule QtCreator

    \brief The PluginSpec class contains the information of the plugin's embedded meta data
    and information about the plugin's current state.

    The plugin spec is also filled with more information as the plugin
    goes through its loading process (see PluginSpec::State).
    If an error occurs, the plugin spec is the place to look for the
    error details.
*/

/*!
    \enum ExtensionSystem::PluginSpec::State
    The State enum indicates the states the plugin goes through while
    it is being loaded.

    The state gives a hint on what went wrong in case of an error.

    \value  Invalid
            Starting point: Even the plugin meta data was not read.
    \value  Read
            The plugin meta data has been successfully read, and its
            information is available via the PluginSpec.
    \value  Resolved
            The dependencies given in the description file have been
            successfully found, and are available via the dependencySpecs() function.
    \value  Loaded
            The plugin's library is loaded and the plugin instance created
            (available through plugin()).
    \value  Initialized
            The plugin instance's IPlugin::initialize() function has been called
            and returned a success value.
    \value  Running
            The plugin's dependencies are successfully initialized and
            extensionsInitialized has been called. The loading process is
            complete.
    \value Stopped
            The plugin has been shut down, i.e. the plugin's IPlugin::aboutToShutdown() function has been called.
    \value Deleted
            The plugin instance has been deleted.
*/

/*!
    \class ExtensionSystem::PluginArgumentDescription
    \inheaderfile extensionsystem/pluginspec.h
    \inmodule QtCreator

    \brief The PluginArgumentDescriptions class holds a list of descriptions of
    command line arguments that a plugin processes.

    \sa PluginSpec::argumentDescriptions()
*/

/*!
    \fn uint ExtensionSystem::qHash(const ExtensionSystem::PluginDependency &value)
    \internal
*/
size_t qHash(const PluginDependency &value)
{
    return qHash(value.name);
}

/*!
    \internal
*/
bool PluginDependency::operator==(const PluginDependency &other) const
{
    return name == other.name && version == other.version && type == other.type;
}

static QString typeString(PluginDependency::Type type)
{
    switch (type) {
    case PluginDependency::Optional:
        return QString(", optional");
    case PluginDependency::Test:
        return QString(", test");
    case PluginDependency::Required:
    default:
        return QString();
    }
}

/*!
    \internal
*/
QString PluginDependency::toString() const
{
    return name + " (" + version + typeString(type) + ")";
}

namespace Internal {
class PluginSpecImplPrivate
{
public:
    std::optional<QPluginLoader> loader;
    std::optional<QStaticPlugin> staticPlugin;

    IPlugin *plugin = nullptr;
};

class PluginSpecPrivate
{
public:
    ExtensionSystem::PerformanceData performanceData;

    QString name;
    QString version;
    QString compatVersion;
    QString vendor;
    QString category;
    QString description;
    QString longDescription;
    QString url;
    QString documentationUrl;
    QString license;
    QString revision;
    QString copyright;
    QStringList arguments;
    QRegularExpression platformSpecification;
    QVector<ExtensionSystem::PluginDependency> dependencies;

    PluginSpec::PluginArgumentDescriptions argumentDescriptions;
    FilePath location;
    FilePath filePath;

    bool experimental{false};
    bool deprecated{false};
    bool required{false};

    bool enabledByDefault{false};
    bool enabledBySettings{true};
    bool enabledIndirectly{false};
    bool forceEnabled{false};
    bool forceDisabled{false};
    bool softLoadable{false};

    std::optional<QString> errorString;

    PluginSpec::State state;
    QHash<PluginDependency, PluginSpec *> dependencySpecs;

    QJsonObject metaData;

    Utils::expected_str<void> readMetaData(const QJsonObject &metaData);
    Utils::expected_str<void> reportError(const QString &error)
    {
        errorString = error;
        return {};
    };
};
} // namespace Internal

/*!
    \internal
*/
CppPluginSpec::CppPluginSpec()
    : d(new PluginSpecImplPrivate)
{}

/*!
    \internal
*/
CppPluginSpec::~CppPluginSpec() = default;

/*!
    Returns the plugin name. This is valid after the PluginSpec::Read state is
    reached.
*/
QString PluginSpec::name() const
{
    return d->name;
}

/*!
    Returns the plugin version. This is valid after the PluginSpec::Read state
    is reached.
*/
QString PluginSpec::version() const
{
    return d->version;
}

/*!
    Returns the plugin compatibility version. This is valid after the
    PluginSpec::Read state is reached.
*/
QString PluginSpec::compatVersion() const
{
    return d->compatVersion;
}

/*!
    Returns the plugin vendor. This is valid after the PluginSpec::Read
    state is reached.
*/
QString PluginSpec::vendor() const
{
    return d->vendor;
}

/*!
    Returns the plugin copyright. This is valid after the PluginSpec::Read
     state is reached.
*/
QString PluginSpec::copyright() const
{
    return d->copyright;
}

/*!
    Returns the plugin license. This is valid after the PluginSpec::Read
    state is reached.
*/
QString PluginSpec::license() const
{
    return d->license;
}

/*!
    Returns the plugin description. This is valid after the PluginSpec::Read
    state is reached.
*/
QString PluginSpec::description() const
{
    return d->description;
}

/*!
    Returns the plugin's long description. This is valid after the PluginSpec::Read
    state is reached.
*/
QString PluginSpec::longDescription() const
{
    return d->longDescription;
}

/*!
    Returns the plugin URL where you can find more information about the plugin.
    This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::url() const
{
    return d->url;
}

/*!
    Returns the documentation URL where you can find the online manual about the plugin.
    This is valid after the PluginSpec::Read state is reached.
*/
QString PluginSpec::documentationUrl() const
{
    return d->documentationUrl;
}

/*!
    Returns the category that the plugin belongs to. Categories are used to
    group plugins together in the UI.
    Returns an empty string if the plugin does not belong to a category.
*/
QString PluginSpec::category() const
{
    return d->category;
}

QString PluginSpec::revision() const
{
    return d->revision;
}

/*!
    Returns a QRegularExpression matching the platforms this plugin works on.
    An empty pattern implies all platforms.
    \since 3.0
*/

QRegularExpression PluginSpec::platformSpecification() const
{
    return d->platformSpecification;
}

/*!
    Returns whether the plugin works on the host platform.
*/
bool PluginSpec::isAvailableForHostPlatform() const
{
    return d->platformSpecification.pattern().isEmpty()
            || d->platformSpecification.match(PluginManager::platformName()).hasMatch();
}

/*!
    Returns whether the plugin is required.
*/
bool PluginSpec::isRequired() const
{
    return d->required;
}

/*!
    Returns whether the plugin has its experimental flag set.
*/
bool PluginSpec::isExperimental() const
{
    return d->experimental;
}

/*!
    Returns whether the plugin has its deprecated flag set.
*/
bool PluginSpec::isDeprecated() const
{
    return d->deprecated;
}

/*!
    Returns whether the plugin is enabled by default.
    A plugin might be disabled because the plugin is experimental or deprecated, or because
    the installation settings define it as disabled by default.
*/
bool PluginSpec::isEnabledByDefault() const
{
    return d->enabledByDefault;
}

/*!
    Returns whether the plugin should be loaded at startup,
    taking into account the default enabled state, and the user's settings.

    \note This function might return \c false even if the plugin is loaded
    as a requirement of another enabled plugin.

    \sa isEffectivelyEnabled()
*/
bool PluginSpec::isEnabledBySettings() const
{
    return d->enabledBySettings;
}

/*!
    Returns whether the plugin is loaded at startup.
    \sa isEnabledBySettings()
*/
bool PluginSpec::isEffectivelyEnabled() const
{
    if (!isAvailableForHostPlatform())
        return false;
    if (isForceEnabled() || isEnabledIndirectly())
        return true;
    if (isForceDisabled())
        return false;
    return isEnabledBySettings();
}

/*!
    Returns \c true if loading was not done due to user unselecting this
    plugin or its dependencies.
*/
bool PluginSpec::isEnabledIndirectly() const
{
    return d->enabledIndirectly;
}

/*!
    Returns whether the plugin is enabled via the \c -load option on the
    command line.
*/
bool PluginSpec::isForceEnabled() const
{
    return d->forceEnabled;
}

/*!
    Returns whether the plugin is disabled via the \c -noload option on the
     command line.
*/
bool PluginSpec::isForceDisabled() const
{
    return d->forceDisabled;
}

/*!
    Returns whether the plugin is allowed to be loaded during runtime
    without a restart.
*/
bool PluginSpec::isSoftLoadable() const
{
    return d->softLoadable;
}

/*!
    The plugin dependencies. This is valid after the PluginSpec::Read state is reached.
*/
QVector<PluginDependency> PluginSpec::dependencies() const
{
    return d->dependencies;
}

/*!
    Returns the plugin meta data.
*/
QJsonObject PluginSpec::metaData() const
{
    return d->metaData;
}

PerformanceData &PluginSpec::performanceData() const
{
    return d->performanceData;
}

/*!
    Returns a list of descriptions of command line arguments the plugin processes.
*/

PluginSpec::PluginArgumentDescriptions PluginSpec::argumentDescriptions() const
{
    return d->argumentDescriptions;
}

/*!
    Returns the absolute path to the directory containing the plugin.
*/
FilePath PluginSpec::location() const
{
    return d->location;
}

/*!
    Returns the absolute path to the plugin.
*/
FilePath PluginSpec::filePath() const
{
    return d->filePath;
}

/*!
    Returns command line arguments specific to the plugin. Set at startup.
*/

QStringList PluginSpec::arguments() const
{
    return d->arguments;
}

/*!
    Sets the command line arguments specific to the plugin to \a arguments.
*/

void PluginSpec::setArguments(const QStringList &arguments)
{
    d->arguments = arguments;
}

/*!
    Adds \a argument to the command line arguments specific to the plugin.
*/

void PluginSpec::addArgument(const QString &argument)
{
    d->arguments.push_back(argument);
}


/*!
    Returns the state in which the plugin currently is.
    See the description of the PluginSpec::State enum for details.
*/
PluginSpec::State PluginSpec::state() const
{
    return d->state;
}

/*!
    Returns whether an error occurred while reading or starting the plugin.
*/
bool PluginSpec::hasError() const
{
    return d->errorString.has_value();
}

/*!
    Returns a detailed, possibly multi-line, error description in case of an
    error.
*/
QString PluginSpec::errorString() const
{
    return d->errorString.value_or(QString());
}

/*!
    Returns whether the plugin \a spec can be used to fill the \a dependency of this plugin.

    \sa PluginSpec::dependencies()
*/
bool PluginSpec::provides(PluginSpec *spec, const PluginDependency &dependency) const
{
    if (QString::compare(dependency.name, spec->name(), Qt::CaseInsensitive) != 0)
        return false;

    return (versionCompare(spec->version(), dependency.version) >= 0)
           && (versionCompare(spec->compatVersion(), dependency.version) <= 0);
}

/*!
    Returns the corresponding IPlugin instance, if the plugin library has
    already been successfully loaded. That is, the PluginSpec::Loaded state
    is reached.
*/
IPlugin *CppPluginSpec::plugin() const
{
    return d->plugin;
}

/*!
    Returns the list of dependencies, already resolved to existing plugin specs.
    Valid if PluginSpec::Resolved state is reached.

    \sa PluginSpec::dependencies()
*/
QHash<PluginDependency, PluginSpec *> PluginSpec::dependencySpecs() const
{
    return d->dependencySpecs;
}

/*!
    Returns whether the plugin requires any of the plugins specified by
    \a plugins.
*/
bool PluginSpec::requiresAny(const QSet<PluginSpec *> &plugins) const
{
    for (auto it = d->dependencySpecs.cbegin(); it != d->dependencySpecs.cend(); ++it) {
        if (it.key().type == PluginDependency::Required && plugins.contains(*it))
            return true;
    }
    return false;
}

void PluginSpec::setEnabledByDefault(bool value)
{
    d->enabledByDefault = value;
}

/*!
    Sets whether the plugin should be loaded at startup to \a value.

    \sa isEnabledBySettings()
*/
void PluginSpec::setEnabledBySettings(bool value)
{
    d->enabledBySettings = value;
}
void PluginSpec::setEnabledIndirectly(bool value)
{
    d->enabledIndirectly = value;
}
void PluginSpec::setForceDisabled(bool value)
{
    if (value)
        d->forceEnabled = false;
    d->forceDisabled = value;
}
void PluginSpec::setForceEnabled(bool value)
{
    if (value)
        d->forceDisabled = false;
    d->forceEnabled = value;
}

// returns the plugins that it actually indirectly enabled
PluginSpecs PluginSpec::enableDependenciesIndirectly(bool enableTestDependencies)
{
    if (!isEffectivelyEnabled()) // plugin not enabled, nothing to do
        return {};

    PluginSpecs enabled;
    for (auto it = d->dependencySpecs.cbegin(), end = d->dependencySpecs.cend(); it != end; ++it) {
        if (it.key().type != PluginDependency::Required
            && (!enableTestDependencies || it.key().type != PluginDependency::Test))
            continue;

        PluginSpec *dependencySpec = it.value();
        if (!dependencySpec->isEffectivelyEnabled()) {
            dependencySpec->setEnabledIndirectly(true);
            enabled << dependencySpec;
        }
    }
    return enabled;
}

//==========PluginSpecPrivate==================

namespace {
    const char PLUGIN_METADATA[] = "MetaData";
    const char PLUGIN_NAME[] = "Name";
    const char PLUGIN_VERSION[] = "Version";
    const char PLUGIN_COMPATVERSION[] = "CompatVersion";
    const char PLUGIN_REQUIRED[] = "Required";
    const char PLUGIN_EXPERIMENTAL[] = "Experimental";
    const char PLUGIN_DISABLED_BY_DEFAULT[] = "DisabledByDefault";
    const char PLUGIN_DEPRECATED[] = "Deprecated";
    const char PLUGIN_SOFTLOADABLE[] = "SoftLoadable";
    const char VENDOR[] = "Vendor";
    const char COPYRIGHT[] = "Copyright";
    const char LICENSE[] = "License";
    const char DESCRIPTION[] = "Description";
    const char LONGDESCRIPTION[] = "LongDescription";
    const char URL[] = "Url";
    const char DOCUMENTATIONURL[] = "DocumentationUrl";
    const char CATEGORY[] = "Category";
    const char PLATFORM[] = "Platform";
    const char DEPENDENCIES[] = "Dependencies";
    const char DEPENDENCY_NAME[] = "Name";
    const char DEPENDENCY_VERSION[] = "Version";
    const char DEPENDENCY_TYPE[] = "Type";
    const char DEPENDENCY_TYPE_SOFT[] = "optional";
    const char DEPENDENCY_TYPE_HARD[] = "required";
    const char DEPENDENCY_TYPE_TEST[] = "test";
    const char ARGUMENTS[] = "Arguments";
    const char ARGUMENT_NAME[] = "Name";
    const char ARGUMENT_PARAMETER[] = "Parameter";
    const char ARGUMENT_DESCRIPTION[] = "Description";
}

/*!
    \internal
    Returns false if the file does not represent a Qt Creator plugin.
*/
expected_str<PluginSpec *> readCppPluginSpec(const FilePath &fileName)
{
    auto spec = new CppPluginSpec;

    const FilePath absPath = fileName.absoluteFilePath();

    spec->setLocation(absPath.parentDir());
    spec->setFilePath(absPath);
    spec->d->loader.emplace();

    if (Utils::HostOsInfo::isMacHost())
        spec->d->loader->setLoadHints(QLibrary::ExportExternalSymbolsHint);

    spec->d->loader->setFileName(absPath.toFSPathString());
    if (spec->d->loader->fileName().isEmpty())
        return make_unexpected(::ExtensionSystem::Tr::tr("Cannot open file"));

    expected_str<void> r = spec->readMetaData(spec->d->loader->metaData());
    if (!r)
        return make_unexpected(r.error());

    return spec;
}

expected_str<PluginSpec *> readCppPluginSpec(const QStaticPlugin &plugin)
{
    auto spec = new CppPluginSpec;

    qCDebug(pluginLog) << "\nReading meta data of static plugin";
    spec->d->staticPlugin = plugin;
    expected_str<void> r = spec->readMetaData(plugin.metaData());
    if (!r)
        return make_unexpected(r.error());

    return spec;
}

static inline QString msgValueMissing(const char *key)
{
    return Tr::tr("\"%1\" is missing").arg(QLatin1String(key));
}

static inline QString msgValueIsNotAString(const char *key)
{
    return Tr::tr("Value for key \"%1\" is not a string").arg(QLatin1String(key));
}

static inline QString msgValueIsNotABool(const char *key)
{
    return Tr::tr("Value for key \"%1\" is not a bool").arg(QLatin1String(key));
}

static inline QString msgValueIsNotAObjectArray(const char *key)
{
    return Tr::tr("Value for key \"%1\" is not an array of objects").arg(QLatin1String(key));
}

static inline QString msgValueIsNotAMultilineString(const char *key)
{
    return Tr::tr("Value for key \"%1\" is not a string and not an array of strings")
        .arg(QLatin1String(key));
}

static inline QString msgInvalidFormat(const char *key, const QString &content)
{
    return Tr::tr("Value \"%2\" for key \"%1\" has invalid format").arg(QLatin1String(key), content);
}

Utils::expected_str<void> PluginSpec::readMetaData(const QJsonObject &metaData)
{
    return d->readMetaData(metaData);
}
Utils::expected_str<void> PluginSpec::reportError(const QString &error)
{
    return d->reportError(error);
}

/*!
    \internal
*/
expected_str<void> CppPluginSpec::readMetaData(const QJsonObject &pluginMetaData)
{
    qCDebug(pluginLog).noquote() << "MetaData:" << QJsonDocument(pluginMetaData).toJson();
    QJsonValue value;
    value = pluginMetaData.value(QLatin1String("IID"));
    if (!value.isString())
        return make_unexpected(::ExtensionSystem::Tr::tr("No IID found"));

    if (value.toString() != PluginManager::pluginIID())
        return make_unexpected(::ExtensionSystem::Tr::tr("Expected IID \"%1\", but found \"%2\"")
                                   .arg(PluginManager::pluginIID())
                                   .arg(value.toString()));

    value = pluginMetaData.value(QLatin1String(PLUGIN_METADATA));
    if (!value.isObject())
        return reportError(::ExtensionSystem::Tr::tr("Plugin meta data not found"));

    return PluginSpec::readMetaData(value.toObject());
}

Utils::expected_str<void> PluginSpecPrivate::readMetaData(const QJsonObject &data)
{
    metaData = data;

    QJsonValue value = metaData.value(QLatin1String(PLUGIN_NAME));
    if (value.isUndefined())
        return reportError(msgValueMissing(PLUGIN_NAME));
    if (!value.isString())
        return reportError(msgValueIsNotAString(PLUGIN_NAME));
    name = value.toString();

    value = metaData.value(QLatin1String(PLUGIN_VERSION));
    if (value.isUndefined())
        return reportError(msgValueMissing(PLUGIN_VERSION));
    if (!value.isString())
        return reportError(msgValueIsNotAString(PLUGIN_VERSION));
    version = value.toString();
    if (!PluginSpec::isValidVersion(version))
        return reportError(msgInvalidFormat(PLUGIN_VERSION, version));

    value = metaData.value(QLatin1String(PLUGIN_COMPATVERSION));
    if (!value.isUndefined() && !value.isString())
        return reportError(msgValueIsNotAString(PLUGIN_COMPATVERSION));
    compatVersion = value.toString(version);
    if (!value.isUndefined() && !PluginSpec::isValidVersion(compatVersion))
        return reportError(msgInvalidFormat(PLUGIN_COMPATVERSION, compatVersion));

    value = metaData.value(QLatin1String(PLUGIN_REQUIRED));
    if (!value.isUndefined() && !value.isBool())
        return reportError(msgValueIsNotABool(PLUGIN_REQUIRED));
    required = value.toBool(false);
    qCDebug(pluginLog) << "required =" << required;

    value = metaData.value(QLatin1String(PLUGIN_EXPERIMENTAL));
    if (!value.isUndefined() && !value.isBool())
        return reportError(msgValueIsNotABool(PLUGIN_EXPERIMENTAL));
    experimental = value.toBool(false);
    qCDebug(pluginLog) << "experimental =" << experimental;

    value = metaData.value(QLatin1String(PLUGIN_DEPRECATED));
    if (!value.isUndefined() && !value.isBool())
        return reportError(msgValueIsNotABool(PLUGIN_DEPRECATED));
    deprecated = value.toBool(false);
    qCDebug(pluginLog) << "deprecated =" << deprecated;

    value = metaData.value(QLatin1String(PLUGIN_DISABLED_BY_DEFAULT));
    if (!value.isUndefined() && !value.isBool())
        return reportError(msgValueIsNotABool(PLUGIN_DISABLED_BY_DEFAULT));
    enabledByDefault = !value.toBool(experimental || deprecated);
    qCDebug(pluginLog) << "enabledByDefault =" << enabledByDefault;

    enabledBySettings = enabledByDefault;

    value = metaData.value(QLatin1String(PLUGIN_SOFTLOADABLE));
    if (!value.isUndefined() && !value.isBool())
        return reportError(msgValueIsNotABool(PLUGIN_SOFTLOADABLE));
    softLoadable = value.toBool(false);
    qCDebug(pluginLog) << "softLoadable =" << softLoadable;

    value = metaData.value(QLatin1String(VENDOR));
    if (!value.isUndefined() && !value.isString())
        return reportError(msgValueIsNotAString(VENDOR));
    vendor = value.toString();

    value = metaData.value(QLatin1String(COPYRIGHT));
    if (!value.isUndefined() && !value.isString())
        return reportError(msgValueIsNotAString(COPYRIGHT));
    copyright = value.toString();

    value = metaData.value(QLatin1String(DESCRIPTION));
    if (!value.isUndefined() && !readMultiLineString(value, &description))
        return reportError(msgValueIsNotAString(DESCRIPTION));

    value = metaData.value(QLatin1String(LONGDESCRIPTION));
    if (!value.isUndefined() && !readMultiLineString(value, &longDescription))
        return reportError(msgValueIsNotAString(LONGDESCRIPTION));

    value = metaData.value(QLatin1String(URL));
    if (!value.isUndefined() && !value.isString())
        return reportError(msgValueIsNotAString(URL));
    url = value.toString();

    value = metaData.value(QLatin1String(DOCUMENTATIONURL));
    if (!value.isUndefined() && !value.isString())
        return reportError(msgValueIsNotAString(DOCUMENTATIONURL));
    documentationUrl = value.toString();

    value = metaData.value(QLatin1String(CATEGORY));
    if (!value.isUndefined() && !value.isString())
        return reportError(msgValueIsNotAString(CATEGORY));
    category = value.toString();

    value = metaData.value(QLatin1String(LICENSE));
    if (!value.isUndefined() && !readMultiLineString(value, &license))
        return reportError(msgValueIsNotAMultilineString(LICENSE));

    value = metaData.value("Revision");
    if (!value.isUndefined() && !value.isString())
        return reportError(msgValueIsNotAString("Revision"));
    revision = value.toString();

    value = metaData.value(QLatin1String(PLATFORM));
    if (!value.isUndefined() && !value.isString())
        return reportError(msgValueIsNotAString(PLATFORM));
    const QString platformSpec = value.toString().trimmed();
    if (!platformSpec.isEmpty()) {
        platformSpecification.setPattern(platformSpec);
        if (!platformSpecification.isValid()) {
            return reportError(::ExtensionSystem::Tr::tr("Invalid platform specification \"%1\": %2")
                                   .arg(platformSpec, platformSpecification.errorString()));
        }
    }

    value = metaData.value(QLatin1String(DEPENDENCIES));
    if (!value.isUndefined() && !value.isArray())
        return reportError(msgValueIsNotAObjectArray(DEPENDENCIES));
    if (!value.isUndefined()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &v : array) {
            if (!v.isObject())
                return reportError(msgValueIsNotAObjectArray(DEPENDENCIES));
            QJsonObject dependencyObject = v.toObject();
            PluginDependency dep;
            value = dependencyObject.value(QLatin1String(DEPENDENCY_NAME));
            if (value.isUndefined()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Dependency: %1")
                        .arg(msgValueMissing(DEPENDENCY_NAME)));
            }
            if (!value.isString()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Dependency: %1")
                        .arg(msgValueIsNotAString(DEPENDENCY_NAME)));
            }
            dep.name = value.toString();
            value = dependencyObject.value(QLatin1String(DEPENDENCY_VERSION));
            if (!value.isUndefined() && !value.isString()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Dependency: %1")
                        .arg(msgValueIsNotAString(DEPENDENCY_VERSION)));
            }
            dep.version = value.toString();
            if (!PluginSpec::isValidVersion(dep.version)) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Dependency: %1")
                        .arg(msgInvalidFormat(DEPENDENCY_VERSION, dep.version)));
            }
            dep.type = PluginDependency::Required;
            value = dependencyObject.value(QLatin1String(DEPENDENCY_TYPE));
            if (!value.isUndefined() && !value.isString()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Dependency: %1")
                        .arg(msgValueIsNotAString(DEPENDENCY_TYPE)));
            }
            if (!value.isUndefined()) {
                const QString typeValue = value.toString();
                if (typeValue.toLower() == QLatin1String(DEPENDENCY_TYPE_HARD)) {
                    dep.type = PluginDependency::Required;
                } else if (typeValue.toLower() == QLatin1String(DEPENDENCY_TYPE_SOFT)) {
                    dep.type = PluginDependency::Optional;
                } else if (typeValue.toLower() == QLatin1String(DEPENDENCY_TYPE_TEST)) {
                    dep.type = PluginDependency::Test;
                } else {
                    return reportError(
                        ::ExtensionSystem::Tr::tr(
                            "Dependency: \"%1\" must be \"%2\" or \"%3\" (is \"%4\").")
                            .arg(QLatin1String(DEPENDENCY_TYPE),
                                 QLatin1String(DEPENDENCY_TYPE_HARD),
                                 QLatin1String(DEPENDENCY_TYPE_SOFT),
                                 typeValue));
                }
            }
            dependencies.append(dep);
        }
    }

    value = metaData.value(QLatin1String(ARGUMENTS));
    if (!value.isUndefined() && !value.isArray())
        return reportError(msgValueIsNotAObjectArray(ARGUMENTS));
    if (!value.isUndefined()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &v : array) {
            if (!v.isObject())
                return reportError(msgValueIsNotAObjectArray(ARGUMENTS));
            QJsonObject argumentObject = v.toObject();
            PluginArgumentDescription arg;
            value = argumentObject.value(QLatin1String(ARGUMENT_NAME));
            if (value.isUndefined()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Argument: %1")
                        .arg(msgValueMissing(ARGUMENT_NAME)));
            }
            if (!value.isString()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Argument: %1")
                        .arg(msgValueIsNotAString(ARGUMENT_NAME)));
            }
            arg.name = value.toString();
            if (arg.name.isEmpty()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Argument: \"%1\" is empty")
                        .arg(QLatin1String(ARGUMENT_NAME)));
            }
            value = argumentObject.value(QLatin1String(ARGUMENT_DESCRIPTION));
            if (!value.isUndefined() && !value.isString()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Argument: %1")
                        .arg(msgValueIsNotAString(ARGUMENT_DESCRIPTION)));
            }
            arg.description = value.toString();
            value = argumentObject.value(QLatin1String(ARGUMENT_PARAMETER));
            if (!value.isUndefined() && !value.isString()) {
                return reportError(
                    ::ExtensionSystem::Tr::tr("Argument: %1")
                        .arg(msgValueIsNotAString(ARGUMENT_PARAMETER)));
            }
            arg.parameter = value.toString();
            argumentDescriptions.append(arg);
            qCDebug(pluginLog) << "Argument:" << arg.name << "Parameter:" << arg.parameter
                               << "Description:" << arg.description;
        }
    }

    state = PluginSpec::Read;

    return {};
}

/*!
    \internal
*/
static const QRegularExpression &versionRegExp()
{
    static const QRegularExpression reg("^([0-9]+)(?:[.]([0-9]+))?(?:[.]([0-9]+))?(?:_([0-9]+))?$");
    return reg;
}
/*!
    \internal
*/
bool PluginSpec::isValidVersion(const QString &version)
{
    return versionRegExp().match(version).hasMatch();
}

/*!
    \internal
*/
int PluginSpec::versionCompare(const QString &version1, const QString &version2)
{
    const QRegularExpressionMatch match1 = versionRegExp().match(version1);
    const QRegularExpressionMatch match2 = versionRegExp().match(version2);
    if (!match1.hasMatch())
        return 0;
    if (!match2.hasMatch())
        return 0;
    for (int i = 0; i < 4; ++i) {
        const int number1 = match1.captured(i + 1).toInt();
        const int number2 = match2.captured(i + 1).toInt();
        if (number1 < number2)
            return -1;
        if (number1 > number2)
            return 1;
    }
    return 0;
}

/*!
    \internal
*/
bool PluginSpec::resolveDependencies(const PluginSpecs &specs)
{
    if (hasError())
        return false;
    if (state() > PluginSpec::Resolved)
        return true; // We are resolved already.
    if (state() == PluginSpec::Resolved)
        setState(PluginSpec::Read); // Go back, so we just re-resolve the dependencies.
    if (state() != PluginSpec::Read) {
        setError(::ExtensionSystem::Tr::tr("Resolving dependencies failed because state != Read"));
        return false;
    }

    QHash<PluginDependency, PluginSpec *> resolvedDependencies;
    for (const PluginDependency &dependency : d->dependencies) {
        PluginSpec *const found = findOrDefault(specs, [this, &dependency](PluginSpec *spec) {
            return provides(spec, dependency);
        });
        if (!found) {
            if (dependency.type == PluginDependency::Required) {
                const QString error = ::ExtensionSystem::Tr::tr(
                                          "Could not resolve dependency '%1(%2)'")
                                          .arg(dependency.name, dependency.version);
                if (hasError())
                    setError(errorString() + '\n' + error);
                else
                    setError(error);
            }
            continue;
        }
        resolvedDependencies.insert(dependency, found);
    }
    if (hasError())
        return false;

    d->dependencySpecs = resolvedDependencies;

    d->state = PluginSpec::Resolved;

    return true;
}

PluginSpec::PluginSpec()
    : d(new PluginSpecPrivate())
{}

PluginSpec::~PluginSpec() = default;

void PluginSpec::setState(State state)
{
    d->state = state;
}

void PluginSpec::setLocation(const FilePath &location)
{
    d->location = location;
}

void PluginSpec::setFilePath(const FilePath &filePath)
{
    d->filePath = filePath;
}

void PluginSpec::setError(const QString &errorString)
{
    qCWarning(pluginSpecLog).noquote() << "[" << name() << "]"
                                       << "Plugin error:" << errorString;
    d->errorString = errorString;
}

/*!
    \internal
*/
bool CppPluginSpec::loadLibrary()
{
    if (hasError())
        return false;

    if (state() != PluginSpec::Resolved) {
        if (state() == PluginSpec::Loaded)
            return true;
        setError(::ExtensionSystem::Tr::tr("Loading the library failed because state != Resolved"));
        return false;
    }
    if (d->loader && !d->loader->load()) {
        setError(filePath().toUserOutput() + QString::fromLatin1(": ") + d->loader->errorString());
        return false;
    }
    auto *pluginObject = d->loader ? qobject_cast<IPlugin *>(d->loader->instance())
                                   : qobject_cast<IPlugin *>(d->staticPlugin->instance());
    if (!pluginObject) {
        setError(::ExtensionSystem::Tr::tr("Plugin is not valid (does not derive from IPlugin)"));
        if (d->loader)
            d->loader->unload();
        return false;
    }
    setState(PluginSpec::Loaded);
    d->plugin = pluginObject;
    return true;
}

/*!
    \internal
*/
bool CppPluginSpec::initializePlugin()
{
    if (hasError())
        return false;

    if (state() != PluginSpec::Loaded) {
        if (state() == PluginSpec::Initialized)
            return true;
        setError(
            ::ExtensionSystem::Tr::tr("Initializing the plugin failed because state != Loaded"));
        return false;
    }
    if (!d->plugin) {
        setError(
            ::ExtensionSystem::Tr::tr("Internal error: have no plugin instance to initialize"));
        return false;
    }
    QString err;
    if (!d->plugin->initialize(arguments(), &err)) {
        setError(::ExtensionSystem::Tr::tr("Plugin initialization failed: %1").arg(err));
        return false;
    }
    setState(PluginSpec::Initialized);
    return true;
}

/*!
    \internal
*/
bool CppPluginSpec::initializeExtensions()
{
    if (hasError())
        return false;

    if (state() != PluginSpec::Initialized) {
        if (state() == PluginSpec::Running)
            return true;
        setError(::ExtensionSystem::Tr::tr(
            "Cannot perform extensionsInitialized because state != Initialized"));
        return false;
    }
    if (!d->plugin) {
        setError(::ExtensionSystem::Tr::tr(
            "Internal error: have no plugin instance to perform extensionsInitialized"));
        return false;
    }
    d->plugin->extensionsInitialized();
    setState(PluginSpec::Running);
    return true;
}

/*!
    \internal
*/
bool CppPluginSpec::delayedInitialize()
{
    if (hasError())
        return true;

    if (state() != PluginSpec::Running)
        return false;
    if (!d->plugin) {
        setError(::ExtensionSystem::Tr::tr(
            "Internal error: have no plugin instance to perform delayedInitialize"));
        return false;
    }
    const bool res = d->plugin->delayedInitialize();
    return res;
}

/*!
    \internal
*/
IPlugin::ShutdownFlag CppPluginSpec::stop()
{
    if (hasError())
        return IPlugin::ShutdownFlag::SynchronousShutdown;

    if (!d->plugin)
        return IPlugin::SynchronousShutdown;
    setState(PluginSpec::Stopped);
    return d->plugin->aboutToShutdown();
}

/*!
    \internal
*/
void CppPluginSpec::kill()
{
    if (hasError())
        return;

    if (!d->plugin)
        return;
    delete d->plugin;
    d->plugin = nullptr;
    setState(PluginSpec::Deleted);
}
} // namespace ExtensionSystem
