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

#ifndef CLANGBACKEND_CODECOMPLETION_H
#define CLANGBACKEND_CODECOMPLETION_H

#include "clangbackendipc_global.h"
#include "codecompletionchunk.h"

#include <utf8string.h>

#include <QVector>

namespace ClangBackEnd {

class CodeCompletion;
using CodeCompletions = QVector<CodeCompletion>;

class CMBIPC_EXPORT CodeCompletion
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CodeCompletion &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CodeCompletion &message);
    friend CMBIPC_EXPORT bool operator==(const CodeCompletion &first, const CodeCompletion &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletion &message);
    friend void PrintTo(const CodeCompletion &message, ::std::ostream* os);

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
                   bool hasParameters = false);

    void setText(const Utf8String &text);
    const Utf8String &text() const;

    void setCompletionKind(Kind completionKind);
    Kind completionKind() const;

    void setChunks(const CodeCompletionChunks &chunks);
    const CodeCompletionChunks &chunks() const;

    void setAvailability(Availability availability);
    Availability availability() const;

    void setHasParameters(bool hasParameters);
    bool hasParameters() const;

    void setPriority(quint32 priority);
    quint32 priority() const;

    void setBriefComment(const Utf8String &briefComment);
    const Utf8String &briefComment() const;

private:
    quint32 &completionKindAsInt();
    quint32 &availabilityAsInt();

private:
    Utf8String text_;
    Utf8String briefComment_;
    CodeCompletionChunks chunks_;
    quint32 priority_ = 0;
    Kind completionKind_ = Other;
    Availability availability_ = NotAvailable;
    bool hasParameters_ = false;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CodeCompletion &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CodeCompletion &message);
CMBIPC_EXPORT bool operator==(const CodeCompletion &first, const CodeCompletion &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletion &message);
CMBIPC_EXPORT QDebug operator<<(QDebug debug, CodeCompletion::Kind kind);

void PrintTo(const CodeCompletion &message, ::std::ostream* os);
void PrintTo(CodeCompletion::Kind kind, ::std::ostream *os);
void PrintTo(CodeCompletion::Availability availability, ::std::ostream *os);
} // namespace ClangBackEnd

#endif // CLANGBACKEND_CODECOMPLETION_H
