// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <designercore/model/modelresourcemanagementfwd.h>
#include <utils/cpplanguage_details.h>
#include <utils/smallstringio.h>

#include <QtGlobal>

#include <iosfwd>
#include <optional>
#include <variant>

namespace Sqlite {
class Value;
class ValueView;
class SessionChangeSet;
enum class Operation : char;
enum class LockingMode : char;
class TimeStamp;
template<auto Type, typename InternalIntegerType>
class BasicId;

std::ostream &operator<<(std::ostream &out, const Value &value);
std::ostream &operator<<(std::ostream &out, const ValueView &value);
std::ostream &operator<<(std::ostream &out, Operation operation);
std::ostream &operator<<(std::ostream &out, const SessionChangeSet &changeset);
std::ostream &operator<<(std::ostream &out, LockingMode lockingMode);
std::ostream &operator<<(std::ostream &out, TimeStamp timeStamp);

template<auto Type, typename InternalIntegerType>
std::ostream &operator<<(std::ostream &out, const BasicId<Type, InternalIntegerType> &id)
{
    return out << "id=" << id.internalId();
}

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

namespace std {

template<typename Type, typename... Types>
std::ostream &operator<<(std::ostream &out, const variant<Type, Types...> &v)
{
    return visit([&](auto &&value) -> std::ostream & { return out << value; }, v);
}

std::ostream &operator<<(std::ostream &out, const monostate &);

} // namespace std

namespace Utils {
class LineColumn;
class SmallStringView;
class FilePath;

std::ostream &operator<<(std::ostream &out, const Utils::Language &language);
std::ostream &operator<<(std::ostream &out, const Utils::LanguageVersion &languageVersion);
std::ostream &operator<<(std::ostream &out, const Utils::LanguageExtension &languageExtension);
std::ostream &operator<<(std::ostream &out, const FilePath &filePath);

template<typename Type>
std::ostream &operator<<(std::ostream &out, const std::optional<Type> &optional)
{
    if (optional)
        return out << "optional " << optional.value();
    else
        return out << "empty optional()";
}

template<typename Type>
void PrintTo(const std::optional<Type> &optional, ::std::ostream *os)
{
    *os << optional;
}

void PrintTo(Utils::SmallStringView text, ::std::ostream *os);
void PrintTo(const Utils::SmallString &text, ::std::ostream *os);
void PrintTo(const Utils::PathString &text, ::std::ostream *os);

} // namespace Utils

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
class AbstractProperty;
class WatcherEntry;
class IdPaths;
class ProjectChunkId;
enum class SourceType : int;
class FileStatus;
class Import;
class NodeMetaInfo;
class PropertyMetaInfo;
struct CompoundPropertyMetaInfo;

std::ostream &operator<<(std::ostream &out, const ModelNode &node);
std::ostream &operator<<(std::ostream &out, const VariantProperty &property);
std::ostream &operator<<(std::ostream &out, const AbstractProperty &property);
std::ostream &operator<<(std::ostream &out, const WatcherEntry &entry);
std::ostream &operator<<(std::ostream &out, const IdPaths &idPaths);
std::ostream &operator<<(std::ostream &out, const ProjectChunkId &id);
std::ostream &operator<<(std::ostream &out, SourceType sourceType);
std::ostream &operator<<(std::ostream &out, const FileStatus &fileStatus);
std::ostream &operator<<(std::ostream &out, const Import &import);
std::ostream &operator<<(std::ostream &out, const ModelResourceSet::SetExpression &setExpression);
std::ostream &operator<<(std::ostream &out, const ModelResourceSet &modelResourceSet);
std::ostream &operator<<(std::ostream &out, const NodeMetaInfo &metaInfo);
std::ostream &operator<<(std::ostream &out, const PropertyMetaInfo &metaInfo);
std::ostream &operator<<(std::ostream &out, const CompoundPropertyMetaInfo &metaInfo);

namespace Cache {
class SourceContext;

std::ostream &operator<<(std::ostream &out, const SourceContext &sourceContext);
} // namespace Cache

namespace ImageCache {
class LibraryIconAuxiliaryData;
class FontCollectorSizeAuxiliaryData;
class FontCollectorSizesAuxiliaryData;

std::ostream &operator<<(std::ostream &out, const LibraryIconAuxiliaryData &date);
std::ostream &operator<<(std::ostream &out, const FontCollectorSizeAuxiliaryData &sourceContext);
std::ostream &operator<<(std::ostream &out, const FontCollectorSizesAuxiliaryData &sourceContext);
} // namespace ImageCache

namespace Storage {
enum class PropertyDeclarationTraits : int;
enum class TypeTraits : int;
class Import;
class Version;
class VersionNumber;

std::ostream &operator<<(std::ostream &out, PropertyDeclarationTraits traits);
std::ostream &operator<<(std::ostream &out, TypeTraits traits);
std::ostream &operator<<(std::ostream &out, const Import &import);
std::ostream &operator<<(std::ostream &out, VersionNumber versionNumber);
std::ostream &operator<<(std::ostream &out, Version version);

} // namespace Storage

namespace Storage::Info {
class ProjectDeclaration;
class Type;
class ExportedTypeName;

std::ostream &operator<<(std::ostream &out, const ProjectDeclaration &declaration);
std::ostream &operator<<(std::ostream &out, const Type &type);
std::ostream &operator<<(std::ostream &out, const ExportedTypeName &name);

} // namespace Storage::Info

namespace Storage::Synchronization {
class Type;
class ExportedType;
class ImportedType;
class QualifiedImportedType;
class PropertyDeclaration;
class FunctionDeclaration;
class ParameterDeclaration;
class SignalDeclaration;
class EnumerationDeclaration;
class EnumeratorDeclaration;
enum class ImportKind : char;
enum class IsAutoVersion : char;
enum class IsQualified : int;
class ProjectData;
class SynchronizationPackage;
enum class FileType : char;
enum class ChangeLevel : char;
class ModuleExportedImport;
class PropertyEditorQmlPath;

std::ostream &operator<<(std::ostream &out, const Type &type);
std::ostream &operator<<(std::ostream &out, const ExportedType &exportedType);
std::ostream &operator<<(std::ostream &out, const ImportedType &importedType);
std::ostream &operator<<(std::ostream &out, const QualifiedImportedType &importedType);
std::ostream &operator<<(std::ostream &out, const PropertyDeclaration &propertyDeclaration);
std::ostream &operator<<(std::ostream &out, const FunctionDeclaration &functionDeclaration);
std::ostream &operator<<(std::ostream &out, const ParameterDeclaration &parameter);
std::ostream &operator<<(std::ostream &out, const SignalDeclaration &signalDeclaration);
std::ostream &operator<<(std::ostream &out, const EnumerationDeclaration &enumerationDeclaration);
std::ostream &operator<<(std::ostream &out, const EnumeratorDeclaration &enumeratorDeclaration);
std::ostream &operator<<(std::ostream &out, const ImportKind &importKind);
std::ostream &operator<<(std::ostream &out, IsQualified isQualified);
std::ostream &operator<<(std::ostream &out, const ProjectData &data);
std::ostream &operator<<(std::ostream &out, const SynchronizationPackage &package);
std::ostream &operator<<(std::ostream &out, FileType fileType);
std::ostream &operator<<(std::ostream &out, ChangeLevel changeLevel);
std::ostream &operator<<(std::ostream &out, const ModuleExportedImport &import);
std::ostream &operator<<(std::ostream &out, const PropertyEditorQmlPath &path);

} // namespace Storage::Synchronization

} // namespace QmlDesigner
