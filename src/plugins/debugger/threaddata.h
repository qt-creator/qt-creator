/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef THREADDATA_H
#define THREADDATA_H

#include <QVector>
#include <QString>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// ThreadData
//
////////////////////////////////////////////////////////////////////////

/*! A structure containing information about a single thread */
struct ThreadData
{
    ThreadData(quint64 threadid = 0)
        : id(threadid), frameLevel(-1), address (0), lineNumber(-1)
    {}

    enum {
        IdColumn,
        AddressColumn,
        FunctionColumn,
        FileColumn,
        LineColumn,
        StateColumn,
        NameColumn,
        TargetIdColumn,
        CoreColumn,
        ComboNameColumn,
        ColumnCount = CoreColumn
    };

    void notifyRunning() // Clear state information.
    {
        address = 0;
        function.clear();
        fileName.clear();
        frameLevel = -1;
        state.clear();
        lineNumber = -1;
    }

    // Permanent data.
    quint64 id;
    QString targetId;
    QString core;

    // State information when stopped.
    qint32  frameLevel;
    quint64 address;
    QString function;
    QString module;
    QString fileName;
    QString state;
    qint32  lineNumber;
    QString name;
};

typedef QVector<ThreadData> Threads;

} // namespace Internal
} // namespace Debugger

#endif // THREADDATA_H
