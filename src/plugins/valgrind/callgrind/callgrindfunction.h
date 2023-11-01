// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMetaType>

namespace Valgrind::Callgrind {

class FunctionCall;
class CostItem;
class ParseData;

class Function
{
public:
    /// @p data the ParseData for the file this function was part of
    ///         required for the decompression of string data like function name etc.
    explicit Function(const ParseData *data);
    virtual ~Function();

    /// @return the compressed function name id
    qint64 nameId() const;
    /// @return the function name.
    QString name() const;
    /**
     * Set function name to internal string id @p id.
     * @see ParseData::stringForFunction()
     */
    void setName(qint64 id);

    /// @return the compressed file id
    qint64 fileId() const;
    /// @return the file path where this function was defined
    QString file() const;
    /**
     * Set function name to internal string id @p id.
     * @see ParseData::stringForFunction()
     */
    void setFile(qint64 id);

    /// @return the compressed object id
    qint64 objectId() const;
    /// @return the object where this function was defined
    QString object() const;
    /**
     * Set function name to internal string id @p id.
     * @see ParseData::stringForFunction()
     */
    void setObject(qint64 id);

    /**
     * @return a string representing the location of this function
     * It is a combination of file, object and line of the first CostItem.
     */
    QString location() const;

    /**
     * @return the line number of the function or -1 if not known
     */
    int lineNumber() const;

    /**
     * total accumulated self cost of @p event
     * @see ParseData::events()
     */
    quint64 selfCost(int event) const;
    QList<quint64> selfCosts() const;

    /**
     * total accumulated inclusive cost of @p event
     * @see ParseData::events()
     */
    quint64 inclusiveCost(int event) const;

    /// calls from other functions to this function
    QList<const FunctionCall *> incomingCalls() const;
    void addIncomingCall(const FunctionCall *call);
    /// @return how often this function was called in total
    quint64 called() const;

    /**
     * The detailed list of cost items, which could e.g. be used for
     * a detailed view of the function's source code annotated with
     * cost per line.
     */
    QList<const CostItem *> costItems() const;

    /**
     * Add parsed @c CostItem @p item to this function.
     *
     * NOTE: The @c Function will take ownership.
     */
    void addCostItem(const CostItem *item);

    /**
     * Function calls from this function to others.
     */
    QList<const FunctionCall *> outgoingCalls() const;
    void addOutgoingCall(const FunctionCall *call);

    /**
     * Gets called after all functions where looked up, required
     * to properly calculate inclusive cost of recursive functions
     * for example
     */
    void finalize();

protected:
    class Private;
    Private *d;

    explicit Function(Private *d);

private:
    Q_DISABLE_COPY(Function)
};

} // namespace Valgrind::Callgrind

Q_DECLARE_METATYPE(const Valgrind::Callgrind::Function *)
