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

#include "testresult.h"

#include <QFutureInterface>
#include <QObject>
#include <QProcess>
#include <QString>

namespace Autotest {

class TestOutputReader : public QObject
{
    Q_OBJECT
public:
    TestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                     QProcess *testApplication, const QString &buildDirectory);

    void processStdOutput(const QByteArray &outputLine);
    virtual void processStdError(const QByteArray &outputLine);
    void reportCrash();
    void createAndReportResult(const QString &message, ResultType type);
    bool hadValidOutput() const { return m_hadValidOutput; }
    int disabledTests() const { return m_disabled; }
    bool hasSummary() const { return !m_summary.isEmpty(); }
    QHash<ResultType, int> summary() const { return m_summary; }
    void setId(const QString &id) { m_id = id; }
    QString id() const { return m_id; }

    void resetCommandlineColor();
signals:
    void newOutputLineAvailable(const QByteArray &outputLine, OutputChannel channel);
protected:
    QString removeCommandlineColors(const QString &original);
    virtual void processOutputLine(const QByteArray &outputLine) = 0;
    virtual TestResultPtr createDefaultResult() const = 0;

    void reportResult(const TestResultPtr &result);
    QFutureInterface<TestResultPtr> m_futureInterface;
    QProcess *m_testApplication;  // not owned
    QString m_buildDir;
    QString m_id;
    QHash<ResultType, int> m_summary;
    int m_disabled = -1;
private:
    bool m_hadValidOutput = false;
};

} // namespace Autotest
