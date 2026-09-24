// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <QObject>

namespace Remote::Internal {

class RemoteRunTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testStandardInput();
    void cleanupTestCase();

private:
    ProjectExplorer::IDeviceConstPtr m_device;
};

} // Remote::Internal
