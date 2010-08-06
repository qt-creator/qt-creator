/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtSCriptTools module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSCRIPTDEBUGGERAGENT_P_H
#define QSCRIPTDEBUGGERAGENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtScript/qscriptengineagent.h>
#include <QtScript/QScriptValue>
#include <QtCore/QEventLoop>
#include <QtCore/QSet>
#include <private/qdeclarativedebugservice_p.h>
#include <QtCore/QStringList>

#include "qmljsdebugger_global.h"

QT_BEGIN_NAMESPACE

class JSAgentWatchData;
class QScriptContext;

class QMLJSDEBUGGER_EXPORT JSDebuggerAgent : public QDeclarativeDebugService , public QScriptEngineAgent
{ Q_OBJECT
public:
    JSDebuggerAgent(QScriptEngine *engine);
    ~JSDebuggerAgent();

    // reimplemented
    void scriptLoad(qint64 id, const QString &program,
                    const QString &fileName, int baseLineNumber);
    void scriptUnload(qint64 id);

    void contextPush();
    void contextPop();

    void functionEntry(qint64 scriptId);
    void functionExit(qint64 scriptId,
                      const QScriptValue &returnValue);

    void positionChange(qint64 scriptId,
                        int lineNumber, int columnNumber);

    void exceptionThrow(qint64 scriptId,
                        const QScriptValue &exception,
                        bool hasHandler);
    void exceptionCatch(qint64 scriptId,
                        const QScriptValue &exception);

    bool supportsExtension(Extension extension) const;
    QVariant extension(Extension extension,
                       const QVariant &argument = QVariant());

    void messageReceived(const QByteArray &);
    void enabledChanged(bool);

public slots:
//    void pauses();

private:
    class SetupExecEnv;
    friend class SetupExecEnv;

    enum State {
        NoState,
        SteppingIntoState,
        SteppingOverState,
        SteppingOutState,
        Stopped
    };
    State state;
    int stepDepth;
    int stepCount;

    void continueExec();
    void stopped(bool becauseOfException = false, const QScriptValue &exception = QScriptValue());


    void recordKnownObjects(const QList<JSAgentWatchData> &);
    QList<JSAgentWatchData> getLocals(QScriptContext *);

    QEventLoop loop;
    QHash <qint64, QString> filenames;
    QSet< QPair<QString, qint32> > breakpointList;
    QStringList watchExpressions;
    QSet<qint64> knownObjectIds;

    Q_DISABLE_COPY(JSDebuggerAgent)
};

QT_END_NAMESPACE

#endif
