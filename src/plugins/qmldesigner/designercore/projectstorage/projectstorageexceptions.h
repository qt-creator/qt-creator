// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <exception>

namespace QmlDesigner {

class NoSourcePathForInvalidSourceId : std::exception
{
public:
    const char *what() const noexcept override
    {
        return "You cannot get a file path for an invalid file path id!";
    }
};

class NoSourceContextPathForInvalidSourceContextId : std::exception
{
public:
    const char *what() const noexcept override
    {
        return "You cannot get a directory path for an invalid directory path id!";
    }
};

class SourceContextIdDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override
    {
        return "The source context id does not exist in the database!";
    }
};

class SourceIdDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override
    {
        return "The source id does not exist in the database!";
    }
};

class TypeHasInvalidSourceId : std::exception
{
public:
    const char *what() const noexcept override { return "The source id is invalid!"; }
};

class ModuleDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override { return "The module does not exist!"; }
};

class ModuleAlreadyExists : std::exception
{
public:
    const char *what() const noexcept override { return "The module does already exist!"; }
};

class ExportedTypeCannotBeInserted : std::exception
{
public:
    const char *what() const noexcept override { return "The exported type cannot be inserted!"; }
};

class TypeNameDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override { return "The type name does not exist!"; }
};

class PropertyNameDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override { return "The property name does not exist!"; }
};

class PrototypeChainCycle : std::exception
{
public:
    const char *what() const noexcept override { return "There is a prototype chain cycle!"; }
};

class AliasChainCycle : std::exception
{
public:
    const char *what() const noexcept override { return "There is a prototype chain cycle!"; }
};

class CannotParseQmlTypesFile : std::exception
{
public:
    const char *what() const noexcept override { return "Cannot parse qml types file!"; }
};

class CannotParseQmlDocumentFile : std::exception
{
public:
    const char *what() const noexcept override { return "Cannot parse qml types file!"; }
};

class ProjectDataHasInvalidProjectSourceId : std::exception
{
public:
    const char *what() const noexcept override { return "The project source id is invalid!"; }
};

class ProjectDataHasInvalidSourceId : std::exception
{
public:
    const char *what() const noexcept override { return "The source id is invalid!"; }
};

class ProjectDataHasInvalidModuleId : std::exception
{
public:
    const char *what() const noexcept override { return "The module id is invalid!"; }
};

class FileStatusHasInvalidSourceId : std::exception
{
public:
    const char *what() const noexcept override
    {
        return "The source id in file status is invalid!";
    }
};

} // namespace QmlDesigner
