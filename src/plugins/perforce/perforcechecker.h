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

#include <QObject>
#include <QProcess>

namespace Perforce {
namespace Internal {

// Perforce checker:  Calls perforce asynchronously to do
// a check of the configuration and emits signals with the top level or
// an error message.

class PerforceChecker : public QObject
{
    Q_OBJECT
public:
    explicit PerforceChecker(QObject *parent = nullptr);
    ~PerforceChecker();

    void start(const QString &binary,
               const QString &workingDirectory,
               const QStringList &basicArgs = QStringList(),
               int timeoutMS = -1);

    bool isRunning() const;

    bool waitForFinished();

    bool useOverideCursor() const;
    void setUseOverideCursor(bool v);

signals:
    void succeeded(const QString &repositoryRoot);
    void failed(const QString &errorMessage);

private:
    void slotError(QProcess::ProcessError error);
    void slotFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotTimeOut();

    void emitFailed(const QString &);
    void emitSucceeded(const QString &);
    void parseOutput(const QString &);
    inline void resetOverrideCursor();

    QProcess m_process;
    QString m_binary;
    int m_timeOutMS = -1;
    bool m_timedOut = false;
    bool m_useOverideCursor = false;
    bool m_isOverrideCursor = false;
};

} // namespace Internal
} // namespace Perforce
