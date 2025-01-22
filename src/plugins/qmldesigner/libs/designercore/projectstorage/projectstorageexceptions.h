// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../include/qmldesignercorelib_global.h"

#include "projectstorageids.h"

#include <sqliteexception.h>

namespace QmlDesigner {

using namespace std::literals::string_view_literals;

class QMLDESIGNERCORE_EXPORT ProjectStorageError : public Sqlite::Exception
{
protected:
    using Sqlite::Exception::Exception;

public:
    const char *what() const noexcept override;
};

class ProjectStorageErrorWithMessage : public ProjectStorageError
{
protected:
    ProjectStorageErrorWithMessage(
        std::string_view error,
        std::string_view errorMessage,
        const Sqlite::source_location &location = Sqlite::source_location::current());

public:
    const char *what() const noexcept override;

public:
    std::string errorMessage;
};

class QMLDESIGNERCORE_EXPORT TypeHasInvalidSourceId : public ProjectStorageError
{
public:
    TypeHasInvalidSourceId(const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleDoesNotExists : public ProjectStorageError
{
public:
    ModuleDoesNotExists(const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleAlreadyExists : public ProjectStorageError
{
public:
    ModuleAlreadyExists(const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ExportedTypeCannotBeInserted : public ProjectStorageErrorWithMessage
{
public:
    ExportedTypeCannotBeInserted(
        std::string_view errorMessage,
        const Sqlite::source_location &location = Sqlite::source_location::current());
};

class QMLDESIGNERCORE_EXPORT TypeNameDoesNotExists : public ProjectStorageErrorWithMessage
{
public:
    TypeNameDoesNotExists(std::string_view typeName,
                          SourceId sourceId = SourceId{},
                          const Sqlite::source_location &location = Sqlite::source_location::current());
};

class QMLDESIGNERCORE_EXPORT PrototypeChainCycle : public ProjectStorageError
{
public:
    PrototypeChainCycle(const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT AliasChainCycle : public ProjectStorageError
{
public:
    AliasChainCycle(const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlTypesFile : public ProjectStorageError
{
public:
    CannotParseQmlTypesFile(const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlDocumentFile : public ProjectStorageError
{
public:
    CannotParseQmlDocumentFile(
        const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT DirectoryInfoHasInvalidProjectSourceId : public ProjectStorageError
{
public:
    DirectoryInfoHasInvalidProjectSourceId(
        const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT DirectoryInfoHasInvalidSourceId : public ProjectStorageError
{
public:
    DirectoryInfoHasInvalidSourceId(
        const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT DirectoryInfoHasInvalidModuleId : public ProjectStorageError
{
public:
    DirectoryInfoHasInvalidModuleId(
        const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT FileStatusHasInvalidSourceId : public ProjectStorageError
{
public:
    FileStatusHasInvalidSourceId(
        const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT TypeAnnotationHasInvalidSourceId : public ProjectStorageError
{
public:
    TypeAnnotationHasInvalidSourceId(
        const Sqlite::source_location &location = Sqlite::source_location::current());
    const char *what() const noexcept override;
};

} // namespace QmlDesigner
