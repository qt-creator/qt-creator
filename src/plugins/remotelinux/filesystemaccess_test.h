// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefactory.h>

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
