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

#include <QString>
#include <QVector>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// ThreadId
//
////////////////////////////////////////////////////////////////////////

/*! A typesafe identifier. */
class ThreadId
{
public:
    ThreadId() : m_id(-1) {}
    explicit ThreadId(qint64 id) : m_id(id) {}

    bool isValid() const { return m_id != -1; }
    qint64 raw() const { return m_id; }
    bool operator==(const ThreadId other) const { return m_id == other.m_id; }
    bool operator!=(const ThreadId other) const { return m_id != other.m_id; }

private:
    qint64 m_id;
};

////////////////////////////////////////////////////////////////////////
//
// ThreadData
//
////////////////////////////////////////////////////////////////////////

/*! A structure containing information about a single thread. */
struct ThreadData
{
    ThreadData()
    {
        frameLevel = -1;
        lineNumber = -1;
        address = 0;
        stopped = true;
    }

    enum {
        IdColumn,
        AddressColumn,
        FunctionColumn,
        FileColumn,
        LineColumn,
        StateColumn,
        NameColumn,
        TargetIdColumn,
        DetailsColumn,
        CoreColumn,
        ComboNameColumn,
        ColumnCount = CoreColumn,

        IdRole = Qt::UserRole
    };

    // Permanent data.
    ThreadId id;
    QString groupId;
    QString targetId;
    QString core;
    bool stopped;

    // State information when stopped.
    qint32  frameLevel;
    qint32  lineNumber;
    quint64 address;
    QString function;
    QString module;
    QString fileName;
    QString details;
    QString state;
    QString name;
};

typedef QVector<ThreadData> Threads;

} // namespace Internal
} // namespace Debugger
