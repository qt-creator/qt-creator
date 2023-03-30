// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteglobal.h"

#include <utils/smallstringvector.h>

#include <utility>
#include <vector>

namespace Sqlite {

class SQLITE_EXPORT SqlStatementBuilder
{
    using BindingPair = std::pair<Utils::SmallString, Utils::SmallString>;
public:
    SqlStatementBuilder(Utils::SmallStringView m_sqlTemplate);

    void bindEmptyText(Utils::SmallString &&name);
    void bind(Utils::SmallString &&name, Utils::SmallString &&text);
    void bind(Utils::SmallString &&name, const Utils::SmallStringVector &textVector);
    void bind(Utils::SmallString &&name, int value);
    void bind(Utils::SmallString &&name, const std::vector<int> &integerVector);
    void bindWithInsertTemplateParameters(Utils::SmallString &&name,
                                          const Utils::SmallStringVector &columns);
    void bindWithUpdateTemplateParameters(Utils::SmallString &&name,
                                          const Utils::SmallStringVector &columns);
    void bindWithUpdateTemplateNames(Utils::SmallString &&name,
                                     const Utils::SmallStringVector &columns);
    void clear();

    Utils::SmallStringView sqlStatement() const;

    bool isBuild() const;

protected:
    static Utils::SmallString insertTemplateParameters(const Utils::SmallStringVector &columns);
    static Utils::SmallString updateTemplateParameters(const Utils::SmallStringVector &columns);
    static Utils::SmallString updateTemplateNames(const Utils::SmallStringVector &columns);

    void sortBindings() const;
    void generateSqlStatement() const;

    void changeBinding(Utils::SmallString &&name, Utils::SmallString &&text);

    void clearSqlStatement();
    void checkIfPlaceHolderExists(Utils::SmallStringView name) const;
    void checkIfNoPlaceHoldersAynmoreExists() const;
    void checkBindingTextIsNotEmpty(Utils::SmallStringView text) const;
    void checkBindingTextVectorIsNotEmpty(const Utils::SmallStringVector &textVector) const;
    void checkBindingIntegerVectorIsNotEmpty(const std::vector<int> &integerVector) const;

    Q_NORETURN static void throwException(const char *whatHasHappened,
                                          Utils::SmallString sqlTemplate);

private:
    Utils::BasicSmallString<510> m_sqlTemplate;
    mutable Utils::BasicSmallString<510> m_sqlStatement;
    mutable std::vector<BindingPair> m_bindings;
};

} // namespace Sqlite
