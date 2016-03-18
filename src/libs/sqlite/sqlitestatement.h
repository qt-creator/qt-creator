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
#include "utf8stringvector.h"

#include <QString>
#include <QVariant>
#include <QVector>

#include <type_traits>
#include <memory>

struct sqlite3_stmt;
struct sqlite3;

class SQLITE_EXPORT SqliteStatement
{
protected:
    explicit SqliteStatement(const Utf8String &sqlStatementUtf8);

    static void deleteCompiledStatement(sqlite3_stmt *compiledStatement);

    bool next() const;
    void step() const;
    void reset() const;

    template<typename Type>
    Type value(int column) const;
    int columnCount() const;
    Utf8StringVector columnNames() const;

    void bind(int index, int value);
    void bind(int index, qint64 value);
    void bind(int index, double value);
    void bind(int index, const QString &text);
    void bind(int index, const QByteArray &blob);
    void bind(int index, const QVariant &value);

    template <typename Type>
    void bind(const Utf8String &name, const Type &value);

    int bindingIndexForName(const Utf8String &name);

    void bind(const RowDictionary &rowDictionary);
    void bindUnchecked(const RowDictionary &rowDictionary);

    void setBindingColumnNames(const Utf8StringVector &bindingColumnNames);
    const Utf8StringVector &bindingColumnNames() const;

    template <typename ContainerType>
    ContainerType values(const QVector<int> &columns, int size = 0) const;

    template <typename ContainerType>
    ContainerType values(int column = 0) const;

    QMap<QString, QVariant> rowColumnValueMap() const;
    QMap<QString, QVariant> twoColumnValueMap() const;

    static void execute(const Utf8String &sqlStatementUtf8);

    template <typename Type>
    static Type toValue(const Utf8String &sqlStatementUtf8);

    void prepare(const Utf8String &sqlStatementUtf8);
    void waitForUnlockNotify() const;

    void write(const RowDictionary &rowDictionary);
    void writeUnchecked(const RowDictionary &rowDictionary);

    static sqlite3 *sqliteDatabaseHandle();
    static TextEncoding databaseTextEncoding();


    bool checkForStepError(int resultCode) const;
    void checkForPrepareError(int resultCode) const;
    void setIfIsReadyToFetchValues(int resultCode) const;
    void checkIfIsReadyToFetchValues() const;
    void checkColumnsAreValid(const QVector<int> &columns) const;
    void checkColumnIsValid(int column) const;
    void checkBindingIndex(int index) const;
    void checkBindingName(int index) const;
    void checkParameterCanBeBound(const RowDictionary &rowDictionary, const Utf8String &columnName);
    void setBindingParameterCount();
    void setBindingColumnNamesFromStatement();
    void setColumnCount();
    void checkBindingValueMapIsEmpty(const RowDictionary &rowDictionary) const;
    bool isReadOnlyStatement() const;
    Q_NORETURN static void throwException(const char *whatHasHappened);

    template <typename ContainerType>
    ContainerType columnValues(const QVector<int> &columnIndices) const;

    QString columnName(int column) const;

private:
    std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)> compiledStatement;
    Utf8StringVector  bindingColumnNames_;
    int bindingParameterCount;
    int columnCount_;
    mutable bool isReadyToFetchValues;
};
