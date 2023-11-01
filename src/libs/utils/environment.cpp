// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environment.h"

#include "algorithm.h"
#include "filepath.h"
#include "qtcassert.h"

#include <QDir>
#include <QProcessEnvironment>
#include <QReadWriteLock>
#include <QSet>

/*!
    \class Utils::Environment
    \inmodule QtCreator

    \brief The Environment class sets \QC's system environment.
*/

namespace Utils {

static QReadWriteLock s_envMutex;
Q_GLOBAL_STATIC_WITH_ARGS(Environment, staticSystemEnvironment,
                          (QProcessEnvironment::systemEnvironment().toStringList()))
Q_GLOBAL_STATIC(QList<EnvironmentProvider>, environmentProviders)

Environment::Environment()
    : m_dict(HostOsInfo::hostOs())
{}

Environment::Environment(OsType osType)
    : m_dict(osType)
{}

Environment::Environment(const QStringList &env, OsType osType)
    : m_dict(osType)
{
    m_changeItems.append(NameValueDictionary(env, osType));
}

Environment::Environment(const NameValuePairs &nameValues)
{
    m_changeItems.append(NameValueDictionary(nameValues));
}

Environment::Environment(const NameValueDictionary &dict)
{
    m_changeItems.append(dict);
}

NameValueItems Environment::diff(const Environment &other, bool checkAppendPrepend) const
{
    const NameValueDictionary &dict = resolved();
    const NameValueDictionary &otherDict = other.resolved();
    return dict.diff(otherDict, checkAppendPrepend);
}

Environment::FindResult Environment::find(const QString &name) const
{
    const NameValueDictionary &dict = resolved();
    const auto it = dict.constFind(name);
    if (it == dict.constEnd())
        return {};
     return Entry{it.key().name, it.value().first, it.value().second};
}

void Environment::forEachEntry(const std::function<void(const QString &, const QString &, bool)> &callBack) const
{
    const NameValueDictionary &dict = resolved();
    for (auto it = dict.m_values.constBegin(); it != dict.m_values.constEnd(); ++it)
        callBack(it.key().name, it.value().first, it.value().second);
}

bool Environment::operator==(const Environment &other) const
{
    const NameValueDictionary &dict = resolved();
    const NameValueDictionary &otherDict = other.resolved();
    return dict == otherDict;
}

bool Environment::operator!=(const Environment &other) const
{
    const NameValueDictionary &dict = resolved();
    const NameValueDictionary &otherDict = other.resolved();
    return dict != otherDict;
}

QString Environment::value(const QString &key) const
{
    const NameValueDictionary &dict = resolved();
    return dict.value(key);
}

QString Environment::value_or(const QString &key, const QString &defaultValue) const
{
    const NameValueDictionary &dict = resolved();
    return dict.hasKey(key) ? dict.value(key) : defaultValue;
}

bool Environment::hasKey(const QString &key) const
{
    const NameValueDictionary &dict = resolved();
    return dict.hasKey(key);
}

bool Environment::hasChanges() const
{
    const NameValueDictionary &dict = resolved();
    return dict.size() != 0;
}

OsType Environment::osType() const
{
    return m_dict.m_osType;
}

QStringList Environment::toStringList() const
{
    const NameValueDictionary &dict = resolved();
    return dict.toStringList();
}

QProcessEnvironment Environment::toProcessEnvironment() const
{
    const NameValueDictionary &dict = resolved();
    QProcessEnvironment result;
    for (auto it = dict.m_values.constBegin(); it != dict.m_values.constEnd(); ++it) {
        if (it.value().second)
            result.insert(it.key().name, expandedValueForKey(dict.key(it)));
    }
    return result;
}

void Environment::appendOrSetPath(const FilePath &value)
{
    QTC_CHECK(value.osType() == m_dict.m_osType);
    if (value.isEmpty())
        return;
    appendOrSet("PATH", value.nativePath(), OsSpecificAspects::pathListSeparator(osType()));
}

void Environment::prependOrSetPath(const FilePath &value)
{
    QTC_CHECK(value.osType() == m_dict.m_osType);
    if (value.isEmpty())
        return;
    prependOrSet("PATH", value.nativePath(), OsSpecificAspects::pathListSeparator(osType()));
}

void Environment::prependOrSetPath(const QString &directories)
{
    prependOrSet("PATH", directories, OsSpecificAspects::pathListSeparator(osType()));
}

void Environment::appendOrSet(const QString &key, const QString &value, const QString &sep)
{
    addItem(Item{std::in_place_index_t<AppendOrSet>(), key, value, sep});
}

void Environment::prependOrSet(const QString &key, const QString &value, const QString &sep)
{
    addItem(Item{std::in_place_index_t<PrependOrSet>(), key, value, sep});
}

void Environment::prependOrSetLibrarySearchPath(const FilePath &value)
{
    QTC_CHECK(value.osType() == osType());
    const QChar sep = OsSpecificAspects::pathListSeparator(osType());
    switch (osType()) {
    case OsTypeWindows: {
        prependOrSet("PATH", value.nativePath(), sep);
        break;
    }
    case OsTypeMac: {
        const QString nativeValue = value.nativePath();
        prependOrSet("DYLD_LIBRARY_PATH", nativeValue, sep);
        prependOrSet("DYLD_FRAMEWORK_PATH", nativeValue, sep);
        break;
    }
    case OsTypeLinux:
    case OsTypeOtherUnix: {
        prependOrSet("LD_LIBRARY_PATH", value.nativePath(), sep);
        break;
    }
    default:
        break;
    }
}

void Environment::prependOrSetLibrarySearchPaths(const FilePaths &values)
{
    reverseForeach(values, [this](const FilePath &value) {
        prependOrSetLibrarySearchPath(value);
    });
}

/*!
    Returns \QC's system environment.

    This can be different from the system environment that \QC started in if the
    user changed it in \uicontrol Preferences > \uicontrol Environment >
    \uicontrol System > \uicontrol Environment.
*/

Environment Environment::systemEnvironment()
{
    QReadLocker lock(&s_envMutex);
    return *staticSystemEnvironment();
}

void Environment::setupEnglishOutput()
{
    addItem(Item{std::in_place_index_t<SetupEnglishOutput>()});
}

using SearchResultCallback = std::function<IterationPolicy(const FilePath &)>;

QString Environment::expandedValueForKey(const QString &key) const
{
    const NameValueDictionary &dict = resolved();
    return expandVariables(dict.value(key));
}

FilePath Environment::searchInPath(const QString &executable,
                                   const FilePaths &additionalDirs,
                                   const FilePathPredicate &filter,
                                   FilePath::MatchScope scope) const
{
    const FilePath exec = FilePath::fromUserInput(expandVariables(executable));
    const FilePaths dirs = path() + additionalDirs;
    return exec.searchInDirectories(dirs, filter, scope);
}

FilePaths Environment::path() const
{
    return pathListValue("PATH");
}

FilePaths Environment::pathListValue(const QString &varName) const
{
    const QStringList pathComponents = expandedValueForKey(varName).split(
        OsSpecificAspects::pathListSeparator(osType()), Qt::SkipEmptyParts);
    return transform(pathComponents, &FilePath::fromUserInput);
}

void Environment::modifySystemEnvironment(const EnvironmentItems &list)
{
    QWriteLocker lock(&s_envMutex);
    staticSystemEnvironment->modify(list);
}

void Environment::setSystemEnvironment(const Environment &environment)
{
    QWriteLocker lock(&s_envMutex);
    *staticSystemEnvironment = environment;
}

/** Expand environment variables in a string.
 *
 * Environment variables are accepted in the following forms:
 * $SOMEVAR, ${SOMEVAR} on Unix and %SOMEVAR% on Windows.
 * No escapes and quoting are supported.
 * If a variable is not found, it is not substituted.
 */
QString Environment::expandVariables(const QString &input) const
{
    const NameValueDictionary &dict = resolved();

    QString result = input;

    if (osType() == OsTypeWindows) {
        for (int vStart = -1, i = 0; i < result.length(); ) {
            if (result.at(i++) == '%') {
                if (vStart > 0) {
                    const auto it = dict.findKey(result.mid(vStart, i - vStart - 1));
                    if (it != dict.m_values.constEnd()) {
                        result.replace(vStart - 1, i - vStart + 1, it->first);
                        i = vStart - 1 + it->first.length();
                        vStart = -1;
                    } else {
                        vStart = i;
                    }
                } else {
                    vStart = i;
                }
            }
        }
    } else {
        enum { BASE, OPTIONALVARIABLEBRACE, VARIABLE, BRACEDVARIABLE } state = BASE;
        int vStart = -1;

        for (int i = 0; i < result.length();) {
            QChar c = result.at(i++);
            if (state == BASE) {
                if (c == '$')
                    state = OPTIONALVARIABLEBRACE;
            } else if (state == OPTIONALVARIABLEBRACE) {
                if (c == '{') {
                    state = BRACEDVARIABLE;
                    vStart = i;
                } else if (c.isLetterOrNumber() || c == '_') {
                    state = VARIABLE;
                    vStart = i - 1;
                } else {
                    state = BASE;
                }
            } else if (state == BRACEDVARIABLE) {
                if (c == '}') {
                    const QString key = result.mid(vStart, i - 1 - vStart);
                    const Environment::FindResult res = find(key);
                    if (res) {
                        result.replace(vStart - 2, i - vStart + 2, res->value);
                        i = vStart - 2 + res->value.length();
                    }
                    state = BASE;
                }
            } else if (state == VARIABLE) {
                if (!c.isLetterOrNumber() && c != '_') {
                    const QString key = result.mid(vStart, i - vStart - 1);
                    const Environment::FindResult res = find(key);
                    if (res) {
                        result.replace(vStart - 1, i - vStart, res->value);
                        i = vStart - 1 + res->value.length();
                    }
                    state = BASE;
                }
            }
        }
        if (state == VARIABLE) {
            const Environment::FindResult res = find(result.mid(vStart));
            if (res)
                result.replace(vStart - 1, result.length() - vStart + 1, res->value);
        }
    }
    return result;
}

FilePath Environment::expandVariables(const FilePath &variables) const
{
    return FilePath::fromString(expandVariables(variables.toString()));
}

QStringList Environment::expandVariables(const QStringList &variables) const
{
    return transform(variables, [this](const QString &i) { return expandVariables(i); });
}

NameValueDictionary Environment::toDictionary() const
{
    const NameValueDictionary &dict = resolved();
    return dict;
}

void EnvironmentProvider::addProvider(EnvironmentProvider &&provider)
{
    environmentProviders->append(std::move(provider));
}

const QList<EnvironmentProvider> EnvironmentProvider::providers()
{
    return *environmentProviders;
}

std::optional<EnvironmentProvider> EnvironmentProvider::provider(const QByteArray &id)
{
    const int index = indexOf(*environmentProviders, equal(&EnvironmentProvider::id, id));
    if (index >= 0)
        return std::make_optional(environmentProviders->at(index));
    return std::nullopt;
}

void Environment::addItem(const Item &item)
{
    m_dict.clear();
    m_changeItems.append(item);
}

void Environment::set(const QString &key, const QString &value, bool enabled)
{
    addItem(Item{std::in_place_index_t<SetValue>(),
                 std::tuple<QString, QString, bool>{key, value, enabled}});
}

void Environment::setFallback(const QString &key, const QString &value)
{
    addItem(Item{std::in_place_index_t<SetFallbackValue>(),
                 std::tuple<QString, QString>{key, value}});
}

void Environment::unset(const QString &key)
{
    addItem(Item{std::in_place_index_t<UnsetValue>(), key});
}

void Environment::modify(const NameValueItems &items)
{
    addItem(Item{std::in_place_index_t<Modify>(), items});
}

void Environment::prependToPath(const FilePaths &values)
{
    m_dict.clear();
    for (int i = values.size(); --i >= 0; ) {
        const FilePath value = values.at(i);
        m_changeItems.append(Item{
            std::in_place_index_t<PrependOrSet>(),
            QString("PATH"),
            value.nativePath(),
            value.pathListSeparator()
        });
    }
}

void Environment::appendToPath(const FilePaths &values)
{
    m_dict.clear();
    for (const FilePath &value : values) {
        m_changeItems.append(Item{
            std::in_place_index_t<AppendOrSet>(),
            QString("PATH"),
            value.nativePath(),
            value.pathListSeparator()
        });
    }
}

const NameValueDictionary &Environment::resolved() const
{
    if (m_dict.size() != 0)
        return m_dict;

    m_fullDict = false;
    for (const Item &item : m_changeItems) {
        switch (item.index()) {
        case SetSystemEnvironment:
            m_dict = Environment::systemEnvironment().toDictionary();
            m_fullDict = true;
            break;
        case SetFixedDictionary:
            m_dict = std::get<SetFixedDictionary>(item);
            m_fullDict = true;
            break;
        case SetValue: {
            auto [key, value, enabled] = std::get<SetValue>(item);
            m_dict.set(key, value, enabled);
            break;
        }
        case SetFallbackValue: {
            auto [key, value] = std::get<SetFallbackValue>(item);
            if (m_fullDict) {
                if (m_dict.value(key).isEmpty())
                    m_dict.set(key, value, true);
            } else {
                QTC_ASSERT(false, qDebug() << "operating on partial dictionary");
                m_dict.set(key, value, true);
            }
            break;
        }
        case UnsetValue:
            m_dict.unset(std::get<UnsetValue>(item));
            break;
        case PrependOrSet: {
            auto [key, value, sep] = std::get<PrependOrSet>(item);
            QTC_ASSERT(!key.contains('='), return m_dict);
            const auto it = m_dict.findKey(key);
            if (it == m_dict.m_values.end()) {
                m_dict.m_values.insert(DictKey(key, m_dict.nameCaseSensitivity()), {value, true});
            } else {
                // Prepend unless it is already there
                const QString toPrepend = value + sep;
                if (!it.value().first.startsWith(toPrepend))
                    it.value().first.prepend(toPrepend);
            }
            break;
        }
        case AppendOrSet: {
            auto [key, value, sep] = std::get<AppendOrSet>(item);
            QTC_ASSERT(!key.contains('='), return m_dict);
            const auto it = m_dict.findKey(key);
            if (it == m_dict.m_values.end()) {
                m_dict.m_values.insert(DictKey(key, m_dict.nameCaseSensitivity()), {value, true});
            } else {
                // Prepend unless it is already there
                const QString toAppend = sep + value;
                if (!it.value().first.endsWith(toAppend))
                    it.value().first.append(toAppend);
            }
            break;
        }
        case Modify: {
            NameValueItems items = std::get<Modify>(item);
            m_dict.modify(items);
            break;
        }
        case SetupEnglishOutput:
            m_dict.set("LC_MESSAGES", "en_US.utf8");
            m_dict.set("LANGUAGE", "en_US:en");
            break;
        }
    }

    return m_dict;
}

Environment Environment::appliedToEnvironment(const Environment &base) const
{
    Environment res = base;
    res.m_dict.clear();
    res.m_changeItems.append(m_changeItems);
    return res;
}

/*!
    Returns the value of \a key in \QC's modified system environment.
    \sa Utils::Environment::systemEnvironment
    \sa qEnvironmentVariable
*/
QString qtcEnvironmentVariable(const QString &key)
{
    return Environment::systemEnvironment().value(key);
}

/*!
    Returns the value of \a key in \QC's modified system environment if it is set,
    or otherwise \a defaultValue.
    \sa Utils::Environment::systemEnvironment
    \sa qEnvironmentVariable
*/
QString qtcEnvironmentVariable(const QString &key, const QString &defaultValue)
{
    if (Environment::systemEnvironment().hasKey(key))
        return Environment::systemEnvironment().value(key);
    return defaultValue;
}

/*!
    Returns if the environment variable \a key is set \QC's modified system environment.
    \sa Utils::Environment::systemEnvironment
    \sa qEnvironmentVariableIsSet
*/
bool qtcEnvironmentVariableIsSet(const QString &key)
{
    return Environment::systemEnvironment().hasKey(key);
}

/*!
    Returns if the environment variable \a key is not set or empty in \QC's modified system
    environment.
    \sa Utils::Environment::systemEnvironment
    \sa qEnvironmentVariableIsEmpty
*/
bool qtcEnvironmentVariableIsEmpty(const QString &key)
{
    return Environment::systemEnvironment().value(key).isEmpty();
}

/*!
    Returns the value of \a key in \QC's modified system environment, converted to an int.
    If \a ok is not null, sets \c{*ok} to true or false depending on the success of the conversion
    \sa Utils::Environment::systemEnvironment
    \sa qEnvironmentVariableIntValue
*/
int qtcEnvironmentVariableIntValue(const QString &key, bool *ok)
{
    return Environment::systemEnvironment().value(key).toInt(ok);
}

} // namespace Utils
