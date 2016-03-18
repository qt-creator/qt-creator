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

#include "utf8string.h"

#include <utility>
#include <vector>

class SQLITE_EXPORT SqlStatementBuilder
{
    using BindingPair = std::pair<Utf8String, Utf8String>;
public:
    SqlStatementBuilder(const Utf8String &sqlTemplate);

    void bindEmptyText(const Utf8String &name);
    void bind(const Utf8String &name, const Utf8String &text);
    void bind(const Utf8String &name, const Utf8StringVector &textVector);
    void bind(const Utf8String &name, int value);
    void bind(const Utf8String &name, const QVector<int> &integerVector);
    void bindWithInsertTemplateParameters(const Utf8String &name, const Utf8StringVector &columns);
    void bindWithUpdateTemplateParameters(const Utf8String &name, const Utf8StringVector &columns);
    void bindWithUpdateTemplateNames(const Utf8String &name, const Utf8StringVector &columns);
    void clear();

    Utf8String sqlStatement() const;

    bool isBuild() const;

    static Utf8String columnTypeToString(ColumnType columnType);

protected:
    static const Utf8String insertTemplateParameters(const Utf8StringVector &columns);
    static const Utf8String updateTemplateParameters(const Utf8StringVector &columns);
    static const Utf8String updateTemplateNames(const Utf8StringVector &columns);

    void sortBindings() const;
    void generateSqlStatement() const;

    void changeBinding(const Utf8String &name, const Utf8String &text);

    void clearSqlStatement();
    void checkIfPlaceHolderExists(const Utf8String &name) const;
    void checkIfNoPlaceHoldersAynmoreExists() const;
    void checkBindingTextIsNotEmpty(const Utf8String &text) const;
    void checkBindingTextVectorIsNotEmpty(const Utf8StringVector &textVector) const;
    void checkBindingIntegerVectorIsNotEmpty(const QVector<int> &integerVector) const;

    Q_NORETURN static void throwException(const char *whatHasHappened, const char *errorMessage);

private:
    Utf8String sqlTemplate;
    mutable Utf8String sqlStatement_;
    mutable std::vector<BindingPair> bindings;
};
