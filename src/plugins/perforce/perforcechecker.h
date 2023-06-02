// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/process.h>

namespace Perforce::Internal {

// Perforce checker:  Calls perforce asynchronously to do
// a check of the configuration and emits signals with the top level or
// an error message.

class PerforceChecker : public QObject
{
    Q_OBJECT
public:
    explicit PerforceChecker(QObject *parent = nullptr);
    ~PerforceChecker() override;

    void start(const Utils::FilePath &binary,
               const Utils::FilePath &workingDirectory,
               const QStringList &basicArgs = {},
               int timeoutMS = -1);

    bool isRunning() const;

    bool waitForFinished();

    bool useOverideCursor() const;
    void setUseOverideCursor(bool v);

signals:
    void succeeded(const Utils::FilePath &repositoryRoot);
    void failed(const QString &errorMessage);

private:
    void slotDone();
    void slotTimeOut();

    void emitFailed(const QString &);
    void emitSucceeded(const QString &);
    void parseOutput(const QString &);
    inline void resetOverrideCursor();

    Utils::Process m_process;
    Utils::FilePath m_binary;
    int m_timeOutMS = -1;
    bool m_timedOut = false;
    bool m_useOverideCursor = false;
    bool m_isOverrideCursor = false;
};

} // Perforce::Internal
