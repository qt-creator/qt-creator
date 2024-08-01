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

NoSourcePathForInvalidSourceId::NoSourcePathForInvalidSourceId()
{
    category().threadEvent("NoSourcePathForInvalidSourceId"_t);
}

const char *NoSourcePathForInvalidSourceId::what() const noexcept
{
    return "You cannot get a file path for an invalid file path id!";
}

NoSourceContextPathForInvalidSourceContextId::NoSourceContextPathForInvalidSourceContextId()
{
    category().threadEvent("NoSourceContextPathForInvalidSourceContextId"_t);
}

const char *NoSourceContextPathForInvalidSourceContextId::what() const noexcept
{
    return "You cannot get a directory path for an invalid directory path id!";
}

SourceContextIdDoesNotExists::SourceContextIdDoesNotExists()
{
    category().threadEvent("SourceContextIdDoesNotExists"_t);
}

const char *SourceContextIdDoesNotExists::what() const noexcept
{
    return "The source context id does not exist in the database!";
}

SourceNameIdDoesNotExists::SourceNameIdDoesNotExists()
{
    category().threadEvent("SourceNameIdDoesNotExists"_t);
}

const char *SourceNameIdDoesNotExists::what() const noexcept
{
    return "The source id does not exist in the database!";
}

TypeHasInvalidSourceId::TypeHasInvalidSourceId()
{
    category().threadEvent("TypeHasInvalidSourceId"_t);
}

const char *TypeHasInvalidSourceId::what() const noexcept
{
    return "The source id is invalid!";
}

ModuleDoesNotExists::ModuleDoesNotExists()
{
    category().threadEvent("ModuleDoesNotExists"_t);
}

const char *ModuleDoesNotExists::what() const noexcept
{
    return "The module does not exist!";
}

ModuleAlreadyExists::ModuleAlreadyExists()
{
    category().threadEvent("ModuleAlreadyExists"_t);
}

const char *ModuleAlreadyExists::what() const noexcept
{
    return "The module does already exist!";
}

TypeNameDoesNotExists::TypeNameDoesNotExists(std::string_view typeName, SourceId sourceId)
    : ProjectStorageErrorWithMessage{
        "TypeNameDoesNotExists"sv,
        Utils::SmallString::join(
            {"type: ", typeName, ", source id: ", Utils::SmallString::number(sourceId.internalId())})}
{
    category().threadEvent("TypeNameDoesNotExists"_t,
                           keyValue("type name", typeName),
                           keyValue("source id", sourceId));
}

PrototypeChainCycle::PrototypeChainCycle()
{
    category().threadEvent("PrototypeChainCycle"_t);
}

const char *PrototypeChainCycle::what() const noexcept
{
    return "There is a prototype chain cycle!";
}

AliasChainCycle::AliasChainCycle()
{
    category().threadEvent("AliasChainCycle"_t);
}

const char *AliasChainCycle::what() const noexcept
{
    return "There is a prototype chain cycle!";
}

CannotParseQmlTypesFile::CannotParseQmlTypesFile()
{
    category().threadEvent("CannotParseQmlTypesFile"_t);
}

const char *CannotParseQmlTypesFile::what() const noexcept
{
    return "Cannot parse qml types file!";
}

CannotParseQmlDocumentFile::CannotParseQmlDocumentFile()
{
    category().threadEvent("CannotParseQmlDocumentFile"_t);
}

const char *CannotParseQmlDocumentFile::what() const noexcept
{
    return "Cannot parse qml types file!";
}

DirectoryInfoHasInvalidProjectSourceId::DirectoryInfoHasInvalidProjectSourceId()
{
    category().threadEvent("DirectoryInfoHasInvalidProjectSourceId"_t);
}

const char *DirectoryInfoHasInvalidProjectSourceId::what() const noexcept
{
    return "The project source id is invalid!";
}

DirectoryInfoHasInvalidSourceId::DirectoryInfoHasInvalidSourceId()
{
    category().threadEvent("DirectoryInfoHasInvalidSourceId"_t);
}

const char *DirectoryInfoHasInvalidSourceId::what() const noexcept
{
    return "The source id is invalid!";
}

DirectoryInfoHasInvalidModuleId::DirectoryInfoHasInvalidModuleId()
{
    category().threadEvent("DirectoryInfoHasInvalidModuleId"_t);
}

const char *DirectoryInfoHasInvalidModuleId::what() const noexcept
{
    return "The module id is invalid!";
}

FileStatusHasInvalidSourceId::FileStatusHasInvalidSourceId()
{
    category().threadEvent("FileStatusHasInvalidSourceId"_t);
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
                                                               std::string_view message)
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

ExportedTypeCannotBeInserted::ExportedTypeCannotBeInserted(std::string_view errorMessage)
    : ProjectStorageErrorWithMessage{"ExportedTypeCannotBeInserted"sv, errorMessage}
{
    category().threadEvent("ExportedTypeCannotBeInserted"_t, keyValue("error message", errorMessage));
}

TypeAnnotationHasInvalidSourceId::TypeAnnotationHasInvalidSourceId()
{
    category().threadEvent("TypeAnnotationHasInvalidSourceId"_t);
}

const char *TypeAnnotationHasInvalidSourceId::what() const noexcept
{
    return "The source id in a type annotation is invalid!";
}

} // namespace QmlDesigner
