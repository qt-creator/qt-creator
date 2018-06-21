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

    static Utils::SmallString columnTypeToString(ColumnType columnType);

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

    Q_NORETURN static void throwException(const char *whatHasHappened, const char *errorMessage);

private:
    Utils::BasicSmallString<510> m_sqlTemplate;
    mutable Utils::BasicSmallString<510> m_sqlStatement;
    mutable std::vector<BindingPair> m_bindings;
};

} // namespace Sqlite
