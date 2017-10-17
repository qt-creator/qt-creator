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

#include <utf8string.h>

#include <QDataStream>
#include <QVector>

namespace ClangBackEnd {

class CodeCompletion;
using CodeCompletions = QVector<CodeCompletion>;

class CodeCompletion
{
public:
    enum Kind : quint32 {
        Other = 0,
        FunctionCompletionKind,
        FunctionOverloadCompletionKind,
        TemplateFunctionCompletionKind,
        ConstructorCompletionKind,
        DestructorCompletionKind,
        VariableCompletionKind,
        ClassCompletionKind,
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
        : m_text(text),
          m_priority(priority),
          m_completionKind(completionKind),
          m_availability(availability),
          m_hasParameters(hasParameters)
    {
    }

    void setText(const Utf8String &text)
    {
        m_text = text;
    }

    const Utf8String &text() const
    {
        return m_text;
    }

    void setCompletionKind(Kind completionKind)
    {
        m_completionKind = completionKind;
    }

    Kind completionKind() const
    {
        return m_completionKind;
    }

    void setChunks(const CodeCompletionChunks &chunks)
    {
        m_chunks = chunks;
    }

    const CodeCompletionChunks &chunks() const
    {
        return m_chunks;
    }

    void setAvailability(Availability availability)
    {
        m_availability = availability;
    }

    Availability availability() const
    {
        return m_availability;
    }

    void setHasParameters(bool hasParameters)
    {
        m_hasParameters = hasParameters;
    }

    bool hasParameters() const
    {
        return m_hasParameters;
    }

    void setPriority(quint32 priority)
    {
        m_priority = priority;
    }

    quint32 priority() const
    {
        return m_priority;
    }

    void setBriefComment(const Utf8String &briefComment)
    {
        m_briefComment = briefComment;
    }

    const Utf8String &briefComment() const
    {
        return m_briefComment;
    }

    friend QDataStream &operator<<(QDataStream &out, const CodeCompletion &message)
    {
        out << message.m_text;
        out << message.m_briefComment;
        out << message.m_chunks;
        out << message.m_priority;
        out << static_cast<quint32>(message.m_completionKind);
        out << static_cast<quint32>(message.m_availability);
        out << message.m_hasParameters;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CodeCompletion &message)
    {
        quint32 completionKind;
        quint32 availability;

        in >> message.m_text;
        in >> message.m_briefComment;
        in >> message.m_chunks;
        in >> message.m_priority;
        in >> completionKind;
        in >> availability;
        in >> message.m_hasParameters;

        message.m_completionKind = static_cast<CodeCompletion::Kind>(completionKind);
        message.m_availability = static_cast<CodeCompletion::Availability>(availability);

        return in;
    }

    friend bool operator==(const CodeCompletion &first, const CodeCompletion &second)
    {
        return first.m_text == second.m_text
                && first.m_completionKind == second.m_completionKind;
    }

    friend CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const CodeCompletion &message);
    friend std::ostream &operator<<(std::ostream &os, const CodeCompletion &message);

private:
    Utf8String m_text;
    Utf8String m_briefComment;
    CodeCompletionChunks m_chunks;
    quint32 m_priority = 0;
    Kind m_completionKind = Other;
    Availability m_availability = NotAvailable;
    bool m_hasParameters = false;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, CodeCompletion::Kind kind);

CLANGSUPPORT_EXPORT std::ostream &operator<<(std::ostream &os, const CodeCompletion::Kind kind);
CLANGSUPPORT_EXPORT std::ostream &operator<<(std::ostream &os, const CodeCompletion::Availability availability);
} // namespace ClangBackEnd
