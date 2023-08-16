// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QList>

namespace Valgrind::Callgrind {

class Function;

/**
 * This represents a function call.
 */
class FunctionCall
{
public:
    explicit FunctionCall();
    ~FunctionCall();

    /// the called function
    const Function *callee() const;
    void setCallee(const Function *function);

    /// the calling function
    const Function *caller() const;
    void setCaller(const Function *function);

    /// how often the function was called
    quint64 calls() const;
    void setCalls(quint64 calls);

    /**
     * Destination position data for the given position-index @p posIdx
     * @see ParseData::positions()
     */
    quint64 destination(int posIdx) const;
    QList<quint64> destinations() const;
    void setDestinations(const QList<quint64> &destinations);

    /**
     * Inclusive cost of the function call.
     * @see ParseData::events()
     */
    quint64 cost(int event) const;
    QList<quint64> costs() const;
    void setCosts(const QList<quint64> &costs);

private:
    Q_DISABLE_COPY(FunctionCall)

    class Private;
    Private *d;
};

} // namespace Valgrind::Callgrind

Q_DECLARE_METATYPE(const Valgrind::Callgrind::FunctionCall *)
