/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

quint32 &CodeCompletion::completionKindAsInt()
{
    return reinterpret_cast<quint32&>(completionKind_);
}

quint32 &CodeCompletion::availabilityAsInt()
{
    return reinterpret_cast<quint32&>(availability_);
}

QDataStream &operator<<(QDataStream &out, const CodeCompletion &command)
{
    out << command.text_;
    out << command.chunks_;
    out << command.priority_;
    out << command.completionKind_;
    out << command.availability_;
    out << command.hasParameters_;

    return out;
}

QDataStream &operator>>(QDataStream &in, CodeCompletion &command)
{
    in >> command.text_;
    in >> command.chunks_;
    in >> command.priority_;
    in >> command.completionKindAsInt();
    in >> command.availabilityAsInt();
    in >> command.hasParameters_;

    return in;
}

bool operator==(const CodeCompletion &first, const CodeCompletion &second)
{
    return first.text_ == second.text_
            && first.completionKind_ == second.completionKind_;
}

bool operator<(const CodeCompletion &first, const CodeCompletion &second)
{
    return first.text_ < second.text_;
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

QDebug operator<<(QDebug debug, const CodeCompletion &command)
{
    debug.nospace() << "CodeCompletion(";

    debug.nospace() << command.text_ << ", ";
    debug.nospace() << command.priority_ << ", ";
    debug.nospace() << completionKindToString(command.completionKind_) << ", ";
    debug.nospace() << availabilityToString(command.availability_) << ", ";
    debug.nospace() << command.hasParameters_;

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const CodeCompletion &command, ::std::ostream* os)
{
    *os << "CodeCompletion(";

    *os << command.text_.constData() << ", ";
    *os << command.priority_ << ", ";
    *os << completionKindToString(command.completionKind_) << ", ";
    *os << availabilityToString(command.availability_) << ", ";
    *os << command.hasParameters_;

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

