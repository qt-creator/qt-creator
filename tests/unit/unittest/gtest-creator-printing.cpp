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

#include "gtest-creator-printing.h"
#include "gtest-std-printing.h"

#include "gtest-qt-printing.h"

#include <gtest/gtest-printers.h>
#include <gmock/gmock-matchers.h>

#include <clangtools/clangtoolsdiagnostic.h>
#include <debugger/analyzer/diagnosticlocation.h>
#include <modelnode.h>
#include <projectstorage/filestatus.h>
#include <projectstorage/projectstoragepathwatchertypes.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorage/sourcepathcachetypes.h>
#include <sqlite.h>
#include <sqlitesessionchangeset.h>
#include <sqlitevalue.h>
#include <utils/fileutils.h>
#include <utils/linecolumn.h>
#include <variantproperty.h>
#include <qmldesigner/designercore/imagecache/imagecachestorageinterface.h>

namespace std {
template <typename T> ostream &operator<<(ostream &out, const QVector<T> &vector)
{
    out << "[";
    copy(vector.cbegin(), vector.cend(), ostream_iterator<T>(out, ", "));
    out << "]";
    return out;
}
} // namespace std

namespace Utils {

std::ostream &operator<<(std::ostream &out, const LineColumn &lineColumn)
{
    return out << "(" << lineColumn.line << ", " << lineColumn.column << ")";
}
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

std::ostream &operator<<(std::ostream &out, sqlite3_changeset_iter *iter)
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

std::ostream &operator<<(std::ostream &out, const SessionChangeSet &changeset)
{
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

namespace Debugger {
std::ostream &operator<<(std::ostream &out, const DiagnosticLocation &loc)
{
    return out << "(" << loc.filePath << ", " << loc.line << ", " << loc.column << ")";
}
} // namespace Debugger

namespace ClangTools {
namespace Internal {
std::ostream &operator<<(std::ostream &out, const ExplainingStep &step)
{
    return out << "("
               << step.message << ", "
               << step.location << ", "
               << step.ranges << ", "
               << step.isFixIt
               << ")";
}

std::ostream &operator<<(std::ostream &out, const Diagnostic &diag) {
    return out << "("
               << diag.name << ", "
               << diag.description << ", "
               << diag.category << ", "
               << diag.type << ", "
               << diag.location << ", "
               << diag.explainingSteps << ", "
               << diag.hasFixits
               << ")";
}
} // namespace Internal
} // namespace ClangTools

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
    }

    return "";
}
} // namespace

std::ostream &operator<<(std::ostream &out, const FileStatus &fileStatus)
{
    return out << "(" << fileStatus.sourceId << ", " << fileStatus.size << ", "
               << fileStatus.lastModified << ")";
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
std::ostream &operator<<(std::ostream &out, const VariantProperty &property)
{
    if (!property.isValid())
        return out << "(invalid)";

    return out << "(" << property.parentModelNode() << ", " << property.name() << ", "
               << property.value() << ")";
}
namespace Cache {

std::ostream &operator<<(std::ostream &out, const SourceContext &sourceContext)
{
    return out << "(" << sourceContext.id << ", " << sourceContext.value << ")";
}
} // namespace Cache

namespace Storage {

namespace {

TypeAccessSemantics cleanFlags(TypeAccessSemantics accessSemantics)
{
    auto data = static_cast<int>(accessSemantics);
    data &= ~static_cast<int>(TypeAccessSemantics::IsEnum);
    return static_cast<TypeAccessSemantics>(data);
}

const char *typeAccessSemanticsToString(TypeAccessSemantics accessSemantics)
{
    switch (cleanFlags(accessSemantics)) {
    case TypeAccessSemantics::None:
        return "None";
    case TypeAccessSemantics::Reference:
        return "Reference";
    case TypeAccessSemantics::Sequence:
        return "Sequence";
    case TypeAccessSemantics::Value:
        return "Value";
    default:
        break;
    }

    return "";
}

bool operator&(TypeAccessSemantics first, TypeAccessSemantics second)
{
    return static_cast<int>(first) & static_cast<int>(second);
}

const char *typeAccessSemanticsFlagsToString(TypeAccessSemantics accessSemantics)
{
    if (accessSemantics & TypeAccessSemantics::IsEnum)
        return "(IsEnum)";

    return "";
}

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
    case ImportKind::Module:
        return "Module";
    case ImportKind::Directory:
        return "Directory";
    case ImportKind::QmlTypesDependency:
        return "QmlTypesDependency";
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
    return out << "(" << package.imports << ", " << package.types << ", "
               << package.updatedSourceIds << ", " << package.fileStatuses << ", "
               << package.updatedFileStatusSourceIds << ", " << package.projectDatas << ")";
}

std::ostream &operator<<(std::ostream &out, const ProjectData &data)
{
    return out << "(" << data.projectSourceId << ", " << data.sourceId << ", " << data.moduleId
               << ", " << data.fileType << ")";
}

std::ostream &operator<<(std::ostream &out, TypeAccessSemantics accessSemantics)
{
    return out << typeAccessSemanticsToString(accessSemantics)
               << typeAccessSemanticsFlagsToString(accessSemantics);
}

std::ostream &operator<<(std::ostream &out, IsQualified isQualified)
{
    return out << isQualifiedToString(isQualified);
}

std::ostream &operator<<(std::ostream &out, VersionNumber versionNumber)
{
    return out << versionNumber.value;
}

std::ostream &operator<<(std::ostream &out, Version version)
{
    return out << "(" << version.major << ", " << version.minor << ")";
}

std::ostream &operator<<(std::ostream &out, const ExportedType &exportedType)
{
    return out << "(\"" << exportedType.name << "\"," << exportedType.moduleId << ", "
               << exportedType.version << ")";
}

std::ostream &operator<<(std::ostream &out, const NativeType &nativeType)
{
    return out << "(\"" << nativeType.name << "\")";
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
    using Utils::operator<<;
    return out << "( typename: \"" << type.typeName << "\", prototype: " << type.prototype << ", "
               << type.prototypeId << ", " << type.accessSemantics << ", source: " << type.sourceId
               << ", exports: " << type.exportedTypes << ", properties: " << type.propertyDeclarations
               << ", functions: " << type.functionDeclarations
               << ", signals: " << type.signalDeclarations << ", changeLevel: " << type.changeLevel
               << ")";
}

std::ostream &operator<<(std::ostream &out, const PropertyDeclaration &propertyDeclaration)
{
    using Utils::operator<<;
    return out << "(\"" << propertyDeclaration.name << "\", " << propertyDeclaration.typeName
               << ", " << propertyDeclaration.typeId << ", " << propertyDeclaration.traits << ", "
               << propertyDeclaration.typeId << ", \"" << propertyDeclaration.aliasPropertyName
               << "\")";
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

std::ostream &operator<<(std::ostream &out, const Import &import)
{
    return out << "(" << import.moduleId << ", " << import.version << ", " << import.sourceId << ")";
}

} // namespace Storage

} // namespace QmlDesigner
