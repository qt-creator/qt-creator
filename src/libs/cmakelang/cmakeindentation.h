// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakelang.h"

#include <QList>
#include <QString>

#include <functional>

namespace CMakeLang {

// Whether the argument names a keyword of the command, the way the signature
// of the command spells it.
using KeywordPredicate = std::function<bool(const QString &command, const QString &argument)>;

// How deep every line of a CMake file sits, counted in indentation levels.
//
// A command indents its arguments by one level and puts its closing
// parenthesis back where its name stands. A line that holds nothing but a
// keyword of the command opens a list: what follows takes one level more,
// until the next keyword ends it. A keyword that shares the line with the
// command name opens no list, so the arguments below it stay at one level.
// A group of parentheses within the arguments takes one level more again. The
// body of if(), foreach(), while(), function(), macro() and block() takes one
// level more than the command that opens it.
//
// It reads the token stream rather than the syntax tree, so a file that is
// halfway typed, with a parenthesis still open or an if() that nothing closes
// yet, gets an answer as well.
class CMAKELANG_EXPORT Indentation
{
public:
    // A line that continues a quoted or a bracket argument. Its leading
    // whitespace is part of the value and has to stay the way it is.
    static constexpr int Keep = -1;

    // The source is read while the object is built and is not kept.
    explicit Indentation(QStringView source, const KeywordPredicate &isKeyword = {});

    int lineCount() const { return int(_levels.size()); }

    // Lines count from one, the way the tokens count them.
    int levelAt(int line) const;

private:
    QList<int> _levels;
};

} // namespace CMakeLang
