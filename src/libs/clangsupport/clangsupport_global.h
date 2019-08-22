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

#include <utils/sizedarray.h>

#include <QtCore/qglobal.h>

#include <utils/smallstringfwd.h>

#if defined(CLANGSUPPORT_BUILD_LIB)
#  define CLANGSUPPORT_EXPORT Q_DECL_EXPORT
#elif defined(CLANGSUPPORT_BUILD_STATIC_LIB)
#  define CLANGSUPPORT_EXPORT
#else
#  define CLANGSUPPORT_EXPORT Q_DECL_IMPORT
#endif

#ifdef Q_CC_GNU
#  define CLANGSUPPORT_GCCEXPORT __attribute__((visibility("default")))
#else
#  define CLANGSUPPORT_GCCEXPORT
#endif

#ifndef CLANGBACKENDPROCESSPATH
# define CLANGBACKENDPROCESSPATH ""
#endif

#ifdef UNIT_TESTS
#define unittest_public public
#else
#define unittest_public private
#endif

namespace ClangBackEnd {

enum class DiagnosticSeverity : quint32 // one to one mapping of the clang enum numbers
{
    Ignored = 0,
    Note = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4
};

enum class HighlightingType : quint8
{
    Invalid,
    Keyword,
    StringLiteral,
    NumberLiteral,
    Comment,
    Function,
    VirtualFunction,
    Type,
    PrimitiveType,
    LocalVariable,
    Field,
    GlobalVariable,
    Enumeration,
    Operator,
    OverloadedOperator,
    Preprocessor,
    PreprocessorDefinition,
    PreprocessorExpansion,
    Punctuation,
    Label,
    Declaration,
    FunctionDefinition,
    OutputArgument,
    Namespace,
    Class,
    Struct,
    Enum,
    Union,
    TypeAlias,
    Typedef,
    QtProperty,
    ObjectiveCClass,
    ObjectiveCCategory,
    ObjectiveCProtocol,
    ObjectiveCInterface,
    ObjectiveCImplementation,
    ObjectiveCProperty,
    ObjectiveCMethod,
    TemplateTypeParameter,
    TemplateTemplateParameter
};

enum class StorageClass : quint8
{
    Invalid,
    None,
    Extern,
    Static,
    PrivateExtern,
    Auto,
    Register
};

enum class AccessSpecifier : quint8
{
    Invalid,
    Public,
    Protected,
    Private
};

enum class CompletionCorrection : quint32
{
    NoCorrection,
    DotToArrowCorrection
};

enum class MessageType : quint8 {
    InvalidMessage,
    AliveMessage,
    EchoMessage,
    EndMessage,

    DocumentsOpenedMessage,
    DocumentsChangedMessage,
    DocumentsClosedMessage,
    DocumentVisibilityChangedMessage,

    UnsavedFilesUpdatedMessage,
    UnsavedFilesRemovedMessage,

    RequestAnnotationsMessage,
    AnnotationsMessage,

    RequestReferencesMessage,
    ReferencesMessage,

    RequestFollowSymbolMessage,
    FollowSymbolMessage,

    RequestToolTipMessage,
    ToolTipMessage,

    RequestCompletionsMessage,
    CompletionsMessage,

    SourceLocationsForRenamingMessage,
    RequestSourceLocationsForRenamingMessage,

    RequestSourceRangesAndDiagnosticsForQueryMessage,
    RequestSourceRangesForQueryMessage,
    SourceRangesAndDiagnosticsForQueryMessage,
    SourceRangesForQueryMessage,

    CancelMessage,
    UpdateProjectPartsMessage,
    RemoveProjectPartsMessage,
    PrecompiledHeadersUpdatedMessage,
    UpdateGeneratedFilesMessage,
    RemoveGeneratedFilesMessage,
    ProgressMessage
};

template<MessageType messageEnumeration>
struct MessageTypeTrait;

template<class Message>
struct MessageTrait;

#define DECLARE_MESSAGE(Message) \
template<> \
struct MessageTrait<Message> \
{ \
    static const MessageType enumeration = MessageType::Message; \
};

using MixinHighlightingTypes = Utils::SizedArray<HighlightingType, 6>;

struct HighlightingTypes {
    HighlightingType mainHighlightingType = HighlightingType::Invalid;
    MixinHighlightingTypes mixinHighlightingTypes;
};

enum class SourceLocationKind : uchar {
    Definition = 1,
    Declaration,
    DeclarationReference,
    MacroDefinition = 128,
    MacroUndefinition,
    MacroUsage,
    None = 255,
};

enum class SymbolKind : uchar
{
    None = 0,
    Enumeration,
    Record,
    Function,
    Variable,
    Macro
};

using SymbolKinds = Utils::SizedArray<SymbolKind, 8>;

enum class SymbolTag : uchar
{
    None = 0,
    Class,
    Struct,
    Union,
    MsvcInterface
};

using SymbolTags = Utils::SizedArray<SymbolTag, 7>;

enum class ProgressType { Invalid, PrecompiledHeader, Indexing, DependencyCreation };
} // namespace ClangBackEnd
