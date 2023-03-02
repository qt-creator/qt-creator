// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"
#include "filepath.h"
#include "namevaluedictionary.h"
#include "utiltypes.h"

#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE
class QProcessEnvironment;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT Environment final
{
public:
    Environment() : m_dict(HostOsInfo::hostOs()) {}
    explicit Environment(OsType osType) : m_dict(osType) {}
    explicit Environment(const QStringList &env, OsType osType = HostOsInfo::hostOs())
        : m_dict(env, osType) {}
    explicit Environment(const NameValuePairs &nameValues) : m_dict(nameValues) {}
    explicit Environment(const NameValueDictionary &dict) : m_dict(dict) {}

    QString value(const QString &key) const { return m_dict.value(key); }
    QString value_or(const QString &key, const QString &defaultValue) const
    {
        return m_dict.hasKey(key) ? m_dict.value(key) : defaultValue;
    }
    bool hasKey(const QString &key) const { return m_dict.hasKey(key); }

    void set(const QString &key, const QString &value, bool enabled = true) { m_dict.set(key, value, enabled); }
    void unset(const QString &key) { m_dict.unset(key); }
    void modify(const NameValueItems &items) { m_dict.modify(items); }

    bool hasChanges() const;
    void clear() { return m_dict.clear(); }

    QStringList toStringList() const { return m_dict.toStringList(); }
    QProcessEnvironment toProcessEnvironment() const;

    void appendOrSet(const QString &key, const QString &value, const QString &sep = QString());
    void prependOrSet(const QString &key, const QString &value, const QString &sep = QString());

    void appendOrSetPath(const FilePath &value);
    void prependOrSetPath(const FilePath &value);

    void prependOrSetLibrarySearchPath(const FilePath &value);
    void prependOrSetLibrarySearchPaths(const FilePaths &values);

    void setupEnglishOutput();

    FilePath searchInPath(const QString &executable,
                          const FilePaths &additionalDirs = FilePaths(),
                          const FilePathPredicate &func = {}) const;
    FilePath searchInDirectories(const QString &executable,
                                 const FilePaths &dirs,
                                 const FilePathPredicate &func = {}) const;
    FilePaths findAllInPath(const QString &executable,
                            const FilePaths &additionalDirs = {},
                            const FilePathPredicate &func = {}) const;

    FilePaths path() const;
    FilePaths pathListValue(const QString &varName) const;

    QString expandedValueForKey(const QString &key) const;
    QString expandVariables(const QString &input) const;
    FilePath expandVariables(const FilePath &input) const;
    QStringList expandVariables(const QStringList &input) const;

    OsType osType() const { return m_dict.osType(); }
    QString userName() const;

    using const_iterator = NameValueMap::const_iterator; // FIXME: avoid
    NameValueDictionary toDictionary() const { return m_dict; } // FIXME: avoid
    NameValueItems diff(const Environment &other, bool checkAppendPrepend = false) const; // FIXME: avoid

    QString key(const_iterator it) const { return m_dict.key(it); } // FIXME: avoid
    QString value(const_iterator it) const { return m_dict.value(it); } // FIXME: avoid
    bool isEnabled(const_iterator it) const { return m_dict.isEnabled(it); } // FIXME: avoid

    void setCombineWithDeviceEnvironment(bool combine) { m_combineWithDeviceEnvironment = combine; }
    bool combineWithDeviceEnvironment() const { return m_combineWithDeviceEnvironment; }

    const_iterator constBegin() const { return m_dict.constBegin(); } // FIXME: avoid
    const_iterator constEnd() const { return m_dict.constEnd(); } // FIXME: avoid

    struct Entry { QString key; QString value; bool enabled; };
    using FindResult = std::optional<Entry>;
    FindResult find(const QString &name) const; // Note res->key may differ in case from name.

    void forEachEntry(const std::function<void (const QString &, const QString &, bool)> &callBack) const;

    friend bool operator!=(const Environment &first, const Environment &second)
    {
        return first.m_dict != second.m_dict;
    }

    friend bool operator==(const Environment &first, const Environment &second)
    {
        return first.m_dict == second.m_dict;
    }

    static Environment systemEnvironment();

    static void modifySystemEnvironment(const EnvironmentItems &list); // use with care!!!
    static void setSystemEnvironment(const Environment &environment);  // don't use at all!!!

private:
    NameValueDictionary m_dict;
    bool m_combineWithDeviceEnvironment = true;
};

class QTCREATOR_UTILS_EXPORT EnvironmentChange final
{
public:
    EnvironmentChange() = default;

    enum Type {
        SetSystemEnvironment,
        SetFixedDictionary,
        SetValue,
        UnsetValue,
        PrependToPath,
        AppendToPath,
    };

    using Item = std::variant<
        std::monostate,          // SetSystemEnvironment dummy
        NameValueDictionary,     // SetFixedDictionary
        QPair<QString, QString>, // SetValue
        QString,                 // UnsetValue
        FilePath,                // PrependToPath
        FilePath                 // AppendToPath
    >;

    static EnvironmentChange fromDictionary(const NameValueDictionary &dict);

    void applyToEnvironment(Environment &) const;

    void addSetValue(const QString &key, const QString &value);
    void addUnsetValue(const QString &key);
    void addPrependToPath(const FilePaths &values);
    void addAppendToPath(const FilePaths &values);

private:
    QList<Item> m_changeItems;
};

class QTCREATOR_UTILS_EXPORT EnvironmentProvider
{
public:
    QByteArray id;
    QString displayName;
    std::function<Environment()> environment;

    static void addProvider(EnvironmentProvider &&provider);
    static const QVector<EnvironmentProvider> providers();
    static std::optional<EnvironmentProvider> provider(const QByteArray &id);
};

QTCREATOR_UTILS_EXPORT QString qtcEnvironmentVariable(const QString &key);
QTCREATOR_UTILS_EXPORT QString qtcEnvironmentVariable(const QString &key,
                                                      const QString &defaultValue);
QTCREATOR_UTILS_EXPORT bool qtcEnvironmentVariableIsSet(const QString &key);
QTCREATOR_UTILS_EXPORT bool qtcEnvironmentVariableIsEmpty(const QString &key);
QTCREATOR_UTILS_EXPORT int qtcEnvironmentVariableIntValue(const QString &key, bool *ok = nullptr);

} // namespace Utils

Q_DECLARE_METATYPE(Utils::Environment)
