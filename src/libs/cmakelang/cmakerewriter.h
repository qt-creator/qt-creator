// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakedocument.h"
#include "cmakelang.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace CMakeLang {

// What to put in place of the span the position and the length name.
class Edit
{
public:
    int position = 0;
    int length = 0;
    QString text;
};

// Changes to a parsed file, spelled out in terms of its AST. What the changes
// amount to is a list of edits to the text: nothing outside their spans moves,
// so the comments of the file and the way the author laid it out stay.
class CMAKELANG_EXPORT Rewriter
{
public:
    explicit Rewriter(const DocumentPtr &document);

    // Puts a value in place of the one the argument names. It keeps the
    // quoting the argument has, and gets quotes of its own where it needs
    // them.
    void replaceValue(ArgumentAST *argument, const QString &value);

    // Takes the arguments away, the first and the last one included, together
    // with the whitespace that separated them from what comes before. The
    // lines they stood on go as well when nothing else is left on them.
    void remove(ArgumentAST *argument);
    void remove(ArgumentAST *first, ArgumentAST *last);

    // Puts values after the argument, one line each, indented the way the
    // line the argument stands on is.
    void insertAfter(ArgumentAST *argument, const QStringList &values);

    // Puts values after the last argument of the call.
    void append(CommandAST *command, const QStringList &values);

    bool isEmpty() const { return _edits.isEmpty(); }

    // In the order the file spells the text they change.
    QList<Edit> edits() const;

private:
    void addEdit(int position, int length, const QString &text);
    QString indentationAt(int position) const;
    QString lines(const QStringList &values, int position) const;

    const DocumentPtr _document;
    const QStringView _source;
    QList<Edit> _edits;
};

} // namespace CMakeLang
