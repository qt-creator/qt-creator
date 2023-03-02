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

namespace Utils {

static QReadWriteLock s_envMutex;
Q_GLOBAL_STATIC_WITH_ARGS(Environment, staticSystemEnvironment,
                          (QProcessEnvironment::systemEnvironment().toStringList()))
Q_GLOBAL_STATIC(QVector<EnvironmentProvider>, environmentProviders)

NameValueItems Environment::diff(const Environment &other, bool checkAppendPrepend) const
{
    return m_dict.diff(other.m_dict, checkAppendPrepend);
}

Environment::FindResult Environment::find(const QString &name) const
{
    const auto it = m_dict.constFind(name);
    if (it == m_dict.constEnd())
        return {};
     return Entry{it.key().name, it.value().first, it.value().second};
}

void Environment::forEachEntry(const std::function<void(const QString &, const QString &, bool)> &callBack) const
{
    for (auto it = m_dict.m_values.constBegin(); it != m_dict.m_values.constEnd(); ++it)
        callBack(it.key().name, it.value().first, it.value().second);
}

bool Environment::hasChanges() const
{
    return m_dict.size() != 0;
}

QProcessEnvironment Environment::toProcessEnvironment() const
{
    QProcessEnvironment result;
    for (auto it = m_dict.m_values.constBegin(); it != m_dict.m_values.constEnd(); ++it) {
        if (it.value().second)
            result.insert(it.key().name, expandedValueForKey(m_dict.key(it)));
    }
    return result;
}

void Environment::appendOrSetPath(const FilePath &value)
{
    QTC_CHECK(value.osType() == osType());
    if (value.isEmpty())
        return;
    appendOrSet("PATH", value.nativePath(),
                QString(OsSpecificAspects::pathListSeparator(osType())));
}

void Environment::prependOrSetPath(const FilePath &value)
{
    QTC_CHECK(value.osType() == osType());
    if (value.isEmpty())
        return;
    prependOrSet("PATH", value.nativePath(),
                 QString(OsSpecificAspects::pathListSeparator(osType())));
}

void Environment::appendOrSet(const QString &key, const QString &value, const QString &sep)
{
    QTC_ASSERT(!key.contains('='), return );
    const auto it = m_dict.findKey(key);
    if (it == m_dict.m_values.end()) {
        m_dict.m_values.insert(DictKey(key, m_dict.nameCaseSensitivity()), {value, true});
    } else {
        // Append unless it is already there
        const QString toAppend = sep + value;
        if (!it.value().first.endsWith(toAppend))
            it.value().first.append(toAppend);
    }
}

void Environment::prependOrSet(const QString &key, const QString &value, const QString &sep)
{
    QTC_ASSERT(!key.contains('='), return );
    const auto it = m_dict.findKey(key);
    if (it == m_dict.m_values.end()) {
        m_dict.m_values.insert(DictKey(key, m_dict.nameCaseSensitivity()), {value, true});
    } else {
        // Prepend unless it is already there
        const QString toPrepend = value + sep;
        if (!it.value().first.startsWith(toPrepend))
            it.value().first.prepend(toPrepend);
    }
}

void Environment::prependOrSetLibrarySearchPath(const FilePath &value)
{
    QTC_CHECK(value.osType() == osType());
    switch (osType()) {
    case OsTypeWindows: {
        const QChar sep = ';';
        prependOrSet("PATH", value.nativePath(), QString(sep));
        break;
    }
    case OsTypeMac: {
        const QString sep =  ":";
        const QString nativeValue = value.nativePath();
        prependOrSet("DYLD_LIBRARY_PATH", nativeValue, sep);
        prependOrSet("DYLD_FRAMEWORK_PATH", nativeValue, sep);
        break;
    }
    case OsTypeLinux:
    case OsTypeOtherUnix: {
        const QChar sep = ':';
        prependOrSet("LD_LIBRARY_PATH", value.nativePath(), QString(sep));
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

Environment Environment::systemEnvironment()
{
    QReadLocker lock(&s_envMutex);
    return *staticSystemEnvironment();
}

void Environment::setupEnglishOutput()
{
    m_dict.set("LC_MESSAGES", "en_US.utf8");
    m_dict.set("LANGUAGE", "en_US:en");
}

using SearchResultCallback = std::function<IterationPolicy(const FilePath &)>;

static IterationPolicy searchInDirectory(const SearchResultCallback &resultCallback,
                                         const FilePaths &execs,
                                         const FilePath &directory,
                                         QSet<FilePath> &alreadyCheckedDirectories,
                                         const FilePathPredicate &filter = {})
{
    // Compare the initial size of the set with the size after insertion to check if the directory
    // was already checked.
    const int initialCount = alreadyCheckedDirectories.count();
    alreadyCheckedDirectories.insert(directory);
    const bool wasAlreadyChecked = alreadyCheckedDirectories.count() == initialCount;

    if (directory.isEmpty() || wasAlreadyChecked)
        return IterationPolicy::Continue;

    for (const FilePath &exec : execs) {
        const FilePath filePath = directory / exec.path();
        if (filePath.isExecutableFile() && (!filter || filter(filePath))) {
            if (resultCallback(filePath) == IterationPolicy::Stop)
                return IterationPolicy::Stop;
        }
    }
    return IterationPolicy::Continue;
}

static FilePaths appendExeExtensions(const Environment &env, const FilePath &executable)
{
    FilePaths execs{executable};
    if (env.osType() == OsTypeWindows) {
        // Check all the executable extensions on windows:
        // PATHEXT is only used if the executable has no extension
        if (executable.suffix().isEmpty()) {
            const QStringList extensions = env.expandedValueForKey("PATHEXT").split(';');

            for (const QString &ext : extensions)
                execs << executable.stringAppended(ext.toLower());
        }
    }
    return execs;
}

QString Environment::expandedValueForKey(const QString &key) const
{
    return expandVariables(m_dict.value(key));
}

static void searchInDirectoriesHelper(const SearchResultCallback &resultCallback,
                                      const Environment &env,
                                      const QString &executable,
                                      const FilePaths &dirs,
                                      const FilePathPredicate &func,
                                      bool usePath)
{
    if (executable.isEmpty())
        return;

    const FilePath exec = FilePath::fromUserInput(QDir::cleanPath(env.expandVariables(executable)));
    const FilePaths execs = appendExeExtensions(env, exec);

    if (exec.isAbsolutePath()) {
        for (const FilePath &path : execs) {
            if (path.isExecutableFile() && (!func || func(path)))
                if (resultCallback(path) == IterationPolicy::Stop)
                    return;
        }
        return;
    }

    QSet<FilePath> alreadyCheckedDirectories;
    for (const FilePath &dir : dirs) {
        if (searchInDirectory(resultCallback, execs, dir, alreadyCheckedDirectories, func)
            == IterationPolicy::Stop)
            return;
    }

    if (usePath) {
        QTC_ASSERT(!executable.contains('/'), return);

        for (const FilePath &p : env.path()) {
            if (searchInDirectory(resultCallback, execs, p, alreadyCheckedDirectories, func)
                == IterationPolicy::Stop)
                return;
        }
    }
    return;
}

FilePath Environment::searchInDirectories(const QString &executable,
                                          const FilePaths &dirs,
                                          const FilePathPredicate &func) const
{
    FilePath result;
    searchInDirectoriesHelper(
        [&result](const FilePath &path) {
            result = path;
            return IterationPolicy::Stop;
        },
        *this,
        executable,
        dirs,
        func,
        false);

    return result;
}

FilePath Environment::searchInPath(const QString &executable,
                                   const FilePaths &additionalDirs,
                                   const FilePathPredicate &func) const
{
    FilePath result;
    searchInDirectoriesHelper(
        [&result](const FilePath &path) {
            result = path;
            return IterationPolicy::Stop;
        },
        *this,
        executable,
        additionalDirs,
        func,
        true);

    return result;
}

FilePaths Environment::findAllInPath(const QString &executable,
                                     const FilePaths &additionalDirs,
                                     const FilePathPredicate &func) const
{
    QSet<FilePath> result;
    searchInDirectoriesHelper(
        [&result](const FilePath &path) {
            result.insert(path);
            return IterationPolicy::Continue;
        },
        *this,
        executable,
        additionalDirs,
        func,
        true);

    return result.values();
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
    QString result = input;

    if (osType() == OsTypeWindows) {
        for (int vStart = -1, i = 0; i < result.length(); ) {
            if (result.at(i++) == '%') {
                if (vStart > 0) {
                    const auto it = m_dict.findKey(result.mid(vStart, i - vStart - 1));
                    if (it != m_dict.m_values.constEnd()) {
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

void EnvironmentProvider::addProvider(EnvironmentProvider &&provider)
{
    environmentProviders->append(std::move(provider));
}

const QVector<EnvironmentProvider> EnvironmentProvider::providers()
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

void EnvironmentChange::addSetValue(const QString &key, const QString &value)
{
    m_changeItems.append(Item{std::in_place_index_t<SetValue>(), QPair<QString, QString>{key, value}});
}

void EnvironmentChange::addUnsetValue(const QString &key)
{
    m_changeItems.append(Item{std::in_place_index_t<UnsetValue>(), key});
}

void EnvironmentChange::addPrependToPath(const FilePaths &values)
{
    for (int i = values.size(); --i >= 0; ) {
        const FilePath value = values.at(i);
        m_changeItems.append(Item{std::in_place_index_t<PrependToPath>(), value});
    }
}

void EnvironmentChange::addAppendToPath(const FilePaths &values)
{
    for (const FilePath &value : values)
        m_changeItems.append(Item{std::in_place_index_t<AppendToPath>(), value});
}

EnvironmentChange EnvironmentChange::fromDictionary(const NameValueDictionary &dict)
{
    EnvironmentChange change;
    change.m_changeItems.append(Item{std::in_place_index_t<SetFixedDictionary>(), dict});
    return change;
}

void EnvironmentChange::applyToEnvironment(Environment &env) const
{
    for (const Item &item : m_changeItems) {
        switch (item.index()) {
        case SetSystemEnvironment:
            env = Environment::systemEnvironment();
            break;
        case SetFixedDictionary:
            env = Environment(std::get<SetFixedDictionary>(item));
            break;
        case SetValue: {
            const QPair<QString, QString> data = std::get<SetValue>(item);
            env.set(data.first, data.second);
            break;
        }
        case UnsetValue:
            env.unset(std::get<UnsetValue>(item));
            break;
        case PrependToPath:
            env.prependOrSetPath(std::get<PrependToPath>(item));
            break;
        case AppendToPath:
            env.appendOrSetPath(std::get<AppendToPath>(item));
            break;
        }
    }
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
