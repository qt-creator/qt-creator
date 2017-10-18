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

#include "sqlitebasestatement.h"

#include "sqlitedatabase.h"
#include "sqlitedatabasebackend.h"
#include "sqliteexception.h"

#include "sqlite3.h"

#include <condition_variable>
#include <mutex>

#if defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif

namespace Sqlite {

BaseStatement::BaseStatement(Utils::SmallStringView sqlStatement, Database &database)
    : m_compiledStatement(nullptr, deleteCompiledStatement),
      m_database(database),
      m_bindingParameterCount(0),
      m_columnCount(0),
      m_isReadyToFetchValues(false)
{
    prepare(sqlStatement);
    setBindingParameterCount();
    setColumnCount();
}

void BaseStatement::deleteCompiledStatement(sqlite3_stmt *compiledStatement)
{
    if (compiledStatement)
        sqlite3_finalize(compiledStatement);
}

class UnlockNotification
{
public:
    static void unlockNotifyCallBack(void **arguments, int argumentCount)
    {
        for (int index = 0; index < argumentCount; index++) {
          UnlockNotification *unlockNotification = static_cast<UnlockNotification *>(arguments[index]);
          unlockNotification->wakeupWaitCondition();
        }
    }

    void wakeupWaitCondition()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_fired = 1;
        }
        m_waitCondition.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_waitCondition.wait(lock, [&] () { return m_fired; });
    }

private:
    bool m_fired = false;
    std::condition_variable m_waitCondition;
    std::mutex m_mutex;
};

void BaseStatement::waitForUnlockNotify() const
{
    UnlockNotification unlockNotification;
    int resultCode = sqlite3_unlock_notify(sqliteDatabaseHandle(),
                                           UnlockNotification::unlockNotifyCallBack,
                                           &unlockNotification);

    if (resultCode == SQLITE_LOCKED)
        throw DeadLock("SqliteStatement::waitForUnlockNotify: database is in a dead lock!");

    unlockNotification.wait();
}

void BaseStatement::reset() const
{
    int resultCode = sqlite3_reset(m_compiledStatement.get());

    if (resultCode != SQLITE_OK) {
        checkForResetError(resultCode);

        m_isReadyToFetchValues = false;
    }
}

bool BaseStatement::next() const
{
    int resultCode;

    do {
        resultCode = sqlite3_step(m_compiledStatement.get());
        if (resultCode == SQLITE_LOCKED) {
            waitForUnlockNotify();
            sqlite3_reset(m_compiledStatement.get());
        }

    } while (resultCode == SQLITE_LOCKED);

    setIfIsReadyToFetchValues(resultCode);

    if (resultCode == SQLITE_ROW)
        return true;
    else if (resultCode == SQLITE_DONE)
        return false;
    else
        checkForStepError(resultCode);

    return false;
}

void BaseStatement::step() const
{
    next();
}

void BaseStatement::execute() const
{
    next();
    reset();
}

int BaseStatement::columnCount() const
{
    return m_columnCount;
}

Utils::SmallStringVector BaseStatement::columnNames() const
{
    Utils::SmallStringVector columnNames;
    int columnCount = BaseStatement::columnCount();
    columnNames.reserve(std::size_t(columnCount));
    for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        columnNames.emplace_back(sqlite3_column_origin_name(m_compiledStatement.get(), columnIndex));

    return columnNames;
}

void BaseStatement::bind(int index, int value)
{
    int resultCode = sqlite3_bind_int(m_compiledStatement.get(), index, value);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, long long value)
{
    int resultCode = sqlite3_bind_int64(m_compiledStatement.get(), index, value);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, double value)
{
    int resultCode = sqlite3_bind_double(m_compiledStatement.get(), index, value);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

void BaseStatement::bind(int index, Utils::SmallStringView text)
{
    int resultCode = sqlite3_bind_text(m_compiledStatement.get(),
                                       index,
                                       text.data(),
                                       int(text.size()),
                                       SQLITE_STATIC);
    if (resultCode != SQLITE_OK)
        checkForBindingError(resultCode);
}

template <typename Type>
void BaseStatement::bind(Utils::SmallStringView name, Type value)
{
    int index = bindingIndexForName(name);
    checkBindingName(index);
    bind(index, value);
}

template SQLITE_EXPORT void BaseStatement::bind(Utils::SmallStringView name, int value);
template SQLITE_EXPORT void BaseStatement::bind(Utils::SmallStringView name, long value);
template SQLITE_EXPORT void BaseStatement::bind(Utils::SmallStringView name, long long value);
template SQLITE_EXPORT void BaseStatement::bind(Utils::SmallStringView name, double value);
template SQLITE_EXPORT void BaseStatement::bind(Utils::SmallStringView name, Utils::SmallStringView text);

int BaseStatement::bindingIndexForName(Utils::SmallStringView name) const
{
    return  sqlite3_bind_parameter_index(m_compiledStatement.get(), name.data());
}

void BaseStatement::prepare(Utils::SmallStringView sqlStatement)
{
    int resultCode;

    do {
        sqlite3_stmt *sqliteStatement = nullptr;
        resultCode = sqlite3_prepare_v2(sqliteDatabaseHandle(),
                                        sqlStatement.data(),
                                        int(sqlStatement.size()),
                                        &sqliteStatement,
                                        nullptr);
        m_compiledStatement.reset(sqliteStatement);

        if (resultCode == SQLITE_LOCKED)
            waitForUnlockNotify();

    } while (resultCode == SQLITE_LOCKED);


    if (resultCode != SQLITE_OK)
        checkForPrepareError(resultCode);
}

sqlite3 *BaseStatement::sqliteDatabaseHandle() const
{
    return m_database.backend().sqliteDatabaseHandle();
}

TextEncoding BaseStatement::databaseTextEncoding()
{
     return m_database.backend().textEncoding();
}

void BaseStatement::checkForStepError(int resultCode) const
{
    switch (resultCode) {
        case SQLITE_BUSY: throwStatementIsBusy("SqliteStatement::stepStatement: database engine was unable to acquire the database locks!");
        case SQLITE_ERROR : throwStatementHasError("SqliteStatement::stepStatement: run-time error (such as a constraint violation) has occurred!");
        case SQLITE_MISUSE:  throwStatementIsMisused("SqliteStatement::stepStatement: was called inappropriately!");
        case SQLITE_CONSTRAINT:  throwConstraintPreventsModification("SqliteStatement::stepStatement: contraint prevent insert or update!");
    }

    throwUnknowError("SqliteStatement::stepStatement: unknown error has happened");
}

void BaseStatement::checkForResetError(int resultCode) const
{
    switch (resultCode) {
        case SQLITE_BUSY: throwStatementIsBusy("SqliteStatement::stepStatement: database engine was unable to acquire the database locks!");
        case SQLITE_ERROR : throwStatementHasError("SqliteStatement::stepStatement: run-time error (such as a constraint violation) has occurred!");
        case SQLITE_MISUSE:  throwStatementIsMisused("SqliteStatement::stepStatement: was called inappropriately!");
        case SQLITE_CONSTRAINT:  throwConstraintPreventsModification("SqliteStatement::stepStatement: contraint prevent insert or update!");
    }

    throwUnknowError("SqliteStatement::reset: unknown error has happened");
}

void BaseStatement::checkForPrepareError(int resultCode) const
{
    switch (resultCode) {
        case SQLITE_BUSY: throwStatementIsBusy("SqliteStatement::prepareStatement: database engine was unable to acquire the database locks!");
        case SQLITE_ERROR : throwStatementHasError("SqliteStatement::prepareStatement: run-time error (such as a constraint violation) has occurred!");
        case SQLITE_MISUSE:  throwStatementIsMisused("SqliteStatement::prepareStatement: was called inappropriately!");
    }

    throwUnknowError("SqliteStatement::prepareStatement: unknown error has happened");
}

void BaseStatement::checkForBindingError(int resultCode) const
{
    switch (resultCode) {
        case SQLITE_TOOBIG: throwBingingTooBig("SqliteStatement::bind: string or blob are over size limits(SQLITE_LIMIT_LENGTH)!");
        case SQLITE_RANGE : throwBindingIndexIsOutOfRange("SqliteStatement::bind: binding index is out of range!");
        case SQLITE_NOMEM: throw std::bad_alloc();
    }

    throwUnknowError("SqliteStatement::bind: unknown error has happened");
}

void BaseStatement::setIfIsReadyToFetchValues(int resultCode) const
{
    if (resultCode == SQLITE_ROW)
        m_isReadyToFetchValues = true;
    else
        m_isReadyToFetchValues = false;

}

void BaseStatement::checkIfIsReadyToFetchValues() const
{
    if (!m_isReadyToFetchValues)
        throwNoValuesToFetch("SqliteStatement::value: there are no values to fetch!");
}

void BaseStatement::checkColumnsAreValid(const std::vector<int> &columns) const
{
    for (int column : columns) {
        if (column < 0 || column >= m_columnCount)
            throwInvalidColumnFetched("SqliteStatement::values: column index out of bound!");
    }
}

void BaseStatement::checkColumnIsValid(int column) const
{
    if (column < 0 || column >= m_columnCount)
        throwInvalidColumnFetched("SqliteStatement::values: column index out of bound!");
}

void BaseStatement::checkBindingName(int index) const
{
    if (index <= 0 || index > m_bindingParameterCount)
        throwWrongBingingName("SqliteStatement::bind: binding name are not exists in this statement!");
}

void BaseStatement::setBindingParameterCount()
{
    m_bindingParameterCount = sqlite3_bind_parameter_count(m_compiledStatement.get());
}

Utils::SmallStringView chopFirstLetter(const char *rawBindingName)
{
    if (rawBindingName != nullptr)
        return Utils::SmallStringView(++rawBindingName);

    return Utils::SmallStringView("");
}

void BaseStatement::setColumnCount()
{
    m_columnCount = sqlite3_column_count(m_compiledStatement.get());
}

bool BaseStatement::isReadOnlyStatement() const
{
    return sqlite3_stmt_readonly(m_compiledStatement.get());
}

void BaseStatement::throwStatementIsBusy(const char *whatHasHappened) const
{
    throw StatementIsBusy(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
}

void BaseStatement::throwStatementHasError(const char *whatHasHappened) const
{
    throw StatementHasError(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
}

void BaseStatement::throwStatementIsMisused(const char *whatHasHappened) const
{
    throw StatementIsMisused(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
}

void BaseStatement::throwConstraintPreventsModification(const char *whatHasHappened) const
{
    throw ContraintPreventsModification(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
}

void BaseStatement::throwNoValuesToFetch(const char *whatHasHappened) const
{
    throw NoValuesToFetch(whatHasHappened);
}

void BaseStatement::throwInvalidColumnFetched(const char *whatHasHappened) const
{
    throw InvalidColumnFetched(whatHasHappened);
}

void BaseStatement::throwBindingIndexIsOutOfRange(const char *whatHasHappened) const
{
    throw BindingIndexIsOutOfRange(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
}

void BaseStatement::throwWrongBingingName(const char *whatHasHappened) const
{
    throw WrongBindingName(whatHasHappened);
}

void BaseStatement::throwUnknowError(const char *whatHasHappened) const
{
    if (sqliteDatabaseHandle())
        throw UnknowError(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
    else
        throw UnknowError(whatHasHappened);
}

void BaseStatement::throwBingingTooBig(const char *whatHasHappened) const
{
    throw BindingTooBig(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
}

QString BaseStatement::columnName(int column) const
{
    return QString::fromUtf8(sqlite3_column_name(m_compiledStatement.get(), column));
}

Database &BaseStatement::database() const
{
    return m_database;
}

template <typename StringType>
static StringType textForColumn(sqlite3_stmt *sqlStatment, int column)
{
    const char *text =  reinterpret_cast<const char*>(sqlite3_column_text(sqlStatment, column));
    std::size_t size = std::size_t(sqlite3_column_bytes(sqlStatment, column));

    return StringType(text, size);
}

template <typename StringType>
static StringType convertToTextForColumn(sqlite3_stmt *sqlStatment, int column)
{
    int dataType = sqlite3_column_type(sqlStatment, column);
    switch (dataType) {
        case SQLITE_INTEGER:
        case SQLITE_FLOAT:
        case SQLITE3_TEXT: return textForColumn<StringType>(sqlStatment, column);
        case SQLITE_BLOB:
        case SQLITE_NULL: break;
    }

    return StringType{"", 0};
}

int BaseStatement::fetchIntValue(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_int(m_compiledStatement.get(), column);
}

template<>
int BaseStatement::fetchValue<int>(int column) const
{
    return fetchIntValue(column);
}

long BaseStatement::fetchLongValue(int column) const
{
    return long(fetchValue<long long>(column));
}

template<>
long BaseStatement::fetchValue<long>(int column) const
{
    return fetchLongValue(column);
}

long long BaseStatement::fetchLongLongValue(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_int64(m_compiledStatement.get(), column);
}

template<>
long long BaseStatement::fetchValue<long long>(int column) const
{
    return fetchLongLongValue(column);
}

double BaseStatement::fetchDoubleValue(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_double(m_compiledStatement.get(), column);
}

template<>
double BaseStatement::fetchValue<double>(int column) const
{
    return fetchDoubleValue(column);
}

template<typename StringType>
StringType BaseStatement::fetchValue(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return convertToTextForColumn<StringType>(m_compiledStatement.get(), column);
}

Utils::SmallStringView BaseStatement::fetchSmallStringViewValue(int column) const
{
    return fetchValue<Utils::SmallStringView>(column);
}

template SQLITE_EXPORT Utils::SmallStringView BaseStatement::fetchValue<Utils::SmallStringView>(int column) const;
template SQLITE_EXPORT Utils::SmallString BaseStatement::fetchValue<Utils::SmallString>(int column) const;
template SQLITE_EXPORT Utils::PathString BaseStatement::fetchValue<Utils::PathString>(int column) const;
} // namespace Sqlite
