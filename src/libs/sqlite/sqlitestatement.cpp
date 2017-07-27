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

#include "sqlitestatement.h"

#include "sqlitedatabase.h"
#include "sqlitedatabasebackend.h"
#include "sqliteexception.h"

#include <QMutex>
#include <QtGlobal>
#include <QVariant>
#include <QWaitCondition>

#include "sqlite3.h"

#if defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif

namespace Sqlite {

SqliteStatement::SqliteStatement(Utils::SmallStringView sqlStatement, SqliteDatabase &database)
    : SqliteStatement(sqlStatement, database.backend())
{

}

SqliteStatement::SqliteStatement(Utils::SmallStringView sqlStatement, SqliteDatabaseBackend &databaseBackend)
    : m_compiledStatement(nullptr, deleteCompiledStatement),
      m_databaseBackend(databaseBackend),
      m_bindingParameterCount(0),
      m_columnCount(0),
      m_isReadyToFetchValues(false)
{
    prepare(sqlStatement);
    setBindingParameterCount();
    setBindingColumnNamesFromStatement();
    setColumnCount();
}

void SqliteStatement::deleteCompiledStatement(sqlite3_stmt *compiledStatement)
{
    if (compiledStatement)
        sqlite3_finalize(compiledStatement);
}

class UnlockNotification {

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
        mutex.lock();
        fired = 1;
        waitCondition.wakeAll();
        mutex.unlock();
    }

    void wait()
    {
        mutex.lock();

        if (!fired) {
            waitCondition.wait(&mutex);
        }

        mutex.unlock();
    }

private:
    bool fired = false;
    QWaitCondition waitCondition;
    QMutex mutex;
};

void SqliteStatement::waitForUnlockNotify() const
{
    UnlockNotification unlockNotification;
    int resultCode = sqlite3_unlock_notify(sqliteDatabaseHandle(), UnlockNotification::unlockNotifyCallBack, &unlockNotification);

    if (resultCode == SQLITE_OK)
        unlockNotification.wait();
    else
        throwException("SqliteStatement::waitForUnlockNotify: database is in a dead lock!");
}

void SqliteStatement::reset() const
{
    int resultCode = sqlite3_reset(m_compiledStatement.get());
    if (resultCode != SQLITE_OK)
        throwException("SqliteStatement::reset: can't reset statement!");

    m_isReadyToFetchValues = false;
}

bool SqliteStatement::next() const
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

    return checkForStepError(resultCode);
}

void SqliteStatement::step() const
{
    next();
}

int SqliteStatement::columnCount() const
{
    return m_columnCount;
}

Utils::SmallStringVector SqliteStatement::columnNames() const
{
    Utils::SmallStringVector columnNames;
    int columnCount = SqliteStatement::columnCount();
    columnNames.reserve(std::size_t(columnCount));
    for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        columnNames.emplace_back(sqlite3_column_origin_name(m_compiledStatement.get(), columnIndex));

    return columnNames;
}

void SqliteStatement::bind(int index, int value)
{
     int resultCode = sqlite3_bind_int(m_compiledStatement.get(), index, value);
     if (resultCode != SQLITE_OK)
         throwException("SqliteStatement::bind: cant' bind 32 bit integer!");
}

void SqliteStatement::bind(int index, qint64 value)
{
     int resultCode = sqlite3_bind_int64(m_compiledStatement.get(), index, value);
     if (resultCode != SQLITE_OK)
         throwException("SqliteStatement::bind: cant' bind 64 bit integer!");
}

void SqliteStatement::bind(int index, double value)
{
    int resultCode = sqlite3_bind_double(m_compiledStatement.get(), index, value);
    if (resultCode != SQLITE_OK)
        throwException("SqliteStatement::bind: cant' bind double!");
}

void SqliteStatement::bind(int index, Utils::SmallStringView text)
{
    int resultCode = sqlite3_bind_text(m_compiledStatement.get(), index, text.data(), int(text.size()), SQLITE_TRANSIENT);
    if (resultCode != SQLITE_OK)
        throwException("SqliteStatement::bind: cant' bind double!");
}

template <typename Type>
void SqliteStatement::bind(Utils::SmallStringView name, Type value)
{
    int index = bindingIndexForName(name);
    checkBindingName(index);
    bind(index, value);
}

template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, int value);
template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, qint64 value);
template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, double value);
template SQLITE_EXPORT void SqliteStatement::bind(Utils::SmallStringView name, Utils::SmallStringView text);

int SqliteStatement::bindingIndexForName(Utils::SmallStringView name)
{
    return  sqlite3_bind_parameter_index(m_compiledStatement.get(), name.data());
}

void SqliteStatement::setBindingColumnNames(const Utils::SmallStringVector &bindingColumnNames)
{
    m_bindingColumnNames = bindingColumnNames;
}

const Utils::SmallStringVector &SqliteStatement::bindingColumnNames() const
{
    return m_bindingColumnNames;
}

void SqliteStatement::prepare(Utils::SmallStringView sqlStatement)
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

    checkForPrepareError(resultCode);
}

sqlite3 *SqliteStatement::sqliteDatabaseHandle() const
{
    return m_databaseBackend.sqliteDatabaseHandle();
}

TextEncoding SqliteStatement::databaseTextEncoding()
{
     return m_databaseBackend.textEncoding();
}

bool SqliteStatement::checkForStepError(int resultCode) const
{
    switch (resultCode) {
        case SQLITE_ROW: return true;
        case SQLITE_DONE: return false;
        case SQLITE_BUSY: throwException("SqliteStatement::stepStatement: database engine was unable to acquire the database locks!");
        case SQLITE_ERROR : throwException("SqliteStatement::stepStatement: run-time error (such as a constraint violation) has occurred!");
        case SQLITE_MISUSE:  throwException("SqliteStatement::stepStatement: was called inappropriately!");
        case SQLITE_CONSTRAINT:  throwException("SqliteStatement::stepStatement: contraint prevent insert or update!");
    }

    throwException("SqliteStatement::stepStatement: unknown error has happened");

    Q_UNREACHABLE();
}

void SqliteStatement::checkForPrepareError(int resultCode) const
{
    switch (resultCode) {
        case SQLITE_OK: return;
        case SQLITE_BUSY: throwException("SqliteStatement::prepareStatement: database engine was unable to acquire the database locks!");
        case SQLITE_ERROR : throwException("SqliteStatement::prepareStatement: run-time error (such as a constraint violation) has occurred!");
        case SQLITE_MISUSE:  throwException("SqliteStatement::prepareStatement: was called inappropriately!");
    }

    throwException("SqliteStatement::prepareStatement: unknown error has happened");
}

void SqliteStatement::setIfIsReadyToFetchValues(int resultCode) const
{
    if (resultCode == SQLITE_ROW)
        m_isReadyToFetchValues = true;
    else
        m_isReadyToFetchValues = false;

}

void SqliteStatement::checkIfIsReadyToFetchValues() const
{
    if (!m_isReadyToFetchValues)
        throwException("SqliteStatement::value: there are no values to fetch!");
}

void SqliteStatement::checkColumnsAreValid(const std::vector<int> &columns) const
{
    for (int column : columns) {
        if (column < 0 || column >= m_columnCount)
            throwException("SqliteStatement::values: column index out of bound!");
    }
}

void SqliteStatement::checkColumnIsValid(int column) const
{
    if (column < 0 || column >= m_columnCount)
        throwException("SqliteStatement::values: column index out of bound!");
}

void SqliteStatement::checkBindingIndex(int index) const
{
    if (index <= 0 || index > m_bindingParameterCount)
        throwException("SqliteStatement::bind: binding index is out of bound!");
}

void SqliteStatement::checkBindingName(int index) const
{
    if (index <= 0 || index > m_bindingParameterCount)
        throwException("SqliteStatement::bind: binding name are not exists in this statement!");
}

void SqliteStatement::setBindingParameterCount()
{
    m_bindingParameterCount = sqlite3_bind_parameter_count(m_compiledStatement.get());
}

Utils::SmallStringView chopFirstLetter(const char *rawBindingName)
{
    if (rawBindingName != nullptr)
        return Utils::SmallStringView(++rawBindingName);

    return Utils::SmallStringView("");
}

void SqliteStatement::setBindingColumnNamesFromStatement()
{
    for (int index = 1; index <= m_bindingParameterCount; index++) {
        Utils::SmallStringView bindingName = chopFirstLetter(sqlite3_bind_parameter_name(m_compiledStatement.get(), index));
        m_bindingColumnNames.push_back(Utils::SmallString(bindingName));
    }
}

void SqliteStatement::setColumnCount()
{
    m_columnCount = sqlite3_column_count(m_compiledStatement.get());
}

bool SqliteStatement::isReadOnlyStatement() const
{
    return sqlite3_stmt_readonly(m_compiledStatement.get());
}

void SqliteStatement::throwException(const char *whatHasHappened) const
{
    throw SqliteException(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
}

QString SqliteStatement::columnName(int column) const
{
    return QString::fromUtf8(sqlite3_column_name(m_compiledStatement.get(), column));
}

static Utils::SmallString textForColumn(sqlite3_stmt *sqlStatment, int column)
{
    const char *text =  reinterpret_cast<const char*>(sqlite3_column_text(sqlStatment, column));
    std::size_t size = std::size_t(sqlite3_column_bytes(sqlStatment, column));

    return Utils::SmallString(text, size);
}

static Utils::SmallString convertToTextForColumn(sqlite3_stmt *sqlStatment, int column)
{
    int dataType = sqlite3_column_type(sqlStatment, column);
    switch (dataType) {
        case SQLITE_INTEGER:
        case SQLITE_FLOAT:
        case SQLITE3_TEXT: return textForColumn(sqlStatment, column);
        case SQLITE_BLOB:
        case SQLITE_NULL: return {};
    }

    Q_UNREACHABLE();
}

template<>
int SqliteStatement::value<int>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_int(m_compiledStatement.get(), column);
}

template<>
qint64 SqliteStatement::value<qint64>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_int64(m_compiledStatement.get(), column);
}

template<>
double SqliteStatement::value<double>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_double(m_compiledStatement.get(), column);
}

template<>
Utils::SmallString SqliteStatement::value<Utils::SmallString>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return convertToTextForColumn(m_compiledStatement.get(), column);
}

Utils::SmallString SqliteStatement::text(int column) const
{
    return value<Utils::SmallString>(column);
}

template <typename ContainerType>
ContainerType SqliteStatement::columnValues(const std::vector<int> &columnIndices) const
{
    using ElementType = typename ContainerType::value_type;
    ContainerType valueContainer;
    valueContainer.reserve(columnIndices.size());
    for (int columnIndex : columnIndices)
        valueContainer.push_back(value<ElementType>(columnIndex));

    return valueContainer;
}

template <typename ContainerType>
ContainerType SqliteStatement::values(const std::vector<int> &columns, int size) const
{
    checkColumnsAreValid(columns);

    ContainerType resultValues;
    resultValues.reserve(typename ContainerType::size_type(size));

    reset();

    while (next()) {
        auto values = columnValues<ContainerType>(columns);
        std::move(values.begin(), values.end(), std::back_inserter(resultValues));
    }

    return resultValues;
}

template SQLITE_EXPORT Utils::SmallStringVector SqliteStatement::values<Utils::SmallStringVector>(const std::vector<int> &columnIndices, int size) const;

template <typename ContainerType>
ContainerType SqliteStatement::values(int column) const
{
    typedef typename ContainerType::value_type ElementType;
    ContainerType resultValues;

    reset();

    while (next()) {
        resultValues.push_back(value<ElementType>(column));
    }

    return resultValues;
}

template SQLITE_EXPORT std::vector<qint64> SqliteStatement::values<std::vector<qint64>>(int column) const;
template SQLITE_EXPORT std::vector<double> SqliteStatement::values<std::vector<double>>(int column) const;
template SQLITE_EXPORT Utils::SmallStringVector SqliteStatement::values<Utils::SmallStringVector>(int column) const;

template <typename Type>
Type SqliteStatement::toValue(Utils::SmallStringView sqlStatement, SqliteDatabase &database)
{
    SqliteStatement statement(sqlStatement, database);

    statement.next();

    return statement.value<Type>(0);
}

template SQLITE_EXPORT int SqliteStatement::toValue<int>(Utils::SmallStringView sqlStatement, SqliteDatabase &database);
template SQLITE_EXPORT qint64 SqliteStatement::toValue<qint64>(Utils::SmallStringView sqlStatement, SqliteDatabase &database);
template SQLITE_EXPORT double SqliteStatement::toValue<double>(Utils::SmallStringView sqlStatement, SqliteDatabase &database);
template SQLITE_EXPORT Utils::SmallString SqliteStatement::toValue<Utils::SmallString>(Utils::SmallStringView sqlStatement, SqliteDatabase &database);

} // namespace Sqlite
