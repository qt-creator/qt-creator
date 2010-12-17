/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CDBSTACKTRACECONTEXT_H
#define CDBSTACKTRACECONTEXT_H

#include "stackhandler.h"
#include "stacktracecontext.h"
#include "threadshandler.h"

#include "cdbcom.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace CdbCore {
    struct ComInterfaces;
}

namespace Debugger {
namespace Internal {

class CdbSymbolGroupContext;
class CdbStackFrameContext;
class CdbDumperHelper;
struct ThreadData;

/* CdbStackTraceContext: Bridges  CdbCore data structures and
 * Debugger structures and handles CdbSymbolGroupContext's. */

class CdbStackTraceContext : public CdbCore::StackTraceContext
{
    Q_DISABLE_COPY(CdbStackTraceContext)

    explicit CdbStackTraceContext(const QSharedPointer<CdbDumperHelper> &dumper);

public:
    static CdbStackTraceContext *create(const QSharedPointer<CdbDumperHelper> &dumper,
                                        QString *errorMessage);

    CdbSymbolGroupContext *cdbSymbolGroupContextAt(int index, QString *errorMessage);

    QList<StackFrame> stackFrames() const;

    // get threads in stopped state
    static bool getThreads(const CdbCore::ComInterfaces &cif,
                           bool stopped,
                           Threads *threads,
                           ULONG *currentThreadId,
                           QString *errorMessage);

protected:
    virtual CdbCore::SymbolGroupContext *createSymbolGroup(const CdbCore::ComInterfaces &cif,
                                                           int index,
                                                           const QString &prefix,
                                                           CIDebugSymbolGroup *comSymbolGroup,
                                                           QString *errorMessage);

private:
    const QSharedPointer<CdbDumperHelper> m_dumper;
};

} // namespace Internal
} // namespace Debugger

#endif // CDBSTACKTRACECONTEXT_H
