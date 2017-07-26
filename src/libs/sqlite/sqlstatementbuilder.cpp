/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "sqlstatementbuilder.h"

#include "sqlstatementbuilderexception.h"
#include "utf8stringvector.h"

#include <algorithm>

namespace Sqlite {

SqlStatementBuilder::SqlStatementBuilder(const Utf8String &sqlTemplate)
    : m_sqlTemplate(sqlTemplate)
{
}

void SqlStatementBuilder::bindEmptyText(const Utf8String &name)
{
    clearSqlStatement();
    checkIfPlaceHolderExists(name);
    changeBinding(name, Utf8String());
}

void SqlStatementBuilder::bind(const Utf8String &name, const Utf8String &text)
{
    clearSqlStatement();
    checkBindingTextIsNotEmpty(text);
    checkIfPlaceHolderExists(name);
    changeBinding(name, text);
}

void SqlStatementBuilder::bind(const Utf8String &name, const Utf8StringVector &textVector)
{
    clearSqlStatement();
    checkBindingTextVectorIsNotEmpty(textVector);
    checkIfPlaceHolderExists(name);
    changeBinding(name, textVector.join(Utf8StringLiteral(", ")));
}

void SqlStatementBuilder::bind(const Utf8String &name, int value)
{
    clearSqlStatement();
    checkIfPlaceHolderExists(name);
    changeBinding(name, Utf8String::number(value));
}

void SqlStatementBuilder::bind(const Utf8String &name, const QVector<int> &integerVector)
{
    clearSqlStatement();
    checkBindingIntegerVectorIsNotEmpty(integerVector);
    checkIfPlaceHolderExists(name);
    changeBinding(name, Utf8StringVector::fromIntegerVector(integerVector).join(Utf8StringLiteral(", ")));
}

void SqlStatementBuilder::bindWithInsertTemplateParameters(const Utf8String &name, const Utf8StringVector &columns)
{
    clearSqlStatement();
    checkBindingTextVectorIsNotEmpty(columns);
    checkIfPlaceHolderExists(name);
    changeBinding(name, insertTemplateParameters(columns));
}

void SqlStatementBuilder::bindWithUpdateTemplateParameters(const Utf8String &name, const Utf8StringVector &columns)
{
    clearSqlStatement();
    checkBindingTextVectorIsNotEmpty(columns);
    checkIfPlaceHolderExists(name);
    changeBinding(name, updateTemplateParameters(columns));
}

void SqlStatementBuilder::bindWithUpdateTemplateNames(const Utf8String &name, const Utf8StringVector &columns)
{
    clearSqlStatement();
    checkBindingTextVectorIsNotEmpty(columns);
    checkIfPlaceHolderExists(name);
    changeBinding(name, updateTemplateNames(columns));
}

void SqlStatementBuilder::clear()
{
    m_bindings.clear();
    m_sqlStatement_.clear();
}

const Utf8String SqlStatementBuilder::insertTemplateParameters(const Utf8StringVector &columns)
{
    const Utf8StringVector templateParamters(columns.count(), Utf8StringLiteral("?"));

    return templateParamters.join(Utf8StringLiteral(", "));
}

const Utf8String SqlStatementBuilder::updateTemplateParameters(const Utf8StringVector &columns)
{
    Utf8String templateParamters = columns.join(Utf8StringLiteral("=?, "));
    templateParamters.append(Utf8StringLiteral("=?"));

    return templateParamters;
}

const Utf8String SqlStatementBuilder::updateTemplateNames(const Utf8StringVector &columns)
{
    Utf8StringVector templateNames;

    foreach (const Utf8String &columnName, columns)
        templateNames.append(columnName+Utf8StringLiteral("=@")+columnName);

    return templateNames.join(Utf8StringLiteral(", "));
}

void SqlStatementBuilder::sortBindings() const
{
    std::sort(m_bindings.begin(), m_bindings.end(), [] (const BindingPair &lhs,const BindingPair &rhs)
    {
        return lhs.first.byteSize() == rhs.first.byteSize() ? lhs.first.toByteArray() < rhs.first.toByteArray() : lhs.first.byteSize() > rhs.first.byteSize();
    });
}

Utf8String SqlStatementBuilder::sqlStatement() const
{
    if (!isBuild())
        generateSqlStatement();

    return m_sqlStatement_;
}

bool SqlStatementBuilder::isBuild() const
{
    return m_sqlStatement_.hasContent();
}

Utf8String SqlStatementBuilder::columnTypeToString(ColumnType columnType)
{
    switch (columnType) {
        case ColumnType::Numeric: return Utf8StringLiteral("NUMERIC");
        case ColumnType::Integer: return Utf8StringLiteral("INTEGER");
        case ColumnType::Real: return Utf8StringLiteral("REAL");
        case ColumnType::Text: return Utf8StringLiteral("TEXT");
        case ColumnType::None: return Utf8String();
    }

    Q_UNREACHABLE();
}

void SqlStatementBuilder::generateSqlStatement() const
{
    m_sqlStatement_ = m_sqlTemplate;

    sortBindings();

    auto bindingIterator = m_bindings.cbegin();
    while (bindingIterator != m_bindings.cend()) {
        const Utf8String &placeHolderToken = bindingIterator->first;
        const Utf8String &replacementToken = bindingIterator->second;
        m_sqlStatement_.replace(placeHolderToken, replacementToken);
        ++bindingIterator;
    }

    checkIfNoPlaceHoldersAynmoreExists();
}

void SqlStatementBuilder::changeBinding(const Utf8String &name, const Utf8String &text)
{

    auto findBindingIterator = std::find_if(m_bindings.begin(), m_bindings.end(), [name] (const BindingPair &binding) {
        return binding.first == name;
    });

    if (findBindingIterator == m_bindings.end())
        m_bindings.push_back(std::make_pair(name, text));
    else
        findBindingIterator->second = text;
}

void SqlStatementBuilder::clearSqlStatement()
{
    m_sqlStatement_.clear();
}

void SqlStatementBuilder::checkIfPlaceHolderExists(const Utf8String &name) const
{
    if (name.byteSize() < 2 || !name.startsWith('$') || !m_sqlTemplate.contains(name))
        throwException("SqlStatementBuilder::bind: placeholder name does not exists!", name.constData());
}

void SqlStatementBuilder::checkIfNoPlaceHoldersAynmoreExists() const
{
    if (m_sqlStatement_.contains('$'))
        throwException("SqlStatementBuilder::bind: there are still placeholder in the sql statement!", m_sqlTemplate.constData());
}

void SqlStatementBuilder::checkBindingTextIsNotEmpty(const Utf8String &text) const
{
    if (text.isEmpty())
        throwException("SqlStatementBuilder::bind: binding text it empty!", m_sqlTemplate.constData());
}

void SqlStatementBuilder::checkBindingTextVectorIsNotEmpty(const Utf8StringVector &textVector) const
{
    if (textVector.isEmpty())
        throwException("SqlStatementBuilder::bind: binding text vector it empty!", m_sqlTemplate.constData());
}

void SqlStatementBuilder::checkBindingIntegerVectorIsNotEmpty(const QVector<int> &integerVector) const
{
    if (integerVector.isEmpty())
        throwException("SqlStatementBuilder::bind: binding integer vector it empty!", m_sqlTemplate.constData());
}

void SqlStatementBuilder::throwException(const char *whatHasHappened, const char *errorMessage)
{
    throw SqlStatementBuilderException(whatHasHappened, errorMessage);
}

} // namespace Sqlite
