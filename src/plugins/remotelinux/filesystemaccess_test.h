// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefactory.h>
#include <utils/filepath.h>

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
    void testWorkingDirectory();
    void testDirStatus();
    void testBytesAvailable();
    void testFileActions();
    void testFileTransfer_data();
    void testFileTransfer();
    void testFileStreamer_data();
    void testFileStreamer();
    void testFileStreamerManager_data();
    void testFileStreamerManager();

    void cleanupTestCase();

private:
    TestLinuxDeviceFactory m_testLinuxDeviceFactory;
    bool m_skippedAtWhole = false;
    ProjectExplorer::IDeviceConstPtr m_device;
    Utils::FilePath m_localStreamerDir;
    Utils::FilePath m_remoteStreamerDir;
    Utils::FilePath m_localSourceDir;
    Utils::FilePath m_remoteSourceDir;
    Utils::FilePath m_localDestDir;
    Utils::FilePath m_remoteDestDir;
    Utils::FilePath m_localLocalDestDir;
    Utils::FilePath m_localRemoteDestDir;
    Utils::FilePath m_remoteLocalDestDir;
    Utils::FilePath m_remoteRemoteDestDir;
};

} // Internal
} // RemoteLinux
