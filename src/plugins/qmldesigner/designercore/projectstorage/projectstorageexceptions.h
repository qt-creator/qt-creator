// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../include/qmldesignercorelib_global.h"

#include <exception>

namespace QmlDesigner {

using namespace std::literals::string_view_literals;

class QMLDESIGNERCORE_EXPORT ProjectStorageError : public std::exception
{
public:
    const char *what() const noexcept override;
};

class ProjectStorageErrorWithMessage : public ProjectStorageError
{
public:
    ProjectStorageErrorWithMessage(std::string_view error, std::string_view errorMessage);

    const char *what() const noexcept override;

public:
    std::string errorMessage;
};

class QMLDESIGNERCORE_EXPORT NoSourcePathForInvalidSourceId : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT NoSourceContextPathForInvalidSourceContextId : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT SourceContextIdDoesNotExists : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT SourceIdDoesNotExists : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT TypeHasInvalidSourceId : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleDoesNotExists : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleAlreadyExists : public ProjectStorageError
{
public:
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
    TypeNameDoesNotExists(std::string_view errorMessage);
};

class QMLDESIGNERCORE_EXPORT PropertyNameDoesNotExists : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT PrototypeChainCycle : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT AliasChainCycle : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlTypesFile : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlDocumentFile : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidProjectSourceId : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidSourceId : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidModuleId : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT FileStatusHasInvalidSourceId : public ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

} // namespace QmlDesigner
