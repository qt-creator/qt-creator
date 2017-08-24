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
        case CodeCompletion::FunctionOverloadCompletionKind: return "FunctionOverload";
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

    debug.nospace() << message.m_text << ", ";
    debug.nospace() << message.m_priority << ", ";
    debug.nospace() << completionKindToString(message.m_completionKind) << ", ";
    debug.nospace() << availabilityToString(message.m_availability) << ", ";
    debug.nospace() << message.m_hasParameters;

    debug.nospace() << ")";

    return debug;
}

std::ostream &operator<<(std::ostream &os, const CodeCompletion &message)
{
    os << "("
       << message.m_text << ", "
       << message.m_priority << ", "
       << message.m_completionKind << ", "
       << message.m_availability << ", "
       << message.m_hasParameters
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const CodeCompletion::Kind kind)
{
    return os << completionKindToString(kind);
}

std::ostream &operator<<(std::ostream &os, const CodeCompletion::Availability availability)
{
    return os << availabilityToString(availability);
}

} // namespace ClangBackEnd

