/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
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

#include <remotelinux/linuxdevicetester.h>
#include <utils/qtcprocess.h>

#include <QStringList>

namespace Qnx {
namespace Internal {

class QnxDeviceTester : public ProjectExplorer::DeviceTester
{
    Q_OBJECT

public:
    explicit QnxDeviceTester(QObject *parent = nullptr);

    void testDevice(const ProjectExplorer::IDevice::Ptr &deviceConfiguration) override;
    void stopTest() override;

private:
    enum State {
        Inactive,
        GenericTest,
        VarRunTest,
        CommandsTest
    };

    void handleGenericTestFinished(ProjectExplorer::DeviceTester::TestResult result);
    void handleProcessDone();
    void handleVarRunDone();
    void handleCommandDone();

    void testNextCommand();
    void setFinished(ProjectExplorer::DeviceTester::TestResult result);

    QStringList versionSpecificCommandsToTest(int versionNumber) const;

    RemoteLinux::GenericLinuxDeviceTester *m_genericTester;
    ProjectExplorer::IDevice::ConstPtr m_deviceConfiguration;
    ProjectExplorer::DeviceTester::TestResult m_result = TestSuccess;
    State m_state = Inactive;

    int m_currentCommandIndex = 0;
    QStringList m_commandsToTest;
    Utils::QtcProcess m_process;
};

} // namespace Internal
} // namespace Qnx
