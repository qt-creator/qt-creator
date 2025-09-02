// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <3rdparty/toml11/toml.hpp>
#include <projectexplorer/buildsystem.h>

namespace Python::Internal {

enum class PyProjectTomlErrorType { ParsingError, MissingNode, TypeError, EmptyNode, FileNotFound };

/*
    \brief Class representing a generic error found in a pyproject.toml file
    Not intended to be constructed directly, but rather calling one of the static factory methods
    to create an instance of this class with the appropriate \a description format.
    \sa PyProjectTomlParseResult
*/
struct PyProjectTomlError
{
    PyProjectTomlErrorType type;
    QString description;
    int line;

    PyProjectTomlError(PyProjectTomlErrorType type, const QString &description, int line = -1)
        : type(type)
        , description(description)
        , line(line)
    {}

    bool operator==(const PyProjectTomlError &other) const
    {
        return type == other.type && description == other.description && line == other.line;
    }

    static PyProjectTomlError ParseError(const std::string &description, int line = -1);
    static PyProjectTomlError TypeError(
        const std::string &nodeName,
        const std::string &expectedTypeName,
        const std::string &actualType,
        int line = -1);
    static PyProjectTomlError MissingNodeError(
        const std::string &nodeName, const std::string &key, int line = -1);
    static PyProjectTomlError EmptyNodeError(const std::string &nodeName, int line = -1);
    static PyProjectTomlError FileNotFoundError(const std::string &filePath, int line = -1);
};

struct PyProjectTomlParseResult
{
    QList<PyProjectTomlError> errors;
    QString projectName;
    QStringList projectFiles;
};

template<typename ExpectedType>
Utils::expected<ExpectedType, PyProjectTomlError> getNodeByKey(
    const std::string expectedTypeName,
    const std::string tableName,
    const toml::ordered_value &table,
    const std::string key);

template<typename ExpectedType>
Utils::expected<ExpectedType, PyProjectTomlError> getNodeValue(
    const std::string expectedTypeName, const std::string nodeName, const toml::ordered_value &node);

PyProjectTomlParseResult parsePyProjectToml(const Utils::FilePath &pyProjectTomlPath);

Utils::Result<QString> updatePyProjectTomlContent(
    const QString &pyProjectTomlContent, const QStringList &projectFiles);

} // namespace Python::Internal
