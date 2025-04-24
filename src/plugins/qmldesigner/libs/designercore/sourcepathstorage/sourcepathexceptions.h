// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../include/qmldesignercorelib_global.h"

#include <exception>

namespace QmlDesigner {

using namespace std::literals::string_view_literals;

class QMLDESIGNERCORE_EXPORT SourcePathError : public std::exception
{
protected:
    SourcePathError() = default;

public:
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT NoSourcePathForInvalidSourceId : public SourcePathError
{
public:
    NoSourcePathForInvalidSourceId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT NoFileNameForInvalidFileNameId : public SourcePathError
{
public:
    NoFileNameForInvalidFileNameId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT NoDirectoryPathForInvalidDirectoryPathId : public SourcePathError
{
public:
    NoDirectoryPathForInvalidDirectoryPathId();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT DirectoryPathIdDoesNotExists : public SourcePathError
{
public:
    DirectoryPathIdDoesNotExists();
    const char *what() const noexcept override;
};

class QMLDESIGNERCORE_EXPORT FileNameIdDoesNotExists : public SourcePathError
{
public:
    FileNameIdDoesNotExists();
    const char *what() const noexcept override;
};

} // namespace QmlDesigner
