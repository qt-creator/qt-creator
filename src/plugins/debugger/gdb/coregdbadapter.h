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

#include "gdbengine.h"

#include <QFile>

namespace Debugger {
namespace Internal {

class GdbCoreEngine : public GdbEngine
{
    Q_OBJECT

public:
    explicit GdbCoreEngine(const DebuggerRunParameters &runParameters);
    ~GdbCoreEngine() override;

    struct CoreInfo
    {
        QString rawStringFromCore;
        QString foundExecutableName; // empty if no corresponding exec could be found
        bool isCore = false;
    };
    static CoreInfo readExecutableNameFromCore(const ProjectExplorer::StandardRunnable &debugger,
                                               const QString &coreFile);

private:
    void setupEngine() override;
    void setupInferior() override;
    void runEngine() override;
    void interruptInferior() override;
    void shutdownEngine() override;

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
