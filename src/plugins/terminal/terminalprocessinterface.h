// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "terminalpane.h"

#include <utils/processinterface.h>

namespace Terminal {

class TerminalProcessInterface : public Utils::ProcessInterface
{
    Q_OBJECT

public:
    TerminalProcessInterface(TerminalPane *terminalPane);

    /*
    // This should be emitted when being in Starting state only.
    // After emitting this signal the process enters Running state.
    void started(qint64 processId, qint64 applicationMainThreadId = 0);

    // This should be emitted when being in Running state only.
    void readyRead(const QByteArray &outputData, const QByteArray &errorData);

    // This should be emitted when being in Starting or Running state.
    // When being in Starting state, the resultData should set error to FailedToStart.
    // After emitting this signal the process enters NotRunning state.
    void done(const Utils::ProcessResultData &resultData);
*/
private:
    // It's being called only in Starting state. Just before this method is being called,
    // the process transitions from NotRunning into Starting state.
    void start() override;

    // It's being called only in Running state.
    qint64 write(const QByteArray &data) override;

    // It's being called in Starting or Running state.
    void sendControlSignal(Utils::ControlSignal controlSignal) override;

    //Utils::ProcessBlockingInterface *processBlockingInterface() const { return nullptr; }

private:
    TerminalPane *m_terminalPane;
};

} // namespace Terminal
