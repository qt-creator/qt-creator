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

#ifndef SHELLCOMMANDPAGE_H
#define SHELLCOMMANDPAGE_H

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

    explicit ShellCommandPage(QWidget *parent = 0);
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
    void reportStdOut(const QString &text);
    void reportStdErr(const QString &text);

    QPlainTextEdit *m_logPlainTextEdit;
    OutputFormatter *m_formatter;
    QLabel *m_statusLabel;

    ShellCommand *m_command;
    QString m_startedStatus;
    bool m_overwriteOutput;

    State m_state;
};

} // namespace Utils

#endif // SHELLCOMMANDPAGE_H
