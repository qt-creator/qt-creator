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
#include <utils/smallstringio.h>
#include <utils/optional.h>

#include <clangsupport_global.h>

#include <QtGlobal>

#include <iosfwd>

#include <gtest/gtest-printers.h>

class Utf8String;
void PrintTo(const Utf8String &text, ::std::ostream *os);

namespace clang {
namespace tooling {
struct CompileCommand;

std::ostream &operator<<(std::ostream &out, const CompileCommand &command);
} // namespace tooling
} // namespace clang

namespace Core {
struct LocatorFilterEntry;

std::ostream &operator<<(std::ostream &out, const LocatorFilterEntry &entry);

namespace Search {

class TextPosition;
class TextRange;

void PrintTo(const TextPosition &position, ::std::ostream *os);
void PrintTo(const TextRange &range, ::std::ostream *os);

} // namespace TextPosition
} // namespace TextPosition

namespace Sqlite {
class Value;
class SessionChangeSet;

std::ostream &operator<<(std::ostream &out, const Value &value);
std::ostream &operator<<(std::ostream &out, const SessionChangeSet &changeset);
} // namespace Sqlite

namespace ProjectExplorer {

enum class MacroType;
class Macro;
enum class HeaderPathType;
class HeaderPath;

std::ostream &operator<<(std::ostream &out, const MacroType &type);
std::ostream &operator<<(std::ostream &out, const Macro &macro);
std::ostream &operator<<(std::ostream &out, const HeaderPathType &headerPathType);
std::ostream &operator<<(std::ostream &out, const HeaderPath &headerPath);

} // namespace ClangRefactoring

namespace Utils {
class LineColumn;
class SmallStringView;
struct Link;

std::ostream &operator<<(std::ostream &out, const LineColumn &lineColumn);
std::ostream &operator<<(std::ostream &out, const Utils::Language &language);
std::ostream &operator<<(std::ostream &out, const Utils::LanguageVersion &languageVersion);
std::ostream &operator<<(std::ostream &out, const Utils::LanguageExtension &languageExtension);
std::ostream &operator<<(std::ostream &out, const Link &link);

template <typename Type>
std::ostream &operator<<(std::ostream &out, const Utils::optional<Type> &optional)
{
    if (optional)
        return out << "optional " << optional.value();
    else
        return out << "empty optional()";
}

template <typename Type>
void PrintTo(const Utils::optional<Type> &optional, ::std::ostream *os)
{
    *os << optional;
}

void PrintTo(Utils::SmallStringView text, ::std::ostream *os);
void PrintTo(const Utils::SmallString &text, ::std::ostream *os);
void PrintTo(const Utils::PathString &text, ::std::ostream *os);

} // namespace Utils

namespace ClangBackEnd {
class SourceLocationEntry;
class IdPaths;
class FilePathId;
class FilePath;
class WatcherEntry;
class SourceLocationsContainer;
class CancelMessage;
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
class DynamicASTMatcherDiagnosticContainer;
class DynamicASTMatcherDiagnosticContextContainer;
class DynamicASTMatcherDiagnosticMessageContainer;
class FileContainer;
class FixItContainer;
class FullTokenInfo;
class HighlightingMarkContainer;
class NativeFilePath;
class PrecompiledHeadersUpdatedMessage;
class ProjectPartPch;
class UnsavedFilesUpdatedMessage;
class RemoveProjectPartsMessage;
class RequestAnnotationsMessage;
class RequestFollowSymbolMessage;
class RequestReferencesMessage;
class RequestToolTipMessage;
class RequestSourceRangesAndDiagnosticsForQueryMessage;
class RequestSourceRangesForQueryMessage;
class SourceLocationContainer;
class SourceRangeContainer;
class SourceRangesAndDiagnosticsForQueryMessage;
class SourceRangesContainer;
class SourceRangesForQueryMessage;
class SourceRangeWithTextContainer;
class TokenInfo;
template<class T>
class TokenProcessor;
class UnsavedFilesRemovedMessage;
class UpdateProjectPartsMessage;
class DocumentsChangedMessage;
class DocumentVisibilityChangedMessage;
class FilePath;
template<char WindowsSlash>
class AbstractFilePathView;
using FilePathView = AbstractFilePathView<'/'>;
using NativeFilePathView = AbstractFilePathView<'\\'>;
class ToolTipInfo;
class ProjectPartEntry;
class UsedMacro;
class FileStatus;
class SourceDependency;
class ProjectPartArtefact;
class CompilerMacro;
class SymbolEntry;
enum class SymbolKind : uchar;
enum class SymbolTag : uchar;
using SymbolTags = Utils::SizedArray<SymbolTag, 7>;
class UpdateGeneratedFilesMessage;
class RemoveGeneratedFilesMessage;
class SuspendResumeJobsEntry;
class ReferencesResult;
class SymbolIndexerTask;
class ProgressMessage;
class PchTask;
class PchTaskSet;
class BuildDependency;
class SourceEntry;
class SourceTimeStamp;
class TimeStamp;
class FilePathCaching;
struct SlotUsage;
class IncludeSearchPath;
enum class IncludeSearchPathType : unsigned char;
struct ArgumentsEntry;
class ProjectPartContainer;
class ProjectPartId;
class PchPaths;
class ProjectChunkId;
class DirectoryPathId;

std::ostream &operator<<(std::ostream &out, const SourceLocationEntry &entry);
std::ostream &operator<<(std::ostream &out, const IdPaths &idPaths);
std::ostream &operator<<(std::ostream &out, const WatcherEntry &entry);
std::ostream &operator<<(std::ostream &out, const SourceLocationsContainer &container);
std::ostream &operator<<(std::ostream &out, const CancelMessage &message);
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
std::ostream &operator<<(std::ostream &out, const DynamicASTMatcherDiagnosticContainer &container);
std::ostream &operator<<(std::ostream &out, const DynamicASTMatcherDiagnosticContextContainer &container);
std::ostream &operator<<(std::ostream &out, const DynamicASTMatcherDiagnosticMessageContainer &container);
std::ostream &operator<<(std::ostream &out, const FileContainer &container);
std::ostream &operator<<(std::ostream &out, const FixItContainer &container);
std::ostream &operator<<(std::ostream &out, HighlightingType highlightingType);
std::ostream &operator<<(std::ostream &out, HighlightingTypes types);
std::ostream &operator<<(std::ostream &out, const HighlightingMarkContainer &container);
std::ostream &operator<<(std::ostream &out, const NativeFilePath &filePath);
std::ostream &operator<<(std::ostream &out, const PrecompiledHeadersUpdatedMessage &message);
std::ostream &operator<<(std::ostream &out, const ProjectPartPch &projectPartPch);
std::ostream &operator<<(std::ostream &out, const UnsavedFilesUpdatedMessage &message);
std::ostream &operator<<(std::ostream &out, const RemoveProjectPartsMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestAnnotationsMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestFollowSymbolMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestReferencesMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestToolTipMessage &message);
std::ostream &operator<<(std::ostream &out, const ToolTipInfo &info);
std::ostream &operator<<(std::ostream &out,
                         const RequestSourceRangesAndDiagnosticsForQueryMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestSourceRangesForQueryMessage &message);
std::ostream &operator<<(std::ostream &out, const SourceLocationContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceRangeContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceRangesAndDiagnosticsForQueryMessage &message);
std::ostream &operator<<(std::ostream &out, const SourceRangesContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceRangesForQueryMessage &message);
std::ostream &operator<<(std::ostream &out, const SourceRangeWithTextContainer &container);
std::ostream &operator<<(std::ostream &out, const UnsavedFilesRemovedMessage &message);
std::ostream &operator<<(std::ostream &out, const UpdateProjectPartsMessage &message);
std::ostream &operator<<(std::ostream &out, const DocumentsChangedMessage &message);
std::ostream &operator<<(std::ostream &out, const DocumentVisibilityChangedMessage &message);
std::ostream &operator<<(std::ostream &out, const FilePath &filePath);
std::ostream &operator<<(std::ostream &out, const FilePathId &filePathId);
std::ostream &operator<<(std::ostream &out, const TokenInfo& tokenInfo);
template<class T>
std::ostream &operator<<(std::ostream &out, const TokenProcessor<T> &tokenInfos);
extern template
std::ostream &operator<<(std::ostream &out, const TokenProcessor<TokenInfo> &tokenInfos);
extern template
std::ostream &operator<<(std::ostream &out, const TokenProcessor<FullTokenInfo> &tokenInfos);
std::ostream &operator<<(std::ostream &out, const FilePathView &filePathView);
std::ostream &operator<<(std::ostream &out, const NativeFilePathView &nativeFilePathView);
std::ostream &operator<<(std::ostream &out, const ProjectPartEntry &projectPartEntry);
std::ostream &operator<<(std::ostream &out, const UsedMacro &usedMacro);
std::ostream &operator<<(std::ostream &out, const FileStatus &fileStatus);
std::ostream &operator<<(std::ostream &out, const SourceDependency &sourceDependency);
std::ostream &operator<<(std::ostream &out, const ProjectPartArtefact &projectPartArtefact);
std::ostream &operator<<(std::ostream &out, const CompilerMacro &compilerMacro);
std::ostream &operator<<(std::ostream &out, const SymbolEntry &symbolEntry);
std::ostream &operator<<(std::ostream &out, SymbolKind symbolKind);
std::ostream &operator<<(std::ostream &out, SymbolTag symbolTag);
std::ostream &operator<<(std::ostream &out, SymbolTags symbolTags);
std::ostream &operator<<(std::ostream &out, const UpdateGeneratedFilesMessage &message);
std::ostream &operator<<(std::ostream &out, const RemoveGeneratedFilesMessage &message);
std::ostream &operator<<(std::ostream &os, const SuspendResumeJobsEntry &entry);
std::ostream &operator<<(std::ostream &os, const ReferencesResult &value);
std::ostream &operator<<(std::ostream &out, const SymbolIndexerTask &task);
std::ostream &operator<<(std::ostream &out, const ProgressMessage &message);
std::ostream &operator<<(std::ostream &out, const PchTask &task);
std::ostream &operator<<(std::ostream &out, const PchTaskSet &taskSet);
std::ostream &operator<<(std::ostream &out, const BuildDependency &dependency);
std::ostream &operator<<(std::ostream &out, const SourceEntry &entry);
std::ostream &operator<<(std::ostream &out, const SourceTimeStamp &sourceTimeStamp);
std::ostream &operator<<(std::ostream &out, const TimeStamp &timeStamp);
std::ostream &operator<<(std::ostream &out, const SlotUsage &slotUsage);
std::ostream &operator<<(std::ostream &out, const IncludeSearchPathType &pathType);
std::ostream &operator<<(std::ostream &out, const IncludeSearchPath &path);
std::ostream &operator<<(std::ostream &out, const ArgumentsEntry &entry);
std::ostream &operator<<(std::ostream &out, const ProjectPartContainer &container);
std::ostream &operator<<(std::ostream &out, const ProjectPartId &projectPathId);
std::ostream &operator<<(std::ostream &out, const PchPaths &pchPaths);
std::ostream &operator<<(std::ostream &out, const ProjectChunkId &chunk);
std::ostream &operator<<(std::ostream &out, const DirectoryPathId &id);

void PrintTo(const FilePath &filePath, ::std::ostream *os);
void PrintTo(const FilePathView &filePathView, ::std::ostream *os);
void PrintTo(const FilePathId &filePathId, ::std::ostream *os);

namespace V2 {
class FileContainer;
class SourceRangeContainer;
class SourceLocationContainer;

std::ostream &operator<<(std::ostream &out, const FileContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceLocationContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceRangeContainer &container);
}  // namespace V2

} // namespace ClangBackEnd

namespace ClangRefactoring {
class SourceLocation;
class Symbol;

std::ostream &operator<<(std::ostream &out, const SourceLocation &location);
std::ostream &operator<<(std::ostream &out, const Symbol &symbol);
} // namespace ClangRefactoring


namespace CppTools {
class Usage;

std::ostream &operator<<(std::ostream &out, const Usage &usage);
} // namespace CppTools

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
} // namespace CppTools

namespace QmlDesigner {
class ModelNode;
class VariantProperty;

std::ostream &operator<<(std::ostream &out, const ModelNode &node);
std::ostream &operator<<(std::ostream &out, const VariantProperty &property);
} // namespace QmlDesigner

void setFilePathCache(ClangBackEnd::FilePathCaching *filePathCache);
