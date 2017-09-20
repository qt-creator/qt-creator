/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    static QByteArray toByteArray(const QVector<Macros> &macross);

    static Macros toMacros(const QByteArray &text);

    // define Foo will be converted to Foo=1
    static Macro fromKeyValue(const QString &utf16text);
    static Macro fromKeyValue(const QByteArray &text);
    QByteArray toKeyValue(const QByteArray &prefix) const;

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

inline
uint qHash(const Macro &macro)
{
    using QT_PREPEND_NAMESPACE(qHash);
    return qHash(macro.key) ^ qHash(macro.value) ^ qHash(int(macro.type));
}

inline
bool operator==(const Macro &first, const Macro &second)
{
    return first.type == second.type
        && first.key == second.key
        && first.value == second.value;
}

} // namespace ProjectExplorer
