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

#include <utils/cpplanguage_details.h>
#include <utils/optional.h>
#include <utils/smallstringio.h>
#include <utils/variant.h>

#include <clangsupport_global.h>

#include <QtGlobal>

#include <iosfwd>

#include <gtest/gtest-printers.h>

class Utf8String;
void PrintTo(const Utf8String &text, ::std::ostream *os);

namespace Sqlite {
class Value;
class ValueView;
class SessionChangeSet;
enum class Operation : char;
enum class LockingMode : char;
class TimeStamp;

std::ostream &operator<<(std::ostream &out, const Value &value);
std::ostream &operator<<(std::ostream &out, const ValueView &value);
std::ostream &operator<<(std::ostream &out, Operation operation);
std::ostream &operator<<(std::ostream &out, const SessionChangeSet &changeset);
std::ostream &operator<<(std::ostream &out, LockingMode lockingMode);
std::ostream &operator<<(std::ostream &out, TimeStamp timeStamp);

namespace SessionChangeSetInternal {
class ConstIterator;
class ConstTupleIterator;
class SentinelIterator;
class Tuple;
class ValueViews;
enum class State : char;

std::ostream &operator<<(std::ostream &out, SentinelIterator iterator);
std::ostream &operator<<(std::ostream &out, const ConstIterator &iterator);
std::ostream &operator<<(std::ostream &out, const ConstTupleIterator &iterator);
std::ostream &operator<<(std::ostream &out, const Tuple &tuple);
std::ostream &operator<<(std::ostream &out, State operation);
std::ostream &operator<<(std::ostream &out, const ValueViews &valueViews);

} // namespace SessionChangeSetInternal
} // namespace Sqlite

namespace Utils {
class LineColumn;
class SmallStringView;
class FilePath;

std::ostream &operator<<(std::ostream &out, const LineColumn &lineColumn);
std::ostream &operator<<(std::ostream &out, const Utils::Language &language);
std::ostream &operator<<(std::ostream &out, const Utils::LanguageVersion &languageVersion);
std::ostream &operator<<(std::ostream &out, const Utils::LanguageExtension &languageExtension);
std::ostream &operator<<(std::ostream &out, const FilePath &filePath);

template<typename Type>
std::ostream &operator<<(std::ostream &out, const optional<Type> &optional)
{
    if (optional)
        return out << "optional " << optional.value();
    else
        return out << "empty optional()";
}

template<typename Type>
void PrintTo(const optional<Type> &optional, ::std::ostream *os)
{
    *os << optional;
}

template<typename... Type>
std::ostream &operator<<(std::ostream &out, const variant<Type...> &variant)
{
    return Utils::visit([&](auto &&value) -> std::ostream & { return out << value; }, variant);
}

void PrintTo(Utils::SmallStringView text, ::std::ostream *os);
void PrintTo(const Utils::SmallString &text, ::std::ostream *os);
void PrintTo(const Utils::PathString &text, ::std::ostream *os);

} // namespace Utils

namespace ClangBackEnd {
class AliveMessage;
class CompletionsMessage;
class EchoMessage;
class AnnotationsMessage;
class ReferencesMessage;
class ToolTipMessage;
class FollowSymbolResult;
class FollowSymbolMessage;
class RequestCompletionsMessage;
class EndMessage;
class DocumentsOpenedMessage;
class DocumentsClosedMessage;
class CodeCompletion;
class CodeCompletionChunk;
class DiagnosticContainer;
class FileContainer;
class FixItContainer;
class FullTokenInfo;
class HighlightingMarkContainer;
class UnsavedFilesUpdatedMessage;
class RequestAnnotationsMessage;
class RequestFollowSymbolMessage;
class RequestReferencesMessage;
class RequestToolTipMessage;
class SourceLocationContainer;
class SourceRangeContainer;
class TokenInfo;
template<class T>
class TokenProcessor;
class DocumentsChangedMessage;
class DocumentVisibilityChangedMessage;
class ToolTipInfo;
class SuspendResumeJobsEntry;
class ReferencesResult;
struct ExtraInfo;
class TokenInfoContainer;
class UnsavedFilesRemovedMessage;

std::ostream &operator<<(std::ostream &out, const AliveMessage &message);
std::ostream &operator<<(std::ostream &out, const CompletionsMessage &message);
std::ostream &operator<<(std::ostream &out, const EchoMessage &message);
std::ostream &operator<<(std::ostream &out, const AnnotationsMessage &message);
std::ostream &operator<<(std::ostream &out, const ReferencesMessage &message);
std::ostream &operator<<(std::ostream &out, const ToolTipMessage &message);
std::ostream &operator<<(std::ostream &out, const FollowSymbolResult &result);
std::ostream &operator<<(std::ostream &out, const FollowSymbolMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestCompletionsMessage &message);
std::ostream &operator<<(std::ostream &out, const EndMessage &message);
std::ostream &operator<<(std::ostream &out, const DocumentsOpenedMessage &message);
std::ostream &operator<<(std::ostream &out, const DocumentsClosedMessage &message);
std::ostream &operator<<(std::ostream &out, const CodeCompletion &message);
std::ostream &operator<<(std::ostream &out, const CodeCompletionChunk &chunk);
std::ostream &operator<<(std::ostream &out, const DiagnosticContainer &container);
std::ostream &operator<<(std::ostream &out, const FileContainer &container);
std::ostream &operator<<(std::ostream &out, const FixItContainer &container);
std::ostream &operator<<(std::ostream &out, HighlightingType highlightingType);
std::ostream &operator<<(std::ostream &out, HighlightingTypes types);
std::ostream &operator<<(std::ostream &out, const HighlightingMarkContainer &container);
std::ostream &operator<<(std::ostream &out, const UnsavedFilesUpdatedMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestAnnotationsMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestFollowSymbolMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestReferencesMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestToolTipMessage &message);
std::ostream &operator<<(std::ostream &out, const ToolTipInfo &info);
std::ostream &operator<<(std::ostream &out, const SourceLocationContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceRangeContainer &container);
std::ostream &operator<<(std::ostream &out, const DocumentsChangedMessage &message);
std::ostream &operator<<(std::ostream &out, const DocumentVisibilityChangedMessage &message);
std::ostream &operator<<(std::ostream &out, const TokenInfo& tokenInfo);
template<class T>
std::ostream &operator<<(std::ostream &out, const TokenProcessor<T> &tokenInfos);
extern template
std::ostream &operator<<(std::ostream &out, const TokenProcessor<TokenInfo> &tokenInfos);
extern template std::ostream &operator<<(std::ostream &out,
                                         const TokenProcessor<FullTokenInfo> &tokenInfos);
std::ostream &operator<<(std::ostream &out, SymbolKind symbolKind);
std::ostream &operator<<(std::ostream &out, SymbolTag symbolTag);
std::ostream &operator<<(std::ostream &out, SymbolTags symbolTags);
std::ostream &operator<<(std::ostream &os, const SuspendResumeJobsEntry &entry);
std::ostream &operator<<(std::ostream &os, const ReferencesResult &value);
std::ostream &operator<<(std::ostream &os, const ExtraInfo &extraInfo);
std::ostream &operator<<(std::ostream &os, const TokenInfoContainer &container);
std::ostream &operator<<(std::ostream &os, const UnsavedFilesRemovedMessage &message);

} // namespace ClangBackEnd

namespace Debugger {
class DiagnosticLocation;
std::ostream &operator<<(std::ostream &out, const DiagnosticLocation &loc);
} // namespace Debugger

namespace ClangTools {
namespace Internal {
class ExplainingStep;
class Diagnostic;
std::ostream &operator<<(std::ostream &out, const ExplainingStep &step);
std::ostream &operator<<(std::ostream &out, const Diagnostic &diag);
} // namespace Internal
} // namespace ClangTools

namespace QmlDesigner {
class ModelNode;
class VariantProperty;
template<auto Type, typename InternalIntergerType>
class BasicId;
class WatcherEntry;
class IdPaths;
class ProjectChunkId;
enum class SourceType : int;
class FileStatus;

std::ostream &operator<<(std::ostream &out, const ModelNode &node);
std::ostream &operator<<(std::ostream &out, const VariantProperty &property);

template<auto Type, typename InternalIntergerType>
std::ostream &operator<<(std::ostream &out, const BasicId<Type, InternalIntergerType> &id)
{
    return out << "id=" << &id;
}

std::ostream &operator<<(std::ostream &out, const WatcherEntry &entry);
std::ostream &operator<<(std::ostream &out, const IdPaths &idPaths);
std::ostream &operator<<(std::ostream &out, const ProjectChunkId &id);
std::ostream &operator<<(std::ostream &out, SourceType sourceType);
std::ostream &operator<<(std::ostream &out, const FileStatus &fileStatus);

namespace Cache {
class SourceContext;

std::ostream &operator<<(std::ostream &out, const SourceContext &sourceContext);
} // namespace Cache

namespace Storage {
class Type;
class ExportedType;
class ImportedType;
class QualifiedImportedType;
class Version;
class VersionNumber;
enum class TypeAccessSemantics : int;
enum class PropertyDeclarationTraits : unsigned int;
class PropertyDeclaration;
class FunctionDeclaration;
class ParameterDeclaration;
class SignalDeclaration;
class EnumerationDeclaration;
class EnumeratorDeclaration;
enum class ImportKind : char;
class Import;
enum class IsQualified : int;
class ProjectData;
class SynchronizationPackage;
enum class FileType : char;
enum class ChangeLevel : char;

std::ostream &operator<<(std::ostream &out, TypeAccessSemantics accessSemantics);
std::ostream &operator<<(std::ostream &out, VersionNumber versionNumber);
std::ostream &operator<<(std::ostream &out, Version version);
std::ostream &operator<<(std::ostream &out, const Type &type);
std::ostream &operator<<(std::ostream &out, const ExportedType &exportedType);
std::ostream &operator<<(std::ostream &out, const ImportedType &importedType);
std::ostream &operator<<(std::ostream &out, const QualifiedImportedType &importedType);
std::ostream &operator<<(std::ostream &out, const PropertyDeclaration &propertyDeclaration);
std::ostream &operator<<(std::ostream &out, PropertyDeclarationTraits traits);
std::ostream &operator<<(std::ostream &out, const FunctionDeclaration &functionDeclaration);
std::ostream &operator<<(std::ostream &out, const ParameterDeclaration &parameter);
std::ostream &operator<<(std::ostream &out, const SignalDeclaration &signalDeclaration);
std::ostream &operator<<(std::ostream &out, const EnumerationDeclaration &enumerationDeclaration);
std::ostream &operator<<(std::ostream &out, const EnumeratorDeclaration &enumeratorDeclaration);
std::ostream &operator<<(std::ostream &out, const ImportKind &importKind);
std::ostream &operator<<(std::ostream &out, const Import &import);
std::ostream &operator<<(std::ostream &out, IsQualified isQualified);
std::ostream &operator<<(std::ostream &out, const ProjectData &data);
std::ostream &operator<<(std::ostream &out, const SynchronizationPackage &package);
std::ostream &operator<<(std::ostream &out, FileType fileType);
std::ostream &operator<<(std::ostream &out, ChangeLevel changeLevel);

} // namespace Storage

} // namespace QmlDesigner
