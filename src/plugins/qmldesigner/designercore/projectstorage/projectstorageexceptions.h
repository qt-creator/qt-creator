// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../include/qmldesignercorelib_global.h"

#include <exception>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT ProjectStorageError : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT NoSourcePathForInvalidSourceId : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT NoSourceContextPathForInvalidSourceContextId : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT SourceContextIdDoesNotExists : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT SourceIdDoesNotExists : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT TypeHasInvalidSourceId : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleDoesNotExists : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleAlreadyExists : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ExportedTypeCannotBeInserted : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT TypeNameDoesNotExists : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT PropertyNameDoesNotExists : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT PrototypeChainCycle : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT AliasChainCycle : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlTypesFile : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlDocumentFile : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidProjectSourceId : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidSourceId : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidModuleId : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT FileStatusHasInvalidSourceId : ProjectStorageError
{
public:
    const char *what() const noexcept override;
};

} // namespace QmlDesigner
