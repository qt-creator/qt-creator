// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <QByteArray>
#include <QHash>
#include <QVector>

namespace ProjectExplorer {

enum class MacroType
{
    Invalid,
    Define,
    Undefine
};

class Macro;

using Macros = QVector<Macro>;

class PROJECTEXPLORER_EXPORT Macro
{
public:
    Macro() = default;

    Macro(QByteArray key, QByteArray value, MacroType type = MacroType::Define)
        : key(key), value(value), type(type)
    {}

    Macro(QByteArray key, MacroType type = MacroType::Define)
        : key(key), type(type)
    {}

    bool isValid() const;

    QByteArray toByteArray() const;
    static QByteArray toByteArray(const Macros &macros);

    static Macros toMacros(const QByteArray &text);

    // define Foo will be converted to Foo=1
    static Macro fromKeyValue(const QString &utf16text);
    static Macro fromKeyValue(const QByteArray &text);
    QByteArray toKeyValue(const QByteArray &prefix) const;

    friend auto qHash(const Macro &macro)
    {
        using QT_PREPEND_NAMESPACE(qHash);
        return qHash(macro.key) ^ qHash(macro.value) ^ qHash(int(macro.type));
    }

    friend bool operator==(const Macro &first, const Macro &second)
    {
        return first.type == second.type
                && first.key == second.key
                && first.value == second.value;
    }

public:
    QByteArray key;
    QByteArray value;
    MacroType type = MacroType::Invalid;

private:
    static QList<QByteArray> splitLines(const QByteArray &text);
    static QByteArray removeNonsemanticSpaces(QByteArray line);
    static QList<QByteArray> tokenizeLine(const QByteArray &line);
    static QList<QList<QByteArray>> tokenizeLines(const QList<QByteArray> &lines);
    static Macro tokensToMacro(const QList<QByteArray> &tokens);
    static Macros tokensLinesToMacros(const QList<QList<QByteArray>> &tokensLines);
};

} // namespace ProjectExplorer
