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
    Environment();
    explicit Environment(OsType osType);
    explicit Environment(const QStringList &env, OsType osType = HostOsInfo::hostOs());
    explicit Environment(const NameValuePairs &nameValues);
    explicit Environment(const NameValueDictionary &dict);

    QString value(const QString &key) const;
    QString value_or(const QString &key, const QString &defaultValue) const;
    bool hasKey(const QString &key) const;

    void set(const QString &key, const QString &value, bool enabled = true);
    void setFallback(const QString &key, const QString &value);
    void unset(const QString &key);
    void modify(const NameValueItems &items);

    bool hasChanges() const;

    OsType osType() const;
    QStringList toStringList() const;
    QProcessEnvironment toProcessEnvironment() const;

    void appendOrSet(const QString &key, const QString &value, const QString &sep = QString());
    void prependOrSet(const QString &key, const QString &value, const QString &sep = QString());

    void appendOrSetPath(const FilePath &value);
    void prependOrSetPath(const FilePath &value);
    void prependOrSetPath(const QString &directories); // Could be several ':'/';' separated entries.

    void prependOrSetLibrarySearchPath(const FilePath &value);
    void prependOrSetLibrarySearchPaths(const FilePaths &values);

    void prependToPath(const FilePaths &values);
    void appendToPath(const FilePaths &values);

    void setupEnglishOutput();
    void setupSudoAskPass(const FilePath &askPass);

    FilePath searchInPath(const QString &executable,
                          const FilePaths &additionalDirs = FilePaths(),
                          const FilePathPredicate &func = {},
                          FilePath::MatchScope = FilePath::WithAnySuffix) const;

    FilePaths path() const;
    FilePaths pathListValue(const QString &varName) const;

    QString expandedValueForKey(const QString &key) const;
    QString expandVariables(const QString &input) const;
    FilePath expandVariables(const FilePath &input) const;
    QStringList expandVariables(const QStringList &input) const;

    NameValueDictionary toDictionary() const; // FIXME: avoid
    NameValueItems diff(const Environment &other, bool checkAppendPrepend = false) const; // FIXME: avoid

    struct Entry { QString key; QString value; bool enabled; };
    using FindResult = std::optional<Entry>;
    FindResult find(const QString &name) const; // Note res->key may differ in case from name.

    void forEachEntry(const std::function<void (const QString &, const QString &, bool)> &callBack) const;

    bool operator!=(const Environment &other) const;
    bool operator==(const Environment &other) const;

    static Environment systemEnvironment();

    static void modifySystemEnvironment(const EnvironmentItems &list); // use with care!!!
    static void setSystemEnvironment(const Environment &environment);  // don't use at all!!!

    enum Type {
        SetSystemEnvironment,
        SetFixedDictionary,
        SetValue,
        SetFallbackValue,
        UnsetValue,
        PrependOrSet,
        AppendOrSet,
        Modify,
        SetupEnglishOutput
    };

    using Item = std::variant<
        std::monostate,                          // SetSystemEnvironment dummy
        NameValueDictionary,                     // SetFixedDictionary
        std::tuple<QString, QString, bool>,      // SetValue (key, value, enabled)
        std::tuple<QString, QString>,            // SetFallbackValue (key, value)
        QString,                                 // UnsetValue (key)
        std::tuple<QString, QString, QString>,   // PrependOrSet (key, value, separator)
        std::tuple<QString, QString, QString>,   // AppendOrSet (key, value, separator)
        NameValueItems,                          // Modify
        std::monostate,                          // SetupEnglishOutput
        FilePath                                 // SetupSudoAskPass (file path of qtc-askpass or ssh-askpass)
    >;

    void addItem(const Item &item);

    Environment appliedToEnvironment(const Environment &base) const;

    const NameValueDictionary &resolved() const;

private:
    mutable QList<Item> m_changeItems;
    mutable NameValueDictionary m_dict; // Latest resolved.
    mutable bool m_fullDict = false;
};

using EnviromentChange = Environment;

class QTCREATOR_UTILS_EXPORT EnvironmentProvider
{
public:
    QByteArray id;
    QString displayName;
    std::function<Environment()> environment;

    static void addProvider(EnvironmentProvider &&provider);
    static const QList<EnvironmentProvider> providers();
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
