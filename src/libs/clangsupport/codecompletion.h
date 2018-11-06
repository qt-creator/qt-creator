/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangsupport_global.h"
#include "codecompletionchunk.h"
#include "fixitcontainer.h"

#include <sqlite/utf8string.h>

#include <QDataStream>
#include <QVector>

namespace ClangBackEnd {

class CodeCompletion;
using CodeCompletions = QVector<CodeCompletion>;

class CodeCompletion
{
public:
    enum Kind : uint8_t {
        Other = 0,
        FunctionCompletionKind,
        FunctionDefinitionCompletionKind,
        FunctionOverloadCompletionKind,
        TemplateFunctionCompletionKind,
        ClassCompletionKind,
        ConstructorCompletionKind,
        DestructorCompletionKind,
        VariableCompletionKind,
        TypeAliasCompletionKind,
        TemplateClassCompletionKind,
        EnumerationCompletionKind,
        EnumeratorCompletionKind,
        NamespaceCompletionKind,
        PreProcessorCompletionKind,
        SignalCompletionKind,
        SlotCompletionKind,
        ObjCMessageCompletionKind,
        KeywordCompletionKind,
        ClangSnippetKind
    };

    enum Availability : quint32 {
        Available,
        Deprecated,
        NotAvailable,
        NotAccessible
    };

public:
    CodeCompletion() = default;
    CodeCompletion(const Utf8String &text,
                   quint32 priority = 0,
                   Kind completionKind = Other,
                   Availability availability = Available,
                   bool hasParameters = false)
        : text(text),
          priority(priority),
          completionKind(completionKind),
          availability(availability),
          hasParameters(hasParameters)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const CodeCompletion &message)
    {
        out << message.text;
        out << message.briefComment;
        out << message.chunks;
        out << message.requiredFixIts;
        out << message.priority;
        out << static_cast<quint32>(message.completionKind);
        out << static_cast<quint32>(message.availability);
        out << message.hasParameters;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CodeCompletion &message)
    {
        quint32 completionKind;
        quint32 availability;

        in >> message.text;
        in >> message.briefComment;
        in >> message.chunks;
        in >> message.requiredFixIts;
        in >> message.priority;
        in >> completionKind;
        in >> availability;
        in >> message.hasParameters;

        message.completionKind = static_cast<CodeCompletion::Kind>(completionKind);
        message.availability = static_cast<CodeCompletion::Availability>(availability);

        return in;
    }

    friend bool operator==(const CodeCompletion &first, const CodeCompletion &second)
    {
        return first.text == second.text
                && first.completionKind == second.completionKind
                && first.requiredFixIts == second.requiredFixIts;
    }

public:
    Utf8String text;
    Utf8String briefComment;
    CodeCompletionChunks chunks;
    QVector<FixItContainer> requiredFixIts;
    quint32 priority = 0;
    Kind completionKind = Other;
    Availability availability = NotAvailable;
    bool hasParameters = false;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const CodeCompletion &message);
CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, CodeCompletion::Kind kind);

CLANGSUPPORT_EXPORT std::ostream &operator<<(std::ostream &os, const CodeCompletion::Kind kind);
CLANGSUPPORT_EXPORT std::ostream &operator<<(std::ostream &os, const CodeCompletion::Availability availability);
} // namespace ClangBackEnd
