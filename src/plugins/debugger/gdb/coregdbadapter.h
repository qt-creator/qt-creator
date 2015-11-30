/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_COREGDBADAPTER_H
#define DEBUGGER_COREGDBADAPTER_H

#include "gdbengine.h"

#include <QFile>

namespace Debugger {
namespace Internal {

class GdbCoreEngine : public GdbEngine
{
    Q_OBJECT

public:
    explicit GdbCoreEngine(const DebuggerRunParameters &runParameters);
    ~GdbCoreEngine();

    struct CoreInfo
    {
        QString rawStringFromCore;
        QString foundExecutableName; // empty if no corresponding exec could be found
        bool isCore = false;
    };
    static CoreInfo readExecutableNameFromCore(const QString &debuggerCmd, const QString &coreFile);

private:
    void setupEngine();
    void setupInferior();
    void runEngine();
    void interruptInferior();
    void shutdownEngine();

    void handleFileExecAndSymbols(const DebuggerResponse &response);
    void handleTargetCore(const DebuggerResponse &response);
    void handleRoundTrip(const DebuggerResponse &response);
    void unpackCoreIfNeeded();
    QString coreFileName() const;
    QString coreName() const;

    void continueSetupEngine();
    void writeCoreChunk();

private:
    QString m_executable;
    QString m_coreName;
    QString m_tempCoreName;
    QProcess *m_coreUnpackProcess;
    QFile m_tempCoreFile;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_COREDBADAPTER_H
