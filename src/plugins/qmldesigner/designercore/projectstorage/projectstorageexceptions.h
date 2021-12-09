/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

} // namespace QmlDesigner
