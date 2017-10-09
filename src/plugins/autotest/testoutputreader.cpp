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

#include "testoutputreader.h"
#include "testresult.h"
#include "testresultspane.h"

#include <QDebug>
#include <QProcess>

namespace Autotest {
namespace Internal {

TestOutputReader::TestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                   QProcess *testApplication, const QString &buildDirectory)
    : m_futureInterface(futureInterface)
    , m_testApplication(testApplication)
    , m_buildDir(buildDirectory)
{
    if (m_testApplication) {
        connect(m_testApplication, &QProcess::readyRead,
                this, [this] () {
            while (m_testApplication->canReadLine()) {
                QByteArray output = m_testApplication->readLine();
                output.chop(1); // remove the newline from the output
                if (output.endsWith('\r'))
                    output.chop(1);

                emit newOutputAvailable(output);
                processOutput(output);
            }
        });
        connect(m_testApplication, &QProcess::readyReadStandardError,
                this, [this] () {
            const QByteArray output = m_testApplication->readAllStandardError();
            emit newOutputAvailable(output);
            processStdError(output);
        });
    }
}

void TestOutputReader::processStdError(const QByteArray &output)
{
    qWarning() << "AutoTest.Run: Ignored plain output:" << output;
}

void TestOutputReader::reportResult(const TestResultPtr &result)
{
    m_futureInterface.reportResult(result);
    m_hadValidOutput = true;
}

} // namespace Internal
} // namespace Autotest
