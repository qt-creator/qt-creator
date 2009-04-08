/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CDBBREAKPOINTS_H
#define CDBBREAKPOINTS_H

#include <windows.h>
#include <inc/dbgeng.h>

#include <QtCore/QString>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class BreakHandler;
class BreakpointData;

/* CDB Break point data structure with utilities to
 * apply to engine and to retrieve them from the engine and comparison. */

struct CDBBreakPoint {
    CDBBreakPoint();
    CDBBreakPoint(const BreakpointData &bpd);

    int compare(const CDBBreakPoint& rhs) const;

    void clear();
    void clearExpressionData();

    QString expression() const;

    // Apply parameters
    bool apply(IDebugBreakpoint2 *ibp, QString *errorMessage) const;
    // Convenience to add to a IDebugControl4
    bool add(IDebugControl4* debugControl, QString *errorMessage) const;

    // Retrieve/parse breakpoints from the interfaces
    bool retrieve(IDebugBreakpoint2 *ibp, QString *errorMessage);
    bool parseExpression(const QString &expr);
    // Retrieve all breakpoints from the engine
    static bool getBreakPointCount(IDebugControl4* debugControl, ULONG *count, QString *errorMessage = 0);
    static bool getBreakPoints(IDebugControl4* debugControl, QList<CDBBreakPoint> *bps, QString *errorMessage);
    // Synchronize (halted) engine with BreakHandler.
    static bool synchronizeBreakPoints(IDebugControl4* ctl, BreakHandler *bh, QString *errorMessage);

    // Return a 'canonical' file (using '/' and capitalized drive letter)
    static QString canonicalSourceFile(const QString &f);

    QString fileName;       // short name of source file
    QString condition;      // condition associated with breakpoint
    unsigned long ignoreCount;    // ignore count associated with breakpoint
    int lineNumber;     // line in source file
    QString funcName;       // name of containing function
};

QDebug operator<<(QDebug, const CDBBreakPoint &bp);

inline bool operator==(const CDBBreakPoint& b1, const CDBBreakPoint& b2)
    { return b1.compare(b2) == 0; }
inline bool operator!=(const CDBBreakPoint& b1, const CDBBreakPoint& b2)
    { return b1.compare(b2) != 0; }
inline bool operator<(const CDBBreakPoint& b1, const CDBBreakPoint& b2)
    { return b1.compare(b2) <  0; }

}
}

#endif // CDBBREAKPOINTS_H
