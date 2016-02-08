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

#include "codecompletion.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

CodeCompletion::CodeCompletion(const Utf8String &text,
                               quint32 priority,
                               Kind completionKind,
                               Availability availability,
                               bool hasParameters)
    : text_(text),
      priority_(priority),
      completionKind_(completionKind),
      availability_(availability),
      hasParameters_(hasParameters)
{
}

void CodeCompletion::setText(const Utf8String &text)
{
    text_ = text;
}

const Utf8String &CodeCompletion::text() const
{
    return text_;
}

void CodeCompletion::setCompletionKind(CodeCompletion::Kind completionKind)
{
    completionKind_ = completionKind;
}

CodeCompletion::Kind CodeCompletion::completionKind() const
{
    return completionKind_;
}

void CodeCompletion::setChunks(const CodeCompletionChunks &chunks)
{
    chunks_ = chunks;
}

const CodeCompletionChunks &CodeCompletion::chunks() const
{
    return chunks_;
}

void CodeCompletion::setAvailability(CodeCompletion::Availability availability)
{
    availability_ = availability;
}

CodeCompletion::Availability CodeCompletion::availability() const
{
    return availability_;
}

void CodeCompletion::setHasParameters(bool hasParameters)
{
    hasParameters_ = hasParameters;
}

bool CodeCompletion::hasParameters() const
{
    return hasParameters_;
}

void CodeCompletion::setPriority(quint32 priority)
{
    priority_ = priority;
}

quint32 CodeCompletion::priority() const
{
    return priority_;
}

void CodeCompletion::setBriefComment(const Utf8String &briefComment)
{
    briefComment_ = briefComment;
}

const Utf8String &CodeCompletion::briefComment() const
{
    return briefComment_;
}

quint32 &CodeCompletion::completionKindAsInt()
{
    return reinterpret_cast<quint32&>(completionKind_);
}

quint32 &CodeCompletion::availabilityAsInt()
{
    return reinterpret_cast<quint32&>(availability_);
}

QDataStream &operator<<(QDataStream &out, const CodeCompletion &message)
{
    out << message.text_;
    out << message.briefComment_;
    out << message.chunks_;
    out << message.priority_;
    out << message.completionKind_;
    out << message.availability_;
    out << message.hasParameters_;

    return out;
}

QDataStream &operator>>(QDataStream &in, CodeCompletion &message)
{
    in >> message.text_;
    in >> message.briefComment_;
    in >> message.chunks_;
    in >> message.priority_;
    in >> message.completionKindAsInt();
    in >> message.availabilityAsInt();
    in >> message.hasParameters_;

    return in;
}

bool operator==(const CodeCompletion &first, const CodeCompletion &second)
{
    return first.text_ == second.text_
            && first.completionKind_ == second.completionKind_;
}

static const char *completionKindToString(CodeCompletion::Kind kind)
{
    switch (kind) {
        case CodeCompletion::Other: return "Other";
        case CodeCompletion::FunctionCompletionKind: return "Function";
        case CodeCompletion::TemplateFunctionCompletionKind: return "TemplateFunction";
        case CodeCompletion::ConstructorCompletionKind: return "Constructor";
        case CodeCompletion::DestructorCompletionKind: return "Destructor";
        case CodeCompletion::VariableCompletionKind: return "Variable";
        case CodeCompletion::ClassCompletionKind: return "Class";
        case CodeCompletion::TypeAliasCompletionKind: return "TypeAlias";
        case CodeCompletion::TemplateClassCompletionKind: return "TemplateClass";
        case CodeCompletion::EnumerationCompletionKind: return "Enumeration";
        case CodeCompletion::EnumeratorCompletionKind: return "Enumerator";
        case CodeCompletion::NamespaceCompletionKind: return "Namespace";
        case CodeCompletion::PreProcessorCompletionKind: return "PreProcessor";
        case CodeCompletion::SignalCompletionKind: return "Signal";
        case CodeCompletion::SlotCompletionKind: return "Slot";
        case CodeCompletion::ObjCMessageCompletionKind: return "ObjCMessage";
        case CodeCompletion::KeywordCompletionKind: return "Keyword";
        case CodeCompletion::ClangSnippetKind: return "ClangSnippet";
    }

    return nullptr;
}

static const char *availabilityToString(CodeCompletion::Availability availability)
{
    switch (availability) {
        case CodeCompletion::Available: return "Available";
        case CodeCompletion::Deprecated: return "Deprecated";
        case CodeCompletion::NotAvailable: return "NotAvailable";
        case CodeCompletion::NotAccessible: return "NotAccessible";
    }
    return nullptr;
}

QDebug operator<<(QDebug debug, const CodeCompletion &message)
{
    debug.nospace() << "CodeCompletion(";

    debug.nospace() << message.text_ << ", ";
    debug.nospace() << message.priority_ << ", ";
    debug.nospace() << completionKindToString(message.completionKind_) << ", ";
    debug.nospace() << availabilityToString(message.availability_) << ", ";
    debug.nospace() << message.hasParameters_;

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const CodeCompletion &message, ::std::ostream* os)
{
    *os << "CodeCompletion(";

    *os << message.text_.constData() << ", ";
    *os << message.priority_ << ", ";
    *os << completionKindToString(message.completionKind_) << ", ";
    *os << availabilityToString(message.availability_) << ", ";
    *os << message.hasParameters_;

    *os << ")";
}

void PrintTo(CodeCompletion::Kind kind, ::std::ostream *os)
{
    *os << completionKindToString(kind);
}

void PrintTo(CodeCompletion::Availability availability, std::ostream *os)
{
    *os << availabilityToString(availability);
}

} // namespace ClangBackEnd

