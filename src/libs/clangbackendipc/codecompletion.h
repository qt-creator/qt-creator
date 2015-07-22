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

#ifndef CLANGBACKEND_CODECOMPLETION_H
#define CLANGBACKEND_CODECOMPLETION_H

#include "clangbackendipc_global.h"
#include "codecompletionchunk.h"

#include <utf8string.h>

#include <QMetaType>
#include <QVector>

namespace ClangBackEnd {

class CodeCompletion;
using CodeCompletions = QVector<CodeCompletion>;

class CMBIPC_EXPORT CodeCompletion
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CodeCompletion &command);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CodeCompletion &command);
    friend CMBIPC_EXPORT bool operator==(const CodeCompletion &first, const CodeCompletion &second);
    friend CMBIPC_EXPORT bool operator<(const CodeCompletion &first, const CodeCompletion &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletion &command);
    friend void PrintTo(const CodeCompletion &command, ::std::ostream* os);

public:
    enum Kind : quint32 {
        Other = 0,
        FunctionCompletionKind,
        TemplateFunctionCompletionKind,
        ConstructorCompletionKind,
        DestructorCompletionKind,
        VariableCompletionKind,
        ClassCompletionKind,
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

private:
    quint32 &completionKindAsInt();
    quint32 &availabilityAsInt();

private:
    Utf8String text_;
    CodeCompletionChunks chunks_;
    quint32 priority_ = 0;
    Kind completionKind_ = Other;
    Availability availability_ = NotAvailable;
    bool hasParameters_ = false;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CodeCompletion &command);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CodeCompletion &command);
CMBIPC_EXPORT bool operator==(const CodeCompletion &first, const CodeCompletion &second);
CMBIPC_EXPORT bool operator<(const CodeCompletion &first, const CodeCompletion &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletion &command);
CMBIPC_EXPORT QDebug operator<<(QDebug debug, CodeCompletion::Kind kind);

void PrintTo(const CodeCompletion &command, ::std::ostream* os);
void PrintTo(CodeCompletion::Kind kind, ::std::ostream *os);
void PrintTo(CodeCompletion::Availability availability, ::std::ostream *os);
} // namespace ClangBackEnd

Q_DECLARE_METATYPE(ClangBackEnd::CodeCompletion)

#endif // CLANGBACKEND_CODECOMPLETION_H
