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

#include "sqliteexception.h"

#include <utils/smallstringvector.h>

#include <type_traits>
#include <memory>

struct sqlite3_stmt;
struct sqlite3;

namespace Sqlite {

class SqliteDatabase;
class SqliteDatabaseBackend;

class SQLITE_EXPORT SqliteStatement
{
protected:
    explicit SqliteStatement(Utils::SmallStringView sqlStatement, SqliteDatabase &database);

    static void deleteCompiledStatement(sqlite3_stmt *m_compiledStatement);

    bool next() const;
    void step() const;
    void reset() const;

    template<typename Type>
    Type value(int column) const;
    Utils::SmallString text(int column) const;
    int columnCount() const;
    Utils::SmallStringVector columnNames() const;

    void bind(int index, int value);
    void bind(int index, qint64 value);
    void bind(int index, double value);
    void bind(int index, Utils::SmallStringView value);

    template <typename Type>
    void bind(Utils::SmallStringView name, Type value);

    int bindingIndexForName(Utils::SmallStringView name);

    void setBindingColumnNames(const Utils::SmallStringVector &bindingColumnNames);
    const Utils::SmallStringVector &bindingColumnNames() const;

    template <typename ContainerType>
    ContainerType values(const std::vector<int> &columns, int size = 0) const;

    template <typename ContainerType>
    ContainerType values(int column = 0) const;

    template <typename Type>
    static Type toValue(Utils::SmallStringView sqlStatement, SqliteDatabase &database);

    void prepare(Utils::SmallStringView sqlStatement);
    void waitForUnlockNotify() const;

    sqlite3 *sqliteDatabaseHandle() const;
    TextEncoding databaseTextEncoding();


    bool checkForStepError(int resultCode) const;
    void checkForPrepareError(int resultCode) const;
    void setIfIsReadyToFetchValues(int resultCode) const;
    void checkIfIsReadyToFetchValues() const;
    void checkColumnsAreValid(const std::vector<int> &columns) const;
    void checkColumnIsValid(int column) const;
    void checkBindingIndex(int index) const;
    void checkBindingName(int index) const;
    void setBindingParameterCount();
    void setBindingColumnNamesFromStatement();
    void setColumnCount();
    bool isReadOnlyStatement() const;
    Q_NORETURN void throwException(const char *whatHasHappened) const;

    template <typename ContainerType>
    ContainerType columnValues(const std::vector<int> &columnIndices) const;

    QString columnName(int column) const;

protected:
    explicit SqliteStatement(Utils::SmallStringView sqlStatement,
                             SqliteDatabaseBackend &databaseBackend);

private:
    std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)> m_compiledStatement;
    Utils::SmallStringVector m_bindingColumnNames;
    SqliteDatabaseBackend &m_databaseBackend;
    int m_bindingParameterCount;
    int m_columnCount;
    mutable bool m_isReadyToFetchValues;
};

} // namespace Sqlite
