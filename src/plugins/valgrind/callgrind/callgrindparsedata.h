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

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
template <typename T> class QVector;
class QStringList;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {

class Function;

/**
 * Represents all the information extracted from a callgrind data file.
 */
class ParseData
{
public:
    explicit ParseData();
    ~ParseData();

    static QString prettyStringForEvent(const QString &event);
    /// List of events reported in the data file.
    QStringList events() const;
    void setEvents(const QStringList &events);

    static QString prettyStringForPosition(const QString &position);
    /// List of positions reported in the data file.
    QStringList positions() const;
    void setPositions(const QStringList &positions);

    /// the index of the line number in @c positions()
    /// or -1 if no line numbers where reported.
    int lineNumberPositionIndex() const;

    /**
     * Total cost of @p event reported in the data file.
     *
     * @see events()
     */
    quint64 totalCost(uint event) const;
    void setTotalCost(uint event, quint64 cost);

    /**
     * When @p detectCycles is set to true, the returned list will have all @c Function's in call
     * cycles replaced with @c FunctionCycle.
     *
     * @return All functions that where reported in the data file.
     */
    QVector<const Function *> functions(bool detectCycles = false) const;
    /// NOTE: The @c ParseData will take ownership.
    void addFunction(const Function *function);

    /// @return executed command with arguments
    QString command() const;
    void setCommand(const QString &command);

    /// @return pid of executed command
    quint64 pid() const;
    void setPid(quint64 pid);

    /// @return number of data, if callgrind_control --dump was used
    uint part() const;
    void setPart(uint part) const;

    /// @return list of desc: lines in the data
    QStringList descriptions() const;
    void addDescription(const QString &description);
    void setDescriptions(const QStringList &descriptions);

    /// @return version of the callgrind data format
    int version() const;
    void setVersion(int version);

    /// @return creator of the data
    QString creator() const;
    void setCreator(const QString &creator);

    /**
     * Internal name compression lookup table.
     *
     * We save the @c QString representations of the compressed data format only once here.
     * This should make sure the memory consumption doesn't skyrocket as long
     * as these strings are only displayed without applying detaching operations on them.
     */

    /// for Objects
    QString stringForObjectCompression(qint64 id) const;
    /// @p id if it is -1, an uncompressed string is assumed and it will be compressed internally
    void addCompressedObject(const QString &object, qint64 &id);

    /// for Files
    QString stringForFileCompression(qint64 id) const;
    /// @p id if it is -1, an uncompressed string is assumed and it will be compressed internally
    void addCompressedFile(const QString &file, qint64 &id);

    /// for Functions
    QString stringForFunctionCompression(qint64 id) const;
    /// @p id if it is -1, an uncompressed string is assumed and it will be compressed internally
    void addCompressedFunction(const QString &function, qint64 &id);

private:
    class Private;
    Private *d;
};

} // namespace Callgrind
} // namespace Valgrind
