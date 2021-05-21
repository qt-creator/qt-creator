/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <texteditor/texteditor_global.h>

#include <utils/id.h>
#include <utils/variant.h>

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

using SnippetParseResult = Utils::variant<ParsedSnippet, SnippetParseError>;
using SnippetParser = std::function<SnippetParseResult (const QString &)>;

} // namespace TextEditor
