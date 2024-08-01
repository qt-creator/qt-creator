// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../include/qmldesignercorelib_global.h"

#include "projectstorageids.h"

#include <exception>

namespace QmlDesigner {

using namespace std::literals::string_view_literals;

class QMLDESIGNERCORE_EXPORT ProjectStorageError : public std::exception
{
protected:
    ProjectStorageError() = default;

public:
    const char *what() const noexcept override;
};

class ProjectStorageErrorWithMessage : public ProjectStorageError
{
protected:
    ProjectStorageErrorWithMessage(std::string_view error, std::string_view errorMessage);

public:
    const char *what() const noexcept override;

public:
    std::string errorMessage;
};

class QMLDESIGNERCORE_EXPORT NoSourcePathForInvalidSourceId : public ProjectStorageError
{
public:
    NoSourcePathForInvalidSourceId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT NoSourceContextPathForInvalidSourceContextId : public ProjectStorageError
{
public:
    NoSourceContextPathForInvalidSourceContextId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT SourceContextIdDoesNotExists : public ProjectStorageError
{
public:
    SourceContextIdDoesNotExists();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT SourceNameIdDoesNotExists : public ProjectStorageError
{
public:
    SourceNameIdDoesNotExists();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT TypeHasInvalidSourceId : public ProjectStorageError
{
public:
    TypeHasInvalidSourceId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleDoesNotExists : public ProjectStorageError
{
public:
    ModuleDoesNotExists();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleAlreadyExists : public ProjectStorageError
{
public:
    ModuleAlreadyExists();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ExportedTypeCannotBeInserted : public ProjectStorageErrorWithMessage
{
public:
    ExportedTypeCannotBeInserted(std::string_view errorMessage);
};

class QMLDESIGNERCORE_EXPORT TypeNameDoesNotExists : public ProjectStorageErrorWithMessage
{
public:
    TypeNameDoesNotExists(std::string_view typeName, SourceId sourceId = SourceId{});
};

class QMLDESIGNERCORE_EXPORT PrototypeChainCycle : public ProjectStorageError
{
public:
    PrototypeChainCycle();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT AliasChainCycle : public ProjectStorageError
{
public:
    AliasChainCycle();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlTypesFile : public ProjectStorageError
{
public:
    CannotParseQmlTypesFile();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlDocumentFile : public ProjectStorageError
{
public:
    CannotParseQmlDocumentFile();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT DirectoryInfoHasInvalidProjectSourceId : public ProjectStorageError
{
public:
    DirectoryInfoHasInvalidProjectSourceId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT DirectoryInfoHasInvalidSourceId : public ProjectStorageError
{
public:
    DirectoryInfoHasInvalidSourceId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT DirectoryInfoHasInvalidModuleId : public ProjectStorageError
{
public:
    DirectoryInfoHasInvalidModuleId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT FileStatusHasInvalidSourceId : public ProjectStorageError
{
public:
    FileStatusHasInvalidSourceId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT TypeAnnotationHasInvalidSourceId : public ProjectStorageError
{
public:
    TypeAnnotationHasInvalidSourceId();
    const char *what() const noexcept override;
};

} // namespace QmlDesigner
