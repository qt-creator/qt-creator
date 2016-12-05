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

#include "utils_global.h"

#include "wizardpage.h"

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLabel;
QT_END_NAMESPACE

namespace Utils {
class OutputFormatter;
class ShellCommand;

class QTCREATOR_UTILS_EXPORT ShellCommandPage : public WizardPage
{
    Q_OBJECT

public:
    enum State { Idle, Running, Failed, Succeeded };

    explicit ShellCommandPage(QWidget *parent = nullptr);
    ~ShellCommandPage();

    void setStartedStatus(const QString &startedStatus);
    void start(ShellCommand *command);

    virtual bool isComplete() const;
    bool isRunning() const{ return m_state == Running; }

    void terminate();

    bool handleReject();

signals:
    void finished(bool success);

private:
    void slotFinished(bool ok, int exitCode, const QVariant &cookie);

    QPlainTextEdit *m_logPlainTextEdit;
    OutputFormatter *m_formatter;
    QLabel *m_statusLabel;

    ShellCommand *m_command = nullptr;
    QString m_startedStatus;
    bool m_overwriteOutput = false;

    State m_state = Idle;
};

} // namespace Utils
