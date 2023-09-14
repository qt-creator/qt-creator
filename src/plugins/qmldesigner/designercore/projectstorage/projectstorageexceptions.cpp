// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "projectstorageexceptions.h"

namespace QmlDesigner {

const char *NoSourcePathForInvalidSourceId::what() const noexcept
{
    return "You cannot get a file path for an invalid file path id!";
}

const char *NoSourceContextPathForInvalidSourceContextId::what() const noexcept
{
    return "You cannot get a directory path for an invalid directory path id!";
}

const char *SourceContextIdDoesNotExists::what() const noexcept
{
    return "The source context id does not exist in the database!";
}

const char *SourceIdDoesNotExists::what() const noexcept
{
    return "The source id does not exist in the database!";
}

const char *TypeHasInvalidSourceId::what() const noexcept
{
    return "The source id is invalid!";
}

const char *ModuleDoesNotExists::what() const noexcept
{
    return "The module does not exist!";
}

const char *ModuleAlreadyExists::what() const noexcept
{
    return "The module does already exist!";
}

TypeNameDoesNotExists::TypeNameDoesNotExists(std::string_view errorMessage)
    : ProjectStorageErrorWithMessage{"TypeNameDoesNotExists"sv, errorMessage}
{}

const char *PropertyNameDoesNotExists::what() const noexcept
{
    return "The property name does not exist!";
}

const char *PrototypeChainCycle::what() const noexcept
{
    return "There is a prototype chain cycle!";
}

const char *AliasChainCycle::what() const noexcept
{
    return "There is a prototype chain cycle!";
}

const char *CannotParseQmlTypesFile::what() const noexcept
{
    return "Cannot parse qml types file!";
}

const char *CannotParseQmlDocumentFile::what() const noexcept
{
    return "Cannot parse qml types file!";
}

const char *ProjectDataHasInvalidProjectSourceId::what() const noexcept
{
    return "The project source id is invalid!";
}

const char *ProjectDataHasInvalidSourceId::what() const noexcept
{
    return "The source id is invalid!";
}

const char *ProjectDataHasInvalidModuleId::what() const noexcept
{
    return "The module id is invalid!";
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
{}

} // namespace QmlDesigner
