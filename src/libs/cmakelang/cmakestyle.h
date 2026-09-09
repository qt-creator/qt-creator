// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakelang.h"

#include <QString>

#include <functional>

namespace CMakeLang {

// Whether the argument names a keyword of the command, the way the signature
// of the command spells it.
using KeywordPredicate = std::function<bool(const QString &command, const QString &argument)>;

// How a CMake file is to be laid out.
class CMAKELANG_EXPORT Style
{
public:
    // Whether the values below a line that holds nothing but a keyword of the
    // command take a level of their own.
    bool indentKeywordValues = true;

    // Whether a space stands between the name of a command and the
    // parenthesis that opens its arguments. CMake spells neither with one,
    // but a file that puts a space there usually puts it there throughout.
    bool spaceBeforeControlParen = false;
    bool spaceBeforeCommandParen = false;

    // Whether the whitespace before a comment at the end of a line is left as
    // it is, so that comments a file lines up in a column stay lined up.
    bool keepCommentColumn = true;

    // The keywords a command takes. Nothing counts as a keyword while it is
    // not set.
    KeywordPredicate isKeyword;

    // The whitespace that puts a line at the level, tabs or spaces as the
    // editor of the user has it. Four spaces a level while it is not set.
    std::function<QString(int level)> indentation;

    QString indentationFor(int level) const;

    // A command may take an argument spelled the way one of its documented
    // values is, so only an argument that looks like a keyword counts as one.
    bool namesKeyword(const QString &command, QStringView argument) const;
};

} // namespace CMakeLang
