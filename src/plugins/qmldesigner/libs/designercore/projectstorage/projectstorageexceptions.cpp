// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "projectstorageexceptions.h"

#include <tracing/qmldesignertracing.h>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;
using NanotraceHR::keyValue;

namespace {
auto &category()
{
    return ProjectStorageTracing::projectStorageCategory();
}
} // namespace

TypeHasInvalidSourceId::TypeHasInvalidSourceId(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("TypeHasInvalidSourceId");
}

const char *TypeHasInvalidSourceId::what() const noexcept
{
    return "The source id is invalid!";
}

ModuleDoesNotExists::ModuleDoesNotExists(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("ModuleDoesNotExists");
}

const char *ModuleDoesNotExists::what() const noexcept
{
    return "The module does not exist!";
}

ModuleAlreadyExists::ModuleAlreadyExists(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("ModuleAlreadyExists");
}

const char *ModuleAlreadyExists::what() const noexcept
{
    return "The module does already exist!";
}

TypeNameDoesNotExists::TypeNameDoesNotExists(std::string_view typeName,
                                             SourceId sourceId,
                                             const Sqlite::source_location &location)
    : ProjectStorageErrorWithMessage{
          "TypeNameDoesNotExists"sv,
          Utils::SmallString::join(
              {"type: ", typeName, ", source id: ", Utils::SmallString::number(sourceId.internalId())}),
          location}
{
    category().threadEvent("TypeNameDoesNotExists",
                           keyValue("type name", typeName),
                           keyValue("source id", sourceId));
}

PrototypeChainCycle::PrototypeChainCycle(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("PrototypeChainCycle");
}

const char *PrototypeChainCycle::what() const noexcept
{
    return "There is a prototype chain cycle!";
}

AliasChainCycle::AliasChainCycle(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("AliasChainCycle");
}

const char *AliasChainCycle::what() const noexcept
{
    return "There is a prototype chain cycle!";
}

CannotParseQmlTypesFile::CannotParseQmlTypesFile(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("CannotParseQmlTypesFile");
}

const char *CannotParseQmlTypesFile::what() const noexcept
{
    return "Cannot parse qml types file!";
}

CannotParseQmlDocumentFile::CannotParseQmlDocumentFile(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("CannotParseQmlDocumentFile");
}

const char *CannotParseQmlDocumentFile::what() const noexcept
{
    return "Cannot parse qml document file!";
}

DirectoryInfoHasInvalidProjectSourceId::DirectoryInfoHasInvalidProjectSourceId(
    const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("DirectoryInfoHasInvalidProjectSourceId");
}

const char *DirectoryInfoHasInvalidProjectSourceId::what() const noexcept
{
    return "The project source id is invalid!";
}

DirectoryInfoHasInvalidSourceId::DirectoryInfoHasInvalidSourceId(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("DirectoryInfoHasInvalidSourceId");
}

const char *DirectoryInfoHasInvalidSourceId::what() const noexcept
{
    return "The source id is invalid!";
}

DirectoryInfoHasInvalidModuleId::DirectoryInfoHasInvalidModuleId(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("DirectoryInfoHasInvalidModuleId");
}

const char *DirectoryInfoHasInvalidModuleId::what() const noexcept
{
    return "The module id is invalid!";
}

FileStatusHasInvalidSourceId::FileStatusHasInvalidSourceId(const Sqlite::source_location &location)
    : ProjectStorageError{location}

{
    category().threadEvent("FileStatusHasInvalidSourceId");
}

const char *FileStatusHasInvalidSourceId::what() const noexcept
{
    return "The source id in file status is invalid!";
}

const char *ProjectStorageError::what() const noexcept
{
    return "Project storage error!";
}

ProjectStorageErrorWithMessage::ProjectStorageErrorWithMessage(std::string_view error,
                                                               std::string_view message,
                                                               const Sqlite::source_location &location)
    : ProjectStorageError{location}
{
    errorMessage += error;
    errorMessage += "{"sv;
    errorMessage += message;
    errorMessage += "}"sv;
}

const char *ProjectStorageErrorWithMessage::what() const noexcept
{
    return errorMessage.c_str();
}

ExportedTypeCannotBeInserted::ExportedTypeCannotBeInserted(std::string_view errorMessage,
                                                           const Sqlite::source_location &location)
    : ProjectStorageErrorWithMessage{"ExportedTypeCannotBeInserted"sv, errorMessage, location}
{
    category().threadEvent("ExportedTypeCannotBeInserted", keyValue("error message", errorMessage));
}

TypeAnnotationHasInvalidSourceId::TypeAnnotationHasInvalidSourceId(const Sqlite::source_location &location)
    : ProjectStorageError{location}
{
    category().threadEvent("TypeAnnotationHasInvalidSourceId");
}

const char *TypeAnnotationHasInvalidSourceId::what() const noexcept
{
    return "The source id in a type annotation is invalid!";
}

} // namespace QmlDesigner
