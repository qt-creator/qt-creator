// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "environmentfwd.h"
#include "filepath.h"
#include "namevaluedictionary.h"
#include "utiltypes.h"

#include <QDateTime>

#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE
class QProcessEnvironment;
QT_END_NAMESPACE

namespace Utils {
class MacroExpander;

class QTCREATOR_UTILS_EXPORT Environment final
{
public:
    enum class PathSeparator { Auto, Colon, Semicolon };

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
    void modify(const EnvironmentItems &items);

    bool hasChanges() const;

    OsType osType() const;
    QStringList toStringList() const;
    QProcessEnvironment toProcessEnvironment() const;

    void appendOrSet(const QString &key,
                     const QString &value,
                     PathSeparator sep = PathSeparator::Auto);
    void prependOrSet(const QString &key,
                      const QString &value,
                      PathSeparator sep = PathSeparator::Auto);

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
    FilePaths mappedPath(const FilePath &anchor) const;
    FilePaths pathListValue(const QString &varName) const;
    void setPathListValue(const QString &varName, const FilePaths &paths);
    static QString valueFromPathList(const FilePaths &paths, OsType osType);
    static FilePaths pathListFromValue(const QString &value, OsType osType);

    QString expandedValueForKey(const QString &key) const;
    QString expandVariables(const QString &input) const;
    FilePath expandVariables(const FilePath &input) const;
    QStringList expandVariables(const QStringList &input) const;

    NameValueDictionary toDictionary() const; // FIXME: avoid
    EnvironmentItems diff(const Environment &other, bool checkAppendPrepend = false) const; // FIXME: avoid

    struct Entry { QString key; QString value; bool enabled; };
    using FindResult = std::optional<Entry>;
    FindResult find(const QString &name) const; // Note res->key may differ in case from name.

    void forEachEntry(const std::function<void (const QString &, const QString &, bool)> &callBack) const;

    bool operator!=(const Environment &other) const;
    bool operator==(const Environment &other) const;

    static Environment systemEnvironment();
    static const Environment &originalSystemEnvironment();

    static void modifySystemEnvironment(const EnvironmentItems &list); // use with care!!!
    static void setSystemEnvironment(const Environment &environment);  // don't use at all!!!

    using ListSeparatorProvider = std::function<std::optional<QString>(QString)>;
    static ListSeparatorProvider listSeparatorProvider();
    static void setListSeparatorProvider(const ListSeparatorProvider &provider);

    QChar pathListSeparator(PathSeparator sep) const;
    QString listSeparator(const QString &varName, PathSeparator sep) const;

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
        std::monostate,                              // SetSystemEnvironment dummy
        NameValueDictionary,                         // SetFixedDictionary
        std::tuple<QString, QString, bool>,          // SetValue (key, value, enabled)
        std::tuple<QString, QString>,                // SetFallbackValue (key, value)
        QString,                                     // UnsetValue (key)
        std::tuple<QString, QString, PathSeparator>, // PrependOrSet (key, value, separator)
        std::tuple<QString, QString, PathSeparator>, // AppendOrSet (key, value, separator)
        EnvironmentItems,                              // Modify
        std::monostate,                              // SetupEnglishOutput
        FilePath                                     // SetupSudoAskPass (file path of qtc-askpass or ssh-askpass)
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

class QTCREATOR_UTILS_EXPORT EnvironmentChanges
{
public:
    EnvironmentChanges() = default;
    EnvironmentChanges(const EnvironmentItems &items, const FilePath &file);

    void setItemsFromUser(const EnvironmentItems &items) { m_itemsFromUser = items; }

    EnvironmentItems itemsFromUser() const { return m_itemsFromUser; }
    EnvironmentItems itemsFromFile() const;

    void removeUserItemAt(int pos) { m_itemsFromUser.removeAt(pos); }
    void removeUserItem(const EnvironmentItem &item) { m_itemsFromUser.removeAll(item); }
    void setUserItemAt(int pos, const EnvironmentItem &item) { m_itemsFromUser[pos] = item; }
    void setUserItemValueAt(int pos, const QString &value) { m_itemsFromUser[pos].value = value; }
    void setUserItemOpAt(int pos, EnvironmentItem::Operation op);
    void appendUserItem(const EnvironmentItem &item) { m_itemsFromUser << item; }
    void transformUserItems(const std::function<void(EnvironmentItem &)> &modifier);

    void setFile(const FilePath &file);
    FilePath file() const { return m_file; }

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);
    static EnvironmentChanges createFromVariant(const QVariant &v);

    void modifyEnvironment(Environment &baseEnv, MacroExpander *expander) const;

    QString toShortSummary(MacroExpander *expander, bool multiLine) const;

    bool hasItems() const { return !m_itemsFromUser.isEmpty() || !itemsFromFile().isEmpty(); }
    bool hasData() const { return !m_itemsFromUser.isEmpty() || !file().isEmpty(); }
    void clear();

    friend bool operator==(const EnvironmentChanges &e1, const EnvironmentChanges &e2)
    {
        return e1.m_itemsFromUser == e2.m_itemsFromUser && e1.m_file == e2.m_file;
    }
    friend bool operator!=(const EnvironmentChanges &e1, const EnvironmentChanges &e2)
    {
        return !(e1 == e2);
    }

private:
    EnvironmentItems m_itemsFromUser;
    FilePath m_file;
    mutable std::optional<std::pair<QDateTime, EnvironmentItems>> m_itemsFromFile;
};

QTCREATOR_UTILS_EXPORT QString qtcEnvironmentVariable(const QString &key);
QTCREATOR_UTILS_EXPORT QString qtcEnvironmentVariable(const QString &key,
                                                      const QString &defaultValue);
QTCREATOR_UTILS_EXPORT bool qtcEnvironmentVariableIsSet(const QString &key);
QTCREATOR_UTILS_EXPORT bool qtcEnvironmentVariableIsEmpty(const QString &key);
QTCREATOR_UTILS_EXPORT int qtcEnvironmentVariableIntValue(const QString &key, bool *ok = nullptr);

QTCREATOR_UTILS_EXPORT Result<Environment> getUnixEnvironment(
    const FilePath &envCommand, OsType osType, const FilePath &scriptToSource);

} // namespace Utils

Q_DECLARE_METATYPE(Utils::Environment)
