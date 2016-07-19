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

#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

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

