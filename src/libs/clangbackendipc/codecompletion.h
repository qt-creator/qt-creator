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

#include "clangbackendipc_global.h"
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
        : text_(text),
          priority_(priority),
          completionKind_(completionKind),
          availability_(availability),
          hasParameters_(hasParameters)
    {
    }

    void setText(const Utf8String &text)
    {
        text_ = text;
    }

    const Utf8String &text() const
    {
        return text_;
    }

    void setCompletionKind(Kind completionKind)
    {
        completionKind_ = completionKind;
    }

    Kind completionKind() const
    {
        return completionKind_;
    }

    void setChunks(const CodeCompletionChunks &chunks)
    {
        chunks_ = chunks;
    }

    const CodeCompletionChunks &chunks() const
    {
        return chunks_;
    }

    void setAvailability(Availability availability)
    {
        availability_ = availability;
    }

    Availability availability() const
    {
        return availability_;
    }

    void setHasParameters(bool hasParameters)
    {
        hasParameters_ = hasParameters;
    }

    bool hasParameters() const
    {
        return hasParameters_;
    }

    void setPriority(quint32 priority)
    {
        priority_ = priority;
    }

    quint32 priority() const
    {
        return priority_;
    }

    void setBriefComment(const Utf8String &briefComment)
    {
        briefComment_ = briefComment;
    }

    const Utf8String &briefComment() const
    {
        return briefComment_;
    }

    friend QDataStream &operator<<(QDataStream &out, const CodeCompletion &message)
    {
        out << message.text_;
        out << message.briefComment_;
        out << message.chunks_;
        out << message.priority_;
        out << static_cast<quint32>(message.completionKind_);
        out << static_cast<quint32>(message.availability_);
        out << message.hasParameters_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CodeCompletion &message)
    {
        quint32 completionKind;
        quint32 availability;

        in >> message.text_;
        in >> message.briefComment_;
        in >> message.chunks_;
        in >> message.priority_;
        in >> completionKind;
        in >> availability;
        in >> message.hasParameters_;

        message.completionKind_ = static_cast<CodeCompletion::Kind>(completionKind);
        message.availability_ = static_cast<CodeCompletion::Availability>(availability);

        return in;
    }

    friend bool operator==(const CodeCompletion &first, const CodeCompletion &second)
    {
        return first.text_ == second.text_
                && first.completionKind_ == second.completionKind_;
    }

    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletion &message);
    friend void PrintTo(const CodeCompletion &message, ::std::ostream* os);

private:
    Utf8String text_;
    Utf8String briefComment_;
    CodeCompletionChunks chunks_;
    quint32 priority_ = 0;
    Kind completionKind_ = Other;
    Availability availability_ = NotAvailable;
    bool hasParameters_ = false;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, CodeCompletion::Kind kind);

void PrintTo(CodeCompletion::Kind kind, ::std::ostream *os);
void PrintTo(CodeCompletion::Availability availability, ::std::ostream *os);
} // namespace ClangBackEnd
