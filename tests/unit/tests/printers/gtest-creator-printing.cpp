// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtest-creator-printing.h"
#include "gtest-std-printing.h"

#include "gtest-qt-printing.h"

#include <gtest/gtest-printers.h>
#include <gmock/gmock-matchers.h>

#include <3rdparty/sqlite/sqlite.h>
#include <imagecache/imagecachestorageinterface.h>
#include <imagecacheauxiliarydata.h>
#include <import.h>
#include <modelnode.h>
#include <nodemetainfo.h>
#include <projectstorage/filestatus.h>
#include <projectstorage/projectstoragepathwatchertypes.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorage/sourcepathcachetypes.h>
#include <sqlitesessionchangeset.h>
#include <sqlitevalue.h>
#include <utils/fileutils.h>
#include <variantproperty.h>

namespace std {
template <typename T> ostream &operator<<(ostream &out, const QVector<T> &vector)
{
    out << "[";
    copy(vector.cbegin(), vector.cend(), ostream_iterator<T>(out, ", "));
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const monostate &)
{
    return out << "monostate";
}

} // namespace std

namespace Utils {
namespace {
const char * toText(Utils::Language language)
{
    using Utils::Language;

    switch (language) {
    case Language::C:
        return "C";
    case Language::Cxx:
        return "Cxx";
    case Language::None:
        return "None";
    }

    return "";
}
} // namespace

std::ostream &operator<<(std::ostream &out, const Utils::Language &language)
{
    return out << "Utils::" << toText(language);
}

namespace {
const char * toText(Utils::LanguageVersion languageVersion)
{
    using Utils::LanguageVersion;

    switch (languageVersion) {
    case LanguageVersion::C11:
        return "C11";
    case LanguageVersion::C18:
        return "C18";
    case LanguageVersion::C89:
        return "C89";
    case LanguageVersion::C99:
        return "C99";
    case LanguageVersion::CXX03:
        return "CXX03";
    case LanguageVersion::CXX11:
        return "CXX11";
    case LanguageVersion::CXX14:
        return "CXX14";
    case LanguageVersion::CXX17:
        return "CXX17";
    case LanguageVersion::CXX20:
        return "CXX20";
    case LanguageVersion::CXX2b:
        return "CXX2b";
    case LanguageVersion::CXX98:
        return "CXX98";
    case LanguageVersion::None:
        return "None";
    }

    return "";
}
} // namespace

std::ostream &operator<<(std::ostream &out, const Utils::LanguageVersion &languageVersion)
{
     return out << "Utils::" << toText(languageVersion);
}

namespace {
const std::string toText(Utils::LanguageExtension extension, std::string prefix = {})
{
    std::stringstream out;
    using Utils::LanguageExtension;

    if (extension == LanguageExtension::None) {
        out << prefix << "None";
    } else if (extension == LanguageExtension::All) {
        out << prefix << "All";
    } else {
        std::string split = "";
        if (extension == LanguageExtension::Gnu) {
            out << prefix << "Gnu";
            split = " | ";
        }
        if (extension == LanguageExtension::Microsoft) {
            out << split << prefix << "Microsoft";
            split = " | ";
        }
        if (extension == LanguageExtension::Borland) {
            out << split << prefix << "Borland";
            split = " | ";
        }
        if (extension == LanguageExtension::OpenMP) {
            out << split << prefix << "OpenMP";
            split = " | ";
        }
        if (extension == LanguageExtension::ObjectiveC) {
            out << split << prefix << "ObjectiveC";
        }
    }

    return out.str();
}
} // namespace

std::ostream &operator<<(std::ostream &out, const Utils::LanguageExtension &languageExtension)
{
    return out << toText(languageExtension, "Utils::");
}

void PrintTo(Utils::SmallStringView text, ::std::ostream *os)
{
    *os << "\"" << text << "\"";
}

void PrintTo(const Utils::SmallString &text, ::std::ostream *os)
{
    *os << "\"" << text << "\"";
}

void PrintTo(const Utils::PathString &text, ::std::ostream *os)
{
    *os << "\"" << text << "\"";
}

std::ostream &operator<<(std::ostream &out, const FilePath &filePath)
{
    return out << filePath.toString();
}

} // namespace Utils

namespace Sqlite {
std::ostream &operator<<(std::ostream &out, const Value &value)
{
    out << "(";

    switch (value.type()) {
    case Sqlite::ValueType::Integer:
        out << value.toInteger();
        break;
    case Sqlite::ValueType::Float:
        out << value.toFloat();
        break;
    case Sqlite::ValueType::String:
        out << "\"" << value.toStringView() << "\"";
        break;
    case Sqlite::ValueType::Blob:
        out << "blob";
        break;
    case Sqlite::ValueType::Null:
        out << "null";
        break;
    }

    return out << ")";
}

std::ostream &operator<<(std::ostream &out, const ValueView &value)
{
    out << "(";

    switch (value.type()) {
    case Sqlite::ValueType::Integer:
        out << value.toInteger();
        break;
    case Sqlite::ValueType::Float:
        out << value.toFloat();
        break;
    case Sqlite::ValueType::String:
        out << "\"" << value.toStringView() << "\"";
        break;
    case Sqlite::ValueType::Blob:
        out << "blob";
        break;
    case Sqlite::ValueType::Null:
        out << "null";
        break;
    }

    return out << ")";
}

namespace {
#if 0
[[maybe_unused]]
Utils::SmallStringView operationText(int operation)
{
    switch (operation) {
    case SQLITE_INSERT:
        return "INSERT";
    case SQLITE_UPDATE:
        return "UPDATE";
    case SQLITE_DELETE:
        return "DELETE";
    }

    return {};
}

std::ostream &operator<<(std::ostream &out, [[maybe_unused]] sqlite3_changeset_iter *iter)
{
    out << "(";

    const char *tableName = nullptr;
    int columns = 0;
    int operation = 0;
    sqlite3_value *value = nullptr;

    sqlite3changeset_op(iter, &tableName, &columns, &operation, 0);

    out << operationText(operation) << " " << tableName << " {";

    if (operation == SQLITE_UPDATE || operation == SQLITE_DELETE) {
        out << "Old: [";

        for (int i = 0; i < columns; i++) {
            sqlite3changeset_old(iter, i, &value);

            if (value)
                out << " " << sqlite3_value_text(value);
            else
                out << " -";
        }
        out << "]";
    }

    if (operation == SQLITE_UPDATE)
        out << ", ";

    if (operation == SQLITE_UPDATE || operation == SQLITE_INSERT) {
        out << "New: [";
        for (int i = 0; i < columns; i++) {
            sqlite3changeset_new(iter, i, &value);

            if (value)
                out << " " << sqlite3_value_text(value);
            else
                out << " -";
        }
        out << "]";
    }

    out << "})";
    return out;
}
#endif

const char *toText(Operation operation)
{
    switch (operation) {
    case Operation::Invalid:
        return "Invalid";
    case Operation::Insert:
        return "Invalid";
    case Operation::Update:
        return "Invalid";
    case Operation::Delete:
        return "Invalid";
    }

    return "";
}

const char *toText(LockingMode lockingMode)
{
    switch (lockingMode) {
    case LockingMode::Default:
        return "Default";
    case LockingMode::Normal:
        return "Normal";
    case LockingMode::Exclusive:
        return "Exclusive";
    }

    return "";
}
} // namespace

std::ostream &operator<<(std::ostream &out, [[maybe_unused]] const SessionChangeSet &changeset)
{
    Q_UNUSED(changeset)
#if 0
    sqlite3_changeset_iter *iter = nullptr;
    sqlite3changeset_start(&iter, changeset.size(), const_cast<void *>(changeset.data()));

    out << "ChangeSets([";

    if (SQLITE_ROW == sqlite3changeset_next(iter)) {
        out << iter;
        while (SQLITE_ROW == sqlite3changeset_next(iter))
            out << ", " << iter;
    }

    sqlite3changeset_finalize(iter);

    out << "])";
#endif
    return out;
}

std::ostream &operator<<(std::ostream &out, LockingMode lockingMode)
{
    return out << toText(lockingMode);
}

std::ostream &operator<<(std::ostream &out, Operation operation)
{
    return out << toText(operation);
}

std::ostream &operator<<(std::ostream &out, TimeStamp timeStamp)
{
    return out << timeStamp.value;
}

namespace SessionChangeSetInternal {
namespace {

const char *toText(State state)
{
    switch (state) {
    case State::Invalid:
        return "Invalid";
    case State::Row:
        return "Row";
    case State::Done:
        return "Done";
    }

    return "";
}
} // namespace

std::ostream &operator<<(std::ostream &out, SentinelIterator)
{
    return out << "sentinel";
}

std::ostream &operator<<(std::ostream &out, State state)
{
    return out << toText(state);
}

std::ostream &operator<<(std::ostream &out, const Tuple &tuple)
{
    return out << "(" << tuple.operation << ", " << tuple.columnCount << ")";
}

std::ostream &operator<<(std::ostream &out, const ValueViews &valueViews)
{
    return out << "(" << valueViews.newValue << ", " << valueViews.oldValue << ")";
}

std::ostream &operator<<(std::ostream &out, const ConstIterator &iterator)
{
    return out << "(" << (*iterator) << ", " << iterator.state() << ")";
}

std::ostream &operator<<(std::ostream &out, const ConstTupleIterator &iterator)
{
    auto value = *iterator;

    return out << "(" << value.newValue << ", " << value.newValue << ")";
}

} // namespace SessionChangeSetInternal
} // namespace Sqlite

namespace QmlDesigner {
namespace {
const char *sourceTypeToText(SourceType sourceType)
{
    switch (sourceType) {
    case SourceType::Qml:
        return "Qml";
    case SourceType::QmlUi:
        return "QmlUi";
    case SourceType::QmlDir:
        return "QmlDir";
    case SourceType::QmlTypes:
        return "QmlTypes";
    case SourceType::Directory:
        return "Directory";
    }

    return "";
}

} // namespace

std::ostream &operator<<(std::ostream &out, const FileStatus &fileStatus)
{
    return out << "(" << fileStatus.sourceId << ", " << fileStatus.size << ", "
               << fileStatus.lastModified << ")";
}

std::ostream &operator<<(std::ostream &out, const Import &import)
{
    return out << "(" << import.url() << ", " << import.version() << ")";
}

std::ostream &operator<<(std::ostream &out, SourceType sourceType)
{
    return out << sourceTypeToText(sourceType);
}

std::ostream &operator<<(std::ostream &out, const ProjectChunkId &id)
{
    return out << "(" << id.id << ", " << id.sourceType << ")";
}

std::ostream &operator<<(std::ostream &out, const IdPaths &idPaths)
{
    return out << idPaths.id << ", " << idPaths.sourceIds << ")";
}

std::ostream &operator<<(std::ostream &out, const WatcherEntry &entry)
{
    return out << "(" << entry.sourceId << ", " << entry.sourceContextId << ", " << entry.id << ", "
               << entry.lastModified << ")";
}

std::ostream &operator<<(std::ostream &out, const ModelNode &node)
{
    if (!node.isValid())
        return out << "(invalid)";

    return out << "(" << node.id() << ")";
}

std::ostream &operator<<(std::ostream &out, const NodeMetaInfo &metaInfo)
{
    return out << "(" << metaInfo.id() << ")";
}

std::ostream &operator<<(std::ostream &out, const PropertyMetaInfo &metaInfo)
{
    return out << "(" << metaInfo.type() << ", " << metaInfo.name() << ", "
               << metaInfo.propertyType() << ")";
}

std::ostream &operator<<(std::ostream &out, const CompoundPropertyMetaInfo &metaInfo)
{
    return out << "(" << metaInfo.property << ", " << metaInfo.parent << ")";
}

std::ostream &operator<<(std::ostream &out, const VariantProperty &property)
{
    if (!property.isValid())
        return out << "(invalid)";

    return out << "(" << property.parentModelNode() << ", " << property.name() << ", "
               << property.value() << ")";
}

std::ostream &operator<<(std::ostream &out, const AbstractProperty &property)
{
    if (!property.isValid())
        return out << "(invalid)";

    return out << "(" << property.parentModelNode() << ", " << property.name() << ")";
}

std::ostream &operator<<(std::ostream &out, const ModelResourceSet::SetExpression &setExpression)
{
    return out << "(" << setExpression.property << ", " << setExpression.expression << ")";
}

std::ostream &operator<<(std::ostream &out, const ModelResourceSet &set)
{
    return out << "(" << set.removeModelNodes << ", " << set.removeProperties << ", "
               << set.setExpressions << ")";
}

namespace Cache {

std::ostream &operator<<(std::ostream &out, const SourceContext &sourceContext)
{
    return out << "(" << sourceContext.id << ", " << sourceContext.value << ")";
}
} // namespace Cache

namespace Storage {

namespace {
TypeTraits cleanFlags(TypeTraits traits)
{
    auto data = static_cast<int>(traits);
    data &= ~static_cast<int>(TypeTraits::IsEnum);
    return static_cast<TypeTraits>(data);
}

const char *typeTraitsToString(TypeTraits traits)
{
    switch (cleanFlags(traits)) {
    case TypeTraits::None:
        return "None";
    case TypeTraits::Reference:
        return "Reference";
    case TypeTraits::Sequence:
        return "Sequence";
    case TypeTraits::Value:
        return "Value";
    default:
        break;
    }

    return "";
}

const char *typeTraitsFlagsToString(TypeTraits traits)
{
    if (bool(traits & TypeTraits::IsEnum))
        return "(IsEnum)";

    return "";
}

} // namespace

std::ostream &operator<<(std::ostream &out, TypeTraits traits)
{
    return out << typeTraitsToString(traits) << typeTraitsFlagsToString(traits);
}

std::ostream &operator<<(std::ostream &out, PropertyDeclarationTraits traits)
{
    const char *padding = "";

    out << "(";
    if (traits & PropertyDeclarationTraits::IsReadOnly) {
        out << "readonly";
        padding = ", ";
    }

    if (traits & PropertyDeclarationTraits::IsPointer) {
        out << padding << "pointer";
        padding = ", ";
    }

    if (traits & PropertyDeclarationTraits::IsList)
        out << padding << "list";

    return out << ")";
}

std::ostream &operator<<(std::ostream &out, VersionNumber versionNumber)
{
    return out << versionNumber.value;
}

std::ostream &operator<<(std::ostream &out, Version version)
{
    return out << "(" << version.major << ", " << version.minor << ")";
}

std::ostream &operator<<(std::ostream &out, const Import &import)
{
    return out << "(" << import.moduleId << ", " << import.version << ", " << import.sourceId << ")";
}
} // namespace Storage

namespace Storage::Info {
std::ostream &operator<<(std::ostream &out, const PropertyDeclaration &propertyDeclaration)
{
    using Utils::operator<<;
    return out << "(\"" << propertyDeclaration.typeId << "\", " << propertyDeclaration.name << ", "
               << propertyDeclaration.typeId << ", " << propertyDeclaration.traits << ", "
               << propertyDeclaration.propertyTypeId << ")";
}

std::ostream &operator<<(std::ostream &out, const Type &type)
{
    return out << "(" << type.defaultPropertyId << ")";
}

std::ostream &operator<<(std::ostream &out, const ExportedTypeName &name)
{
    return out << "(\"" << name.name << "\"," << name.moduleId << ", " << name.version << ")";
}
} // namespace Storage::Info

namespace Storage::Synchronization {

namespace {

const char *isQualifiedToString(IsQualified isQualified)
{
    switch (isQualified) {
    case IsQualified::No:
        return "no";
    case IsQualified::Yes:
        return "yes";
    }

    return "";
}

const char *importKindToText(ImportKind kind)
{
    switch (kind) {
    case ImportKind::Import:
        return "Import";
    case ImportKind::ModuleDependency:
        return "ModuleDependency";
    case ImportKind::ModuleExportedImport:
        return "ModuleExportedImport";
    case ImportKind::ModuleExportedModuleDependency:
        return "ModuleExportedModuleDependency";
    }

    return "";
}

const char *isAutoVersionToText(IsAutoVersion isAutoVersion)
{
    switch (isAutoVersion) {
    case IsAutoVersion::No:
        return "is not autoversion";
    case IsAutoVersion::Yes:
        return "is auto version";
    }

    return "";
}

const char *fileTypeToText(FileType fileType)
{
    switch (fileType) {
    case FileType::QmlDocument:
        return "QmlDocument";
    case FileType::QmlTypes:
        return "QmlTypes";
    }

    return "";
}

const char *changeLevelToText(ChangeLevel changeLevel)
{
    switch (changeLevel) {
    case ChangeLevel::Full:
        return "Full";
    case ChangeLevel::Minimal:
        return "Minimal";
    case ChangeLevel::ExcludeExportedTypes:
        return "ExcludeExportedTypes";
    }

    return "";
}

} // namespace

std::ostream &operator<<(std::ostream &out, FileType fileType)
{
    return out << fileTypeToText(fileType);
}

std::ostream &operator<<(std::ostream &out, ChangeLevel changeLevel)
{
    return out << changeLevelToText(changeLevel);
}

std::ostream &operator<<(std::ostream &out, const SynchronizationPackage &package)
{
    return out << "(imports: " << package.imports << ", types: " << package.types
               << ", updatedSourceIds: " << package.updatedSourceIds
               << ", fileStatuses: " << package.fileStatuses
               << ", updatedFileStatusSourceIds: " << package.updatedFileStatusSourceIds
               << ", updatedProjectSourceIds: " << package.updatedProjectSourceIds
               << ", projectDatas: " << package.projectDatas
               << ", propertyEditorQmlPaths: " << package.propertyEditorQmlPaths
               << ", updatedPropertyEditorQmlPathSourceIds: "
               << package.updatedPropertyEditorQmlPathSourceIds << ")";
}

std::ostream &operator<<(std::ostream &out, const ProjectData &data)
{
    return out << "(" << data.projectSourceId << ", " << data.sourceId << ", " << data.moduleId
               << ", " << data.fileType << ")";
}

std::ostream &operator<<(std::ostream &out, IsQualified isQualified)
{
    return out << isQualifiedToString(isQualified);
}

std::ostream &operator<<(std::ostream &out, const ExportedType &exportedType)
{
    return out << "(\"" << exportedType.name << "\"," << exportedType.moduleId << ", "
               << exportedType.version << ")";
}

std::ostream &operator<<(std::ostream &out, const ImportedType &importedType)
{
    return out << "(\"" << importedType.name << "\")";
}
std::ostream &operator<<(std::ostream &out, const QualifiedImportedType &importedType)
{
    return out << "(\"" << importedType.name << "\", " << importedType.import << ")";
}

std::ostream &operator<<(std::ostream &out, const Type &type)
{
    using std::operator<<;
    using Utils::operator<<;
    return out << "( typename: \"" << type.typeName << "\", prototype: " << type.prototype << ", "
               << type.prototypeId << ", " << type.traits << ", source: " << type.sourceId
               << ", exports: " << type.exportedTypes << ", properties: " << type.propertyDeclarations
               << ", functions: " << type.functionDeclarations
               << ", signals: " << type.signalDeclarations << ", changeLevel: " << type.changeLevel
               << ", default: " << type.defaultPropertyName << ")";
}

std::ostream &operator<<(std::ostream &out, const PropertyDeclaration &propertyDeclaration)
{
    using Utils::operator<<;
    return out << "(\"" << propertyDeclaration.name << "\", " << propertyDeclaration.typeName
               << ", " << propertyDeclaration.typeId << ", " << propertyDeclaration.traits << ", "
               << propertyDeclaration.propertyTypeId << ", \""
               << propertyDeclaration.aliasPropertyName << "\")";
}

std::ostream &operator<<(std::ostream &out, const FunctionDeclaration &functionDeclaration)
{
    return out << "(\"" << functionDeclaration.name << "\", \"" << functionDeclaration.returnTypeName
               << "\", " << functionDeclaration.parameters << ")";
}

std::ostream &operator<<(std::ostream &out, const ParameterDeclaration &parameter)
{
    return out << "(\"" << parameter.name << "\", \"" << parameter.typeName << "\", "
               << parameter.traits << ")";
}

std::ostream &operator<<(std::ostream &out, const SignalDeclaration &signalDeclaration)
{
    return out << "(\"" << signalDeclaration.name << "\", " << signalDeclaration.parameters << ")";
}

std::ostream &operator<<(std::ostream &out, const EnumeratorDeclaration &enumeratorDeclaration)
{
    if (enumeratorDeclaration.hasValue) {
        return out << "(\"" << enumeratorDeclaration.name << "\", " << enumeratorDeclaration.value
                   << ")";
    } else {
        return out << "(\"" << enumeratorDeclaration.name << ")";
    }
}

std::ostream &operator<<(std::ostream &out, const EnumerationDeclaration &enumerationDeclaration)
{
    return out << "(\"" << enumerationDeclaration.name << "\", "
               << enumerationDeclaration.enumeratorDeclarations << ")";
}

std::ostream &operator<<(std::ostream &out, const ImportKind &importKind)
{
    return out << importKindToText(importKind);
}

std::ostream &operator<<(std::ostream &out, const IsAutoVersion &isAutoVersion)
{
    return out << isAutoVersionToText(isAutoVersion);
}

std::ostream &operator<<(std::ostream &out, const ModuleExportedImport &import)
{
    return out << "(" << import.moduleId << ", " << import.exportedModuleId << ", "
               << import.version << ", " << import.isAutoVersion << ")";
}

std::ostream &operator<<(std::ostream &out, const PropertyEditorQmlPath &path)
{
    return out << "(" << path.moduleId << ", " << path.typeName << ", " << path.pathId << ")";
}

} // namespace Storage::Synchronization

namespace ImageCache {

std::ostream &operator<<(std::ostream &out, const LibraryIconAuxiliaryData &data)
{
    return out << "(" << data.enable << ")";
}
std::ostream &operator<<(std::ostream &out, const FontCollectorSizeAuxiliaryData &data)
{
    return out << "(" << data.text << ", " << data.size << ", " << data.colorName << ")";
}
std::ostream &operator<<(std::ostream &out, const FontCollectorSizesAuxiliaryData &data)
{
    return out << "(" << data.text << ", " << data.colorName << ")";
}
} // namespace ImageCache

} // namespace QmlDesigner
