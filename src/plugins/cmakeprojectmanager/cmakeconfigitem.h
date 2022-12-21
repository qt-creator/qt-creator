// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <QByteArray>
#include <QStringList>

#include <optional>

namespace Utils {
class FilePath;
class MacroExpander;
} // namespace Utils

namespace ProjectExplorer {
class Kit;
}

namespace CMakeProjectManager {

class CMAKE_EXPORT CMakeConfigItem
{
public:
    enum Type { FILEPATH, PATH, BOOL, STRING, INTERNAL, STATIC, UNINITIALIZED };
    CMakeConfigItem();
    CMakeConfigItem(const QByteArray &k, Type t, const QByteArray &d, const QByteArray &v, const QStringList &s = {});
    CMakeConfigItem(const QByteArray &k, Type t, const QByteArray &v);
    CMakeConfigItem(const QByteArray &k, const QByteArray &v);

    static QStringList cmakeSplitValue(const QString &in, bool keepEmpty = false);
    static Type typeStringToType(const QByteArray &typeString);
    static QString typeToTypeString(const Type t);
    static std::optional<bool> toBool(const QString &value);
    bool isNull() const { return key.isEmpty(); }

    QString expandedValue(const ProjectExplorer::Kit *k) const;
    QString expandedValue(const Utils::MacroExpander *expander) const;

    static bool less(const CMakeConfigItem &a, const CMakeConfigItem &b);
    static CMakeConfigItem fromString(const QString &s);
    QString toString(const Utils::MacroExpander *expander = nullptr) const;
    QString toArgument() const;
    QString toArgument(const Utils::MacroExpander *expander) const;
    QString toCMakeSetLine(const Utils::MacroExpander *expander = nullptr) const;

    bool operator==(const CMakeConfigItem &o) const;
    friend size_t qHash(const CMakeConfigItem &it);  // needed for MSVC

    QByteArray key;
    Type type = STRING;
    bool isAdvanced = false;
    bool inCMakeCache = false;
    bool isUnset = false;
    bool isInitial = false;
    QByteArray value; // converted to string as needed
    QByteArray documentation;
    QStringList values;
};

class CMAKE_EXPORT CMakeConfig : public QList<CMakeConfigItem>
{
public:
    CMakeConfig() = default;
    CMakeConfig(const QList<CMakeConfigItem> &items) : QList<CMakeConfigItem>(items) {}
    CMakeConfig(std::initializer_list<CMakeConfigItem> items) : QList<CMakeConfigItem>(items) {}

    const QList<CMakeConfigItem> &toList() const { return *this; }

    static CMakeConfig fromArguments(const QStringList &list, QStringList &unknownOptions);
    static CMakeConfig fromFile(const Utils::FilePath &input, QString *errorMessage);

    QByteArray valueOf(const QByteArray &key) const;
    QString stringValueOf(const QByteArray &key) const;
    Utils::FilePath filePathValueOf(const QByteArray &key) const;
    QString expandedValueOf(const ProjectExplorer::Kit *k, const QByteArray &key) const;
};

} // namespace CMakeProjectManager
