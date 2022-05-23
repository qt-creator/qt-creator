/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/fileutils.h>

namespace RemoteLinux {
namespace Internal {

class TestLinuxDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    TestLinuxDeviceFactory();
};

class FileSystemAccessTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testCreateRemoteFile_data();
    void testCreateRemoteFile();
    void testDirStatus();
    void testBytesAvailable();
    void testFileActions();
    void testFileTransfer_data();
    void testFileTransfer();

    void cleanupTestCase();

private:
    TestLinuxDeviceFactory m_testLinuxDeviceFactory;
    bool m_skippedAtWhole = false;
    ProjectExplorer::IDeviceConstPtr m_device;
};

} // Internal
} // RemoteLinux
