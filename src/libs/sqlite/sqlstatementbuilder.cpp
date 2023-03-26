// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqlstatementbuilder.h"

#include "sqlstatementbuilderexception.h"

#include <utils/smallstringvector.h>

#include <algorithm>

namespace Sqlite {

SqlStatementBuilder::SqlStatementBuilder(Utils::SmallStringView sqlTemplate)
    : m_sqlTemplate(std::move(sqlTemplate))
{
}

void SqlStatementBuilder::bindEmptyText(Utils::SmallString &&name)
{
    clearSqlStatement();
    checkIfPlaceHolderExists(name);
    changeBinding(std::move(name), {});
}

void SqlStatementBuilder::bind(Utils::SmallString &&name, Utils::SmallString &&text)
{
    clearSqlStatement();
    checkBindingTextIsNotEmpty(text);
    checkIfPlaceHolderExists(name);
    changeBinding(std::move(name), std::move(text));
}

void SqlStatementBuilder::bind(Utils::SmallString &&name, const Utils::SmallStringVector &textVector)
{
    clearSqlStatement();
    checkBindingTextVectorIsNotEmpty(textVector);
    checkIfPlaceHolderExists(name);
    changeBinding(std::move(name), textVector.join(", "));
}

void SqlStatementBuilder::bind(Utils::SmallString &&name, int value)
{
    clearSqlStatement();
    checkIfPlaceHolderExists(name);
    changeBinding(std::move(name), Utils::SmallString::number(value));
}

namespace {
Utils::SmallStringVector integerVectorToStringVector(const std::vector<int> &integerVector)
{
    Utils::SmallStringVector stringVector;
    stringVector.reserve(integerVector.size());

    std::transform(integerVector.begin(),
                   integerVector.end(),
                   std::back_inserter(stringVector),
                   [] (int i) { return Utils::SmallString::number(i); });

    return stringVector;
}
}
void SqlStatementBuilder::bind(Utils::SmallString &&name, const std::vector<int> &integerVector)
{
    clearSqlStatement();
    checkBindingIntegerVectorIsNotEmpty(integerVector);
    checkIfPlaceHolderExists(name);
    changeBinding(std::move(name), integerVectorToStringVector(integerVector).join(", "));
}

void SqlStatementBuilder::bindWithInsertTemplateParameters(Utils::SmallString &&name,
                                                           const Utils::SmallStringVector &columns)
{
    clearSqlStatement();
    checkBindingTextVectorIsNotEmpty(columns);
    checkIfPlaceHolderExists(name);
    changeBinding(std::move(name), insertTemplateParameters(columns));
}

void SqlStatementBuilder::bindWithUpdateTemplateParameters(Utils::SmallString &&name,
                                                           const Utils::SmallStringVector &columns)
{
    clearSqlStatement();
    checkBindingTextVectorIsNotEmpty(columns);
    checkIfPlaceHolderExists(name);
    changeBinding(std::move(name), updateTemplateParameters(columns));
}

void SqlStatementBuilder::bindWithUpdateTemplateNames(Utils::SmallString &&name,
                                                      const Utils::SmallStringVector &columns)
{
    clearSqlStatement();
    checkBindingTextVectorIsNotEmpty(columns);
    checkIfPlaceHolderExists(name);
    changeBinding(std::move(name), updateTemplateNames(columns));
}

void SqlStatementBuilder::clear()
{
    m_bindings.clear();
    m_sqlStatement.clear();
}

Utils::SmallString SqlStatementBuilder::insertTemplateParameters(const Utils::SmallStringVector &columns)
{
    const Utils::SmallStringVector templateParamters(columns.size(), "?");

    return templateParamters.join(", ");
}

Utils::SmallString SqlStatementBuilder::updateTemplateParameters(const Utils::SmallStringVector &columns)
{
    Utils::SmallString templateParamters = columns.join("=?, ");
    templateParamters.append("=?");

    return templateParamters;
}

Utils::SmallString SqlStatementBuilder::updateTemplateNames(const Utils::SmallStringVector &columns)
{
    Utils::SmallStringVector templateNames;
    templateNames.reserve(columns.size());

    auto convertToTemplateNames = [] (const Utils::SmallString &column) {
        return Utils::SmallString::join({column, "=@", column});
    };

    std::transform(columns.begin(),
                   columns.end(),
                   std::back_inserter(templateNames),
                   convertToTemplateNames);

    return templateNames.join(", ");
}

void SqlStatementBuilder::sortBindings() const
{
    std::sort(m_bindings.begin(), m_bindings.end(), [] (const BindingPair &lhs,const BindingPair &rhs)
    {
        return lhs.first.size() == rhs.first.size() ? lhs.first < rhs.first : lhs.first.size() > rhs.first.size();
    });
}

Utils::SmallStringView SqlStatementBuilder::sqlStatement() const
{
    if (!isBuild())
        generateSqlStatement();

    return m_sqlStatement;
}

bool SqlStatementBuilder::isBuild() const
{
    return m_sqlStatement.hasContent();
}

void SqlStatementBuilder::generateSqlStatement() const
{
    m_sqlStatement = m_sqlTemplate;

    sortBindings();

    for (const auto &entry : m_bindings) {
        const Utils::SmallStringView placeHolderToken = entry.first;
        const Utils::SmallStringView replacementToken = entry.second;
        m_sqlStatement.replace(placeHolderToken, replacementToken);
    }

    checkIfNoPlaceHoldersAynmoreExists();
}

void SqlStatementBuilder::changeBinding(Utils::SmallString &&name, Utils::SmallString &&text)
{
    auto findBindingIterator = std::find_if(m_bindings.begin(),
                                            m_bindings.end(),
                                            [&] (const BindingPair &binding) {
        return binding.first == name;
    });

    if (findBindingIterator == m_bindings.end())
        m_bindings.push_back(std::make_pair(std::move(name), std::move(text)));
    else
        findBindingIterator->second = std::move(text);
}

void SqlStatementBuilder::clearSqlStatement()
{
    m_sqlStatement.clear();
}

void SqlStatementBuilder::checkIfPlaceHolderExists(Utils::SmallStringView name) const
{
    if (name.size() < 2 || !name.startsWith('$') || !m_sqlTemplate.contains(name))
        throwException("SqlStatementBuilder::bind: placeholder name does not exist!", name.data());
}

void SqlStatementBuilder::checkIfNoPlaceHoldersAynmoreExists() const
{
    if (m_sqlStatement.contains('$'))
        throwException(
            "SqlStatementBuilder::bind: there are still placeholder in the sql statement!",
            m_sqlTemplate);
}

void SqlStatementBuilder::checkBindingTextIsNotEmpty(Utils::SmallStringView text) const
{
    if (text.isEmpty())
        throwException("SqlStatementBuilder::bind: binding text it empty!", m_sqlTemplate);
}

void SqlStatementBuilder::checkBindingTextVectorIsNotEmpty(const Utils::SmallStringVector &textVector) const
{
    if (textVector.empty())
        throwException("SqlStatementBuilder::bind: binding text vector it empty!", m_sqlTemplate);
}

void SqlStatementBuilder::checkBindingIntegerVectorIsNotEmpty(const std::vector<int> &integerVector) const
{
    if (integerVector.empty())
        throwException("SqlStatementBuilder::bind: binding integer vector it empty!", m_sqlTemplate);
}

void SqlStatementBuilder::throwException(const char *whatHasHappened, Utils::SmallString sqlTemplate)
{
    throw SqlStatementBuilderException(whatHasHappened, std::move(sqlTemplate));
}

const char *SqlStatementBuilderException::what() const noexcept
{
    return m_whatHasHappened;
}

} // namespace Sqlite
