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
namespace Internal {

class TestOutputReader : public QObject
{
    Q_OBJECT
public:
    TestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                     QProcess *testApplication, const QString &buildDirectory);

    virtual void processOutput(const QByteArray &outputLine) = 0;
    virtual void processStdError(const QByteArray &output);
    bool hadValidOutput() const { return m_hadValidOutput; }

signals:
    void newOutputAvailable(const QByteArray &output);
protected:
    void reportResult(const TestResultPtr &result);
    QFutureInterface<TestResultPtr> m_futureInterface;
    QProcess *m_testApplication;  // not owned
    QString m_buildDir;
private:
    bool m_hadValidOutput = false;
};

} // namespace Internal
} // namespace Autotest
