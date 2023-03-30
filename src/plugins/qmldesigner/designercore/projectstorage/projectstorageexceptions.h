// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../include/qmldesignercorelib_global.h"

#include <exception>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT NoSourcePathForInvalidSourceId : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT NoSourceContextPathForInvalidSourceContextId : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT SourceContextIdDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT SourceIdDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT TypeHasInvalidSourceId : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ModuleAlreadyExists : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ExportedTypeCannotBeInserted : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT TypeNameDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT PropertyNameDoesNotExists : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT PrototypeChainCycle : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT AliasChainCycle : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlTypesFile : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT CannotParseQmlDocumentFile : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidProjectSourceId : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidSourceId : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT ProjectDataHasInvalidModuleId : std::exception
{
public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT FileStatusHasInvalidSourceId : std::exception
{
public:
    const char *what() const noexcept override;
};

} // namespace QmlDesigner
