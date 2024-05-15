// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>

#include <QString>
#include <QStringList>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT UniqueName
{
public:
    static QString get(const QString &name, std::function<bool(const QString &)> predicate);
    static QString getPath(const QString &path);
    static QString getId(const QString &id, std::function<bool(const QString &)> predicate);

private:
    static inline const QStringList keywords {
        "anchors", "as", "baseState", "border", "bottom", "break", "case", "catch", "clip", "color",
        "continue", "data", "debugger", "default", "delete", "do", "else", "enabled", "finally",
        "flow", "focus", "font", "for", "function", "height", "if", "import", "in", "instanceof",
        "item", "layer", "left", "margin", "new", "opacity", "padding", "parent", "print", "rect",
        "return", "right", "scale", "shaderInfo", "source", "sprite", "spriteSequence", "state",
        "switch", "text", "this", "throw", "top", "try", "typeof", "var", "visible", "void",
        "while", "with", "x", "y"
    };
};

} // namespace QmlDesigner
