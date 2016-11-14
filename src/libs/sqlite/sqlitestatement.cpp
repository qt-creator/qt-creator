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

SqliteStatement::SqliteStatement(const Utf8String &sqlStatementUtf8)
    : compiledStatement(nullptr, deleteCompiledStatement),
      bindingParameterCount(0),
      columnCount_(0),
      isReadyToFetchValues(false)
{
    prepare(sqlStatementUtf8);
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
    UnlockNotification() : fired(false) {};

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
    bool fired;
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
    int resultCode = sqlite3_reset(compiledStatement.get());
    if (resultCode != SQLITE_OK)
        throwException("SqliteStatement::reset: can't reset statement!");

    isReadyToFetchValues = false;
}

bool SqliteStatement::next() const
{
    int resultCode;

    do {
        resultCode = sqlite3_step(compiledStatement.get());
        if (resultCode == SQLITE_LOCKED) {
            waitForUnlockNotify();
            sqlite3_reset(compiledStatement.get());
        }

    } while (resultCode == SQLITE_LOCKED);

    setIfIsReadyToFetchValues(resultCode);

    return checkForStepError(resultCode);
}

void SqliteStatement::step() const
{
    next();
}

void SqliteStatement::write(const RowDictionary &rowDictionary)
{
    bind(rowDictionary);
    step();
    reset();
}

void SqliteStatement::writeUnchecked(const RowDictionary &rowDictionary)
{
    bindUnchecked(rowDictionary);
    step();
    reset();
}

int SqliteStatement::columnCount() const
{
    return columnCount_;
}

Utf8StringVector SqliteStatement::columnNames() const
{
    Utf8StringVector columnNames;
    int columnCount = SqliteStatement::columnCount();
    columnNames.reserve(columnCount);
    for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        columnNames.append(Utf8String(sqlite3_column_origin_name(compiledStatement.get(), columnIndex), -1));

    return columnNames;
}

void SqliteStatement::bind(int index, int value)
{
     int resultCode = sqlite3_bind_int(compiledStatement.get(), index, value);
     if (resultCode != SQLITE_OK)
         throwException("SqliteStatement::bind: cant' bind 32 bit integer!");
}

void SqliteStatement::bind(int index, qint64 value)
{
     int resultCode = sqlite3_bind_int64(compiledStatement.get(), index, value);
     if (resultCode != SQLITE_OK)
         throwException("SqliteStatement::bind: cant' bind 64 bit integer!");
}

void SqliteStatement::bind(int index, double value)
{
    int resultCode = sqlite3_bind_double(compiledStatement.get(), index, value);
    if (resultCode != SQLITE_OK)
        throwException("SqliteStatement::bind: cant' bind double!");
}

void SqliteStatement::bind(int index, const QString &text)
{
    int resultCode;
    if (databaseTextEncoding() == Utf8) {
        QByteArray textUtf8 = text.toUtf8();
        resultCode = sqlite3_bind_text(compiledStatement.get(), index, textUtf8.constData(), textUtf8.size(), SQLITE_TRANSIENT);
    } else {
        resultCode = sqlite3_bind_text16(compiledStatement.get(), index, text.constData(), text.size() * 2, SQLITE_TRANSIENT);
    }

    if (resultCode != SQLITE_OK)
        throwException("SqliteStatement::bind: cant' not bind text!");
}

void SqliteStatement::bind(int index, const QByteArray &blob)
{
    sqlite3_bind_blob(compiledStatement.get(), index, blob.constData(), blob.size(), SQLITE_TRANSIENT);
}

void SqliteStatement::bind(int index, const QVariant &value)
{
    checkBindingIndex(index);

    switch (value.type()) {
        case QVariant::Bool:
        case QVariant::Int:
            bind(index, value.toInt());
            break;
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
            bind(index, value.toLongLong());
            break;
        case QVariant::Double:
            bind(index, value.toDouble());
            break;
        case QVariant::String:
            bind(index, value.toString());
            break;
        case QVariant::ByteArray:
            bind(index, value.toByteArray());
            break;
        default:
            sqlite3_bind_null(compiledStatement.get(), index);
    }
}

template <typename Type>
void SqliteStatement::bind(const Utf8String &name, const Type &value)
{
    int index = bindingIndexForName(name);
    checkBindingName(index);
    bind(index, value);
}

template SQLITE_EXPORT void SqliteStatement::bind(const Utf8String &name, const int &value);
template SQLITE_EXPORT void SqliteStatement::bind(const Utf8String &name, const qint64 &value);
template SQLITE_EXPORT void SqliteStatement::bind(const Utf8String &name, const double &value);
template SQLITE_EXPORT void SqliteStatement::bind(const Utf8String &name, const QString &text);
template SQLITE_EXPORT void SqliteStatement::bind(const Utf8String &name, const QByteArray &blob);
template SQLITE_EXPORT void SqliteStatement::bind(const Utf8String &name, const QVariant &value);

int SqliteStatement::bindingIndexForName(const Utf8String &name)
{
    return  sqlite3_bind_parameter_index(compiledStatement.get(), name.constData());
}

void SqliteStatement::bind(const RowDictionary &rowDictionary)
{
    checkBindingValueMapIsEmpty(rowDictionary);

    int columnIndex = 1;
    foreach (const Utf8String &columnName, bindingColumnNames_) {
        checkParameterCanBeBound(rowDictionary, columnName);
        QVariant value = rowDictionary.value(columnName);
        bind(columnIndex, value);
        columnIndex += 1;
    }
}

void SqliteStatement::bindUnchecked(const RowDictionary &rowDictionary)
{
    checkBindingValueMapIsEmpty(rowDictionary);

    int columnIndex = 1;
    foreach (const Utf8String &columnName, bindingColumnNames_) {
        if (rowDictionary.contains(columnName)) {
            QVariant value = rowDictionary.value(columnName);
            bind(columnIndex, value);
        }
        columnIndex += 1;
    }
}

void SqliteStatement::setBindingColumnNames(const Utf8StringVector &bindingColumnNames)
{
    bindingColumnNames_ = bindingColumnNames;
}

const Utf8StringVector &SqliteStatement::bindingColumnNames() const
{
    return bindingColumnNames_;
}

void SqliteStatement::execute(const Utf8String &sqlStatementUtf8)
{
    SqliteStatement statement(sqlStatementUtf8);
    statement.step();
}

void SqliteStatement::prepare(const Utf8String &sqlStatementUtf8)
{
    int resultCode;

    do {
        sqlite3_stmt *sqliteStatement = nullptr;
        resultCode = sqlite3_prepare_v2(sqliteDatabaseHandle(), sqlStatementUtf8.constData(), sqlStatementUtf8.byteSize(), &sqliteStatement, nullptr);
        compiledStatement.reset(sqliteStatement);

        if (resultCode == SQLITE_LOCKED)
            waitForUnlockNotify();

    } while (resultCode == SQLITE_LOCKED);

    checkForPrepareError(resultCode);
}

sqlite3 *SqliteStatement::sqliteDatabaseHandle()
{
return SqliteDatabaseBackend::sqliteDatabaseHandle();
}

TextEncoding SqliteStatement::databaseTextEncoding()
{
    if (SqliteDatabaseBackend::threadLocalInstance())
        return SqliteDatabaseBackend::threadLocalInstance()->textEncoding();

    throwException("SqliteStatement::databaseTextEncoding: database backend instance is null!");

    Q_UNREACHABLE();
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
        isReadyToFetchValues = true;
    else
        isReadyToFetchValues = false;

}

void SqliteStatement::checkIfIsReadyToFetchValues() const
{
    if (!isReadyToFetchValues)
        throwException("SqliteStatement::value: there are no values to fetch!");
}

void SqliteStatement::checkColumnsAreValid(const QVector<int> &columns) const
{
    foreach (int column, columns) {
        if (column < 0 || column >= columnCount_)
            throwException("SqliteStatement::values: column index out of bound!");
    }
}

void SqliteStatement::checkColumnIsValid(int column) const
{
    if (column < 0 || column >= columnCount_)
        throwException("SqliteStatement::values: column index out of bound!");
}

void SqliteStatement::checkBindingIndex(int index) const
{
    if (index <= 0 || index > bindingParameterCount)
        throwException("SqliteStatement::bind: binding index is out of bound!");
}

void SqliteStatement::checkBindingName(int index) const
{
    if (index <= 0 || index > bindingParameterCount)
        throwException("SqliteStatement::bind: binding name are not exists in this statement!");
}

void SqliteStatement::checkParameterCanBeBound(const RowDictionary &rowDictionary, const Utf8String &columnName)
{
    if (!rowDictionary.contains(columnName))
        throwException("SqliteStatement::bind: Not all parameters are bound!");
}

void SqliteStatement::setBindingParameterCount()
{
    bindingParameterCount = sqlite3_bind_parameter_count(compiledStatement.get());
}

Utf8String chopFirstLetter(const char *rawBindingName)
{
    QByteArray bindingName(rawBindingName);
    bindingName = bindingName.mid(1);

    return Utf8String::fromByteArray(bindingName);
}

void SqliteStatement::setBindingColumnNamesFromStatement()
{
    for (int index = 1; index <= bindingParameterCount; index++) {
        Utf8String bindingName = chopFirstLetter(sqlite3_bind_parameter_name(compiledStatement.get(), index));
        bindingColumnNames_.append(bindingName);
    }
}

void SqliteStatement::setColumnCount()
{
    columnCount_ = sqlite3_column_count(compiledStatement.get());
}

void SqliteStatement::checkBindingValueMapIsEmpty(const RowDictionary &rowDictionary) const
{
    if (rowDictionary.isEmpty())
        throwException("SqliteStatement::bind: can't bind empty row!");
}

bool SqliteStatement::isReadOnlyStatement() const
{
    return sqlite3_stmt_readonly(compiledStatement.get());
}

void SqliteStatement::throwException(const char *whatHasHappened)
{
    throw SqliteException(whatHasHappened, sqlite3_errmsg(sqliteDatabaseHandle()));
}

QString SqliteStatement::columnName(int column) const
{
    return QString::fromUtf8(sqlite3_column_name(compiledStatement.get(), column));
}

static bool columnIsBlob(sqlite3_stmt *sqlStatment, int column)
{
    return sqlite3_column_type(sqlStatment, column) == SQLITE_BLOB;
}

static QByteArray byteArrayForColumn(sqlite3_stmt *sqlStatment, int column)
{
    if (columnIsBlob(sqlStatment, column)) {
        const char *blob =  static_cast<const char*>(sqlite3_column_blob(sqlStatment, column));
        int size = sqlite3_column_bytes(sqlStatment, column);

        return QByteArray(blob, size);
    }

    return QByteArray();
}

static QString textForColumn(sqlite3_stmt *sqlStatment, int column)
{
    const QChar *text =  static_cast<const QChar*>(sqlite3_column_text16(sqlStatment, column));
    int size = sqlite3_column_bytes16(sqlStatment, column) / 2;

    return QString(text, size);
}

static Utf8String utf8TextForColumn(sqlite3_stmt *sqlStatment, int column)
{
    const char *text =  reinterpret_cast<const char*>(sqlite3_column_text(sqlStatment, column));
    int size = sqlite3_column_bytes(sqlStatment, column);

    return Utf8String(text, size);
}


static Utf8String convertedToUtf8StringForColumn(sqlite3_stmt *sqlStatment, int column)
{
    int dataType = sqlite3_column_type(sqlStatment, column);
    switch (dataType) {
        case SQLITE_INTEGER: return Utf8String::fromByteArray(QByteArray::number(sqlite3_column_int64(sqlStatment, column)));
        case SQLITE_FLOAT: return Utf8String::fromByteArray(QByteArray::number(sqlite3_column_double(sqlStatment, column)));
        case SQLITE_BLOB: return Utf8String();
        case SQLITE3_TEXT: return utf8TextForColumn(sqlStatment, column);
        case SQLITE_NULL: return Utf8String();
    }

    Q_UNREACHABLE();
}


static QVariant variantForColumn(sqlite3_stmt *sqlStatment, int column)
{
    int dataType = sqlite3_column_type(sqlStatment, column);
    switch (dataType) {
        case SQLITE_INTEGER: return QVariant::fromValue(sqlite3_column_int64(sqlStatment, column));
        case SQLITE_FLOAT: return QVariant::fromValue(sqlite3_column_double(sqlStatment, column));
        case SQLITE_BLOB: return QVariant::fromValue(byteArrayForColumn(sqlStatment, column));
        case SQLITE3_TEXT: return QVariant::fromValue(textForColumn(sqlStatment, column));
        case SQLITE_NULL: return QVariant();
    }

    Q_UNREACHABLE();
}

template<>
int SqliteStatement::value<int>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_int(compiledStatement.get(), column);
}

template<>
qint64 SqliteStatement::value<qint64>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_int64(compiledStatement.get(), column);
}

template<>
double SqliteStatement::value<double>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return sqlite3_column_double(compiledStatement.get(), column);
}

template<>
QByteArray SqliteStatement::value<QByteArray>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return byteArrayForColumn(compiledStatement.get(), column);
}

template<>
Utf8String SqliteStatement::value<Utf8String>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return convertedToUtf8StringForColumn(compiledStatement.get(), column);
}

template<>
QString SqliteStatement::value<QString>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return textForColumn(compiledStatement.get(), column);
}

template<>
QVariant SqliteStatement::value<QVariant>(int column) const
{
    checkIfIsReadyToFetchValues();
    checkColumnIsValid(column);
    return variantForColumn(compiledStatement.get(), column);
}

template <typename ContainerType>
 ContainerType SqliteStatement::columnValues(const QVector<int> &columnIndices) const
{
    typedef typename ContainerType::value_type ElementType;
    ContainerType valueContainer;
    valueContainer.reserve(columnIndices.count());
    for (int columnIndex : columnIndices)
        valueContainer += value<ElementType>(columnIndex);

    return valueContainer;
}

QMap<QString, QVariant> SqliteStatement::rowColumnValueMap() const
{
    QMap<QString, QVariant> values;

    reset();

    if (next()) {
        for (int column = 0; column < columnCount(); column++)
            values.insert(columnName(column), variantForColumn(compiledStatement.get(), column));
    }

    return values;
}

QMap<QString, QVariant> SqliteStatement::twoColumnValueMap() const
{
    QMap<QString, QVariant> values;

    reset();

    while (next())
        values.insert(textForColumn(compiledStatement.get(), 0), variantForColumn(compiledStatement.get(), 1));

    return values;
}

template <typename ContainerType>
ContainerType SqliteStatement::values(const QVector<int> &columns, int size) const
{
    checkColumnsAreValid(columns);

    ContainerType resultValues;
    resultValues.reserve(size);

    reset();

    while (next()) {
        resultValues += columnValues<ContainerType>(columns);
    }

    return resultValues;
}

template SQLITE_EXPORT QVector<QVariant> SqliteStatement::values<QVector<QVariant>>(const QVector<int> &columnIndices, int size) const;
template SQLITE_EXPORT QVector<Utf8String> SqliteStatement::values<QVector<Utf8String>>(const QVector<int> &columnIndices, int size) const;

template <typename ContainerType>
ContainerType SqliteStatement::values(int column) const
{
    typedef typename ContainerType::value_type ElementType;
    ContainerType resultValues;

    reset();

    while (next()) {
        resultValues += value<ElementType>(column);
    }

    return resultValues;
}

template SQLITE_EXPORT QVector<qint64> SqliteStatement::values<QVector<qint64>>(int column) const;
template SQLITE_EXPORT QVector<double> SqliteStatement::values<QVector<double>>(int column) const;
template SQLITE_EXPORT QVector<QByteArray> SqliteStatement::values<QVector<QByteArray>>(int column) const;
template SQLITE_EXPORT Utf8StringVector SqliteStatement::values<Utf8StringVector>(int column) const;
template SQLITE_EXPORT QVector<QString> SqliteStatement::values<QVector<QString>>(int column) const;

template <typename Type>
Type SqliteStatement::toValue(const Utf8String &sqlStatementUtf8)
{
    SqliteStatement statement(sqlStatementUtf8);

    statement.next();

    return statement.value<Type>(0);
}

template SQLITE_EXPORT int SqliteStatement::toValue<int>(const Utf8String &sqlStatementUtf8);
template SQLITE_EXPORT qint64 SqliteStatement::toValue<qint64>(const Utf8String &sqlStatementUtf8);
template SQLITE_EXPORT double SqliteStatement::toValue<double>(const Utf8String &sqlStatementUtf8);
template SQLITE_EXPORT QString SqliteStatement::toValue<QString>(const Utf8String &sqlStatementUtf8);
template SQLITE_EXPORT QByteArray SqliteStatement::toValue<QByteArray>(const Utf8String &sqlStatementUtf8);
template SQLITE_EXPORT Utf8String SqliteStatement::toValue<Utf8String>(const Utf8String &sqlStatementUtf8);
template SQLITE_EXPORT QVariant SqliteStatement::toValue<QVariant>(const Utf8String &sqlStatementUtf8);

