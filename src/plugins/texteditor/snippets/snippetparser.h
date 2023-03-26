// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include <utils/id.h>

#include <variant>

namespace TextEditor {

class TEXTEDITOR_EXPORT NameMangler
{
public:
    virtual ~NameMangler();

    virtual Utils::Id id() const = 0;
    virtual QString mangle(const QString &unmangled) const = 0;
};

class TEXTEDITOR_EXPORT ParsedSnippet
{
public:
    class Part {
    public:
        Part() = default;
        explicit Part(const QString &text) : text(text) {}
        QString text;
        int variableIndex = -1; // if variable index is >= 0 the text is interpreted as a variable
        NameMangler *mangler = nullptr;
        bool finalPart = false;
    };
    QList<Part> parts;
    QList<QList<int>> variables;
};

class TEXTEDITOR_EXPORT SnippetParseError
{
public:
    QString errorMessage;
    QString text;
    int pos;

    QString htmlMessage() const;
};

using SnippetParseResult = std::variant<ParsedSnippet, SnippetParseError>;
using SnippetParser = std::function<SnippetParseResult (const QString &)>;

} // namespace TextEditor
