/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef THREADDATA_H
#define THREADDATA_H

#include <QtCore/QList>

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
    inline ThreadData(quint64 threadid = 0)
        : id (threadid)
        , frameLevel(-1)
        , address (0)
        , lineNumber(-1)
    {
    }

    enum {
        IdColumn,
        AddressColumn,
        FunctionColumn,
        FileColumn,
        LineColumn,
        StateColumn,
        NameColumn,
        CoreColumn,
        ColumnCount = CoreColumn
    };

    // Permanent data.
    quint64 id;
    QString targetId;
    QString core;

    // State information when stopped
    inline void notifyRunning() // Clear state information
    {
        address = 0;
        function.clear();
        fileName.clear();
        frameLevel = -1;
        state.clear();
        lineNumber = -1;
    }

    qint32  frameLevel;
    quint64 address;
    QString function;
    QString fileName;
    QString state;
    qint32  lineNumber;
    QString name;
};

typedef QVector<ThreadData> Threads;

} // namespace Internal
} // namespace Debugger

#endif // THREADDATA_H
