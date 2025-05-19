// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pyprojecttoml.h"
#include "pythontr.h"

#include <3rdparty/toml11/toml.hpp>

#include <QString>
#include <QStringList>

using namespace Utils;

namespace Python::Internal {

PyProjectTomlError PyProjectTomlError::ParseError(const std::string &description, int line)
{
    return PyProjectTomlError(
        PyProjectTomlErrorType::ParsingError,
        Tr::tr("Parsing error: %1").arg(QString::fromUtf8(description)),
        line);
}

PyProjectTomlError PyProjectTomlError::TypeError(
    const std::string &nodeName,
    const std::string &expectedType,
    const std::string &actualType,
    int line)
{
    return PyProjectTomlError(
        PyProjectTomlErrorType::TypeError,
        Tr::tr("Type error: \"%1\" must be a \"%2\", not a \"%3\".")
            .arg(QString::fromUtf8(nodeName))
            .arg(QString::fromUtf8(expectedType))
            .arg(QString::fromUtf8(actualType)),
        line);
}

PyProjectTomlError PyProjectTomlError::MissingNodeError(
    const std::string &nodeName, const std::string &key, int line)
{
    return PyProjectTomlError(
        PyProjectTomlErrorType::MissingNode,
        Tr::tr("Missing node error: \"%1\" table must contain a \"%2\" node.")
            .arg(QString::fromUtf8(nodeName))
            .arg(QString::fromUtf8(key)),
        line);
}

PyProjectTomlError PyProjectTomlError::EmptyNodeError(const std::string &nodeName, int line)
{
    return PyProjectTomlError(
        PyProjectTomlErrorType::EmptyNode,
        Tr::tr("Node \"%1\" is empty.").arg(QString::fromUtf8(nodeName)),
        line);
}

PyProjectTomlError PyProjectTomlError::FileNotFoundError(const std::string &filePath, int line)
{
    return PyProjectTomlError(
        PyProjectTomlErrorType::FileNotFound,
        Tr::tr("File \"%1\" does not exist.").arg(QString::fromUtf8(filePath)),
        line);
}

/*!
    \brief Returns the value associated with a given key from a TOML table node, if it exists.
    The value can be a TOML table, array or primitive value.
    \tparam ExpectedType The type of the value to fetch. Must be a TOML value type.
    \param expectedTypeName The name of the expected type
    \param tableName The name of the table to fetch the value from
    \param table The table to fetch the value from
    \param key The key to fetch the value from. May be a dotted key.
    \return The value if found, otherwise an error string
    \note The \a expectedTypeName and \a tableName parameters are only used for error reporting
*/
template<typename ExpectedType>
expected<ExpectedType, PyProjectTomlError> getNodeByKey(
    const std::string expectedTypeName,
    const std::string tableName,
    const toml::ordered_value &table,
    const std::string key)
{
    // Edge case: Do not show the the root table errors in a specific line
    int nodeLine = tableName != "root" ? static_cast<int>(table.location().first_line_number())
                                       : -1;

    if (!table.is_table()) {
        return make_unexpected(PyProjectTomlError::TypeError(
            tableName, "table", toml::to_string(table.type()), nodeLine));
    }

    try {
        const auto &node = toml::find(table, key);
        return getNodeValue<ExpectedType>(expectedTypeName, key, node);
    } catch (const std::out_of_range &) {
        return make_unexpected(PyProjectTomlError::MissingNodeError(tableName, key, nodeLine));
    }
}

/*!
    \brief Fetches a value of certain type from a TOML node by key if existing

    \tparam ExpectedType The type of the value to fetch. Must be a TOML primitive value type.
    \param expectedTypeName The name of the expected type
    \param nodeName The name of the node to fetch the value from
    \param node The node to fetch the value from

    \return The value if found, otherwise an error string

    \note The \a expectedTypeName and \a nodeName parameters are only used for error reporting
*/
template<typename ExpectedType>
expected<ExpectedType, PyProjectTomlError> getNodeValue(
    const std::string expectedTypeName, const std::string nodeName, const toml::ordered_value &node)
{
    auto nodeLine = static_cast<int>(node.location().first_line_number());

    if (node.is_empty())
        return make_unexpected(PyProjectTomlError::EmptyNodeError(nodeName, nodeLine));

    if constexpr (std::is_same_v<ExpectedType, toml::table>) {
        if (!node.is_table())
            return make_unexpected(PyProjectTomlError::TypeError(
                nodeName, "table", toml::to_string(node.type()), nodeLine));
        return node.as_table();
    }

    if constexpr (std::is_same_v<ExpectedType, toml::array>) {
        if (!node.is_array())
            return make_unexpected(PyProjectTomlError::TypeError(
                nodeName, "array", toml::to_string(node.type()), nodeLine));
        return node.as_array();
    }

    try {
        return toml::get<ExpectedType>(node);
    } catch (const toml::type_error &) {
        return make_unexpected(PyProjectTomlError::TypeError(
            nodeName, expectedTypeName, toml::to_string(node.type()), nodeLine));
    }
}

/*
    \brief Parses the given pyproject.toml file and returns a PyProjectTomlParseResult
    \param pyProjectTomlPath The path to the pyproject.toml file
    \returns A PyProjectTomlParseResult containing the errors found and the project information
*/
PyProjectTomlParseResult parsePyProjectToml(const FilePath &pyProjectTomlPath)
{
    PyProjectTomlParseResult result;

    const Result<QByteArray> fileContentsResult = pyProjectTomlPath.fileContents();
    if (!fileContentsResult) {
        result.errors << PyProjectTomlError::FileNotFoundError(
            pyProjectTomlPath.toUserOutput().toStdString(), -1);
        return result;
    }

    QString pyProjectTomlContent = QString::fromUtf8(fileContentsResult.value());
    toml::ordered_value rootTable;
    try {
        rootTable = toml::parse_str<toml::ordered_type_config>(pyProjectTomlContent.toStdString());
    } catch (const toml::syntax_error &syntaxErrors) {
        for (const auto &error : syntaxErrors.errors()) {
            auto errorLine = static_cast<int>(error.locations().at(0).first.first_line_number());
            result.errors << PyProjectTomlError::ParseError(error.title(), errorLine);
        }
        return result;
    }

    auto projectTable = getNodeByKey<toml::ordered_value>("table", "root", rootTable, "project");
    if (!projectTable) {
        result.errors << projectTable.error();
        return result;
    }

    auto projectName = getNodeByKey<std::string>("table", "project", projectTable.value(), "name");
    if (!projectName) {
        result.errors << projectName.error();
    } else {
        result.projectName = QString::fromUtf8(projectName.value());
    }

    auto toolTable = getNodeByKey<toml::ordered_value>("table", "root", rootTable, "tool");
    if (!toolTable) {
        result.errors << toolTable.error();
        return result;
    }

    auto pysideTable
        = getNodeByKey<toml::ordered_value>("table", "tool", toolTable.value(), "pyside6-project");
    if (!pysideTable) {
        result.errors << pysideTable.error();
        return result;
    }

    auto files
        = getNodeByKey<toml::ordered_array>("array", "pyside6-project", pysideTable.value(), "files");
    if (!files) {
        result.errors << files.error();
        return result;
    }

    const auto &filesArray = files.value();
    result.projectFiles.reserve(filesArray.size());

    for (const auto &fileNode : filesArray) {
        auto possibleFile = getNodeValue<std::string>("string", "file", fileNode);
        if (!possibleFile) {
            result.errors << possibleFile.error();
            continue;
        }
        auto file = QString::fromUtf8(possibleFile.value());
        auto filePath = pyProjectTomlPath.parentDir().pathAppended(file);
        if (!filePath.exists()) {
            auto line = static_cast<int>(fileNode.location().first_line_number());
            result.errors << PyProjectTomlError::FileNotFoundError(possibleFile.value(), line);
            continue;
        }
        result.projectFiles.append(file);
    }
    return result;
}

/*!
    \brief Given an existing pyproject.toml file, update it with the given \a projectFiles.
    \return If successful, returns the new contents of the file. Otherwise, returns an error.
*/
Result<QString> updatePyProjectTomlContent(
    const QString &pyProjectTomlContent, const QStringList &projectFiles)
{
    toml::ordered_value rootTable;
    try {
        rootTable = toml::parse_str<toml::ordered_type_config>(pyProjectTomlContent.toStdString());
    } catch (const toml::exception &error) {
        return make_unexpected(QString::fromUtf8(error.what()));
    }

    auto &toolTable = rootTable["tool"];
    if (!toolTable.is_table()) {
        toolTable = toml::ordered_table{};
    }

    auto &pysideTable = toolTable.as_table()["pyside6-project"];
    if (!pysideTable.is_table()) {
        pysideTable = toml::ordered_table{};
    }

    std::vector<std::string> filesArray;
    filesArray.reserve(projectFiles.size());
    std::transform(
        projectFiles.begin(),
        projectFiles.end(),
        std::back_inserter(filesArray),
        [](const QString &file) { return file.toStdString(); });
    std::sort(filesArray.begin(), filesArray.end());

    pysideTable["files"] = std::move(filesArray);

    auto result = QString::fromUtf8(toml::format(rootTable));
    // For some reason, the TOML library adds two trailing newlines.
    while (result.endsWith("\n\n")) {
        result.chop(1);
    }
    return result;
}

} // namespace Python::Internal
