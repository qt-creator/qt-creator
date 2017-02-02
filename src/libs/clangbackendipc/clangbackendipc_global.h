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

#ifdef UNIT_TESTS
#include <gtest/gtest.h>
#endif

#if defined(CLANGBACKENDIPC_BUILD_LIB)
#  define CMBIPC_EXPORT Q_DECL_EXPORT
#elif defined(CLANGBACKENDIPC_BUILD_STATIC_LIB)
#  define CMBIPC_EXPORT
#else
#  define CMBIPC_EXPORT Q_DECL_IMPORT
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
    LocalVariable,
    Field,
    GlobalVariable,
    Enumeration,
    Operator,
    Preprocessor,
    PreprocessorDefinition,
    PreprocessorExpansion,
    Label,
    OutputArgument,
    Declaration
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

    RegisterTranslationUnitForEditorMessage,
    UpdateTranslationUnitsForEditorMessage,
    UnregisterTranslationUnitsForEditorMessage,

    RegisterUnsavedFilesForEditorMessage,
    UnregisterUnsavedFilesForEditorMessage,

    RegisterProjectPartsForEditorMessage,
    UnregisterProjectPartsForEditorMessage,

    RequestDocumentAnnotationsMessage,
    DocumentAnnotationsChangedMessage,

    UpdateVisibleTranslationUnitsMessage,

    CompleteCodeMessage,
    CodeCompletedMessage,

    TranslationUnitDoesNotExistMessage,
    ProjectPartsDoNotExistMessage,

    SourceLocationsForRenamingMessage,
    RequestSourceLocationsForRenamingMessage,

    RequestSourceRangesAndDiagnosticsForQueryMessage,
    SourceRangesAndDiagnosticsForQueryMessage,

    CancelMessage,
    UpdatePchProjectPartsMessage,
    RemovePchProjectPartsMessage,
    PrecompiledHeadersUpdatedMessage
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
    HighlightingType mainHighlightingType;
    MixinHighlightingTypes mixinHighlightingTypes;
};

}
