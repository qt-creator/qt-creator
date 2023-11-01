// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

namespace Valgrind::Callgrind {

class FunctionCall;
class ParseData;

/**
 * This class represents the cost(s) at given position(s).
 */
class CostItem
{
public:
    /// @p data the file data this cost item was parsed in.
    ///         required for decompression of string data like differing source file information
    explicit CostItem(ParseData *data);
    ~CostItem();

    /**
     * Position data for the given position-index @p posIdx
     * @see ParseData::positions()
     */
    quint64 position(int posIdx) const;
    void setPosition(int posIdx, quint64 position);
    QList<quint64> positions() const;

    /**
     * Cost data for the given event-index @p event
     * @see ParseData::events()
     */
    quint64 cost(int event) const;
    void setCost(int event, quint64 cost);
    QList<quint64> costs() const;

    /**
     * If this cost item represents a function call, this will return the @c Callee.
     * Otherwise zero will be returned.
     */
    const FunctionCall *call() const;
    ///NOTE: @c CostItem will take ownership
    void setCall(const FunctionCall *call);

    /**
     * If this cost item represents a jump to a different file, this will
     * return the path to that file. The string will be empty otherwise.
     */
    QString differingFile() const;
    /// @return compressed file id or -1 if none is set
    qint64 differingFileId() const;
    void setDifferingFile(qint64 fileId);

private:
    Q_DISABLE_COPY(CostItem)

    class Private;
    Private *d;
};

} // Valgrind::Callgrind
