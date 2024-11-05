// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/qtversionmanager.h>

#include <solutions/tasking/tasktree.h>

#include <utils/filepath.h>

#include <QStringList>
#include <QVersionNumber>

using namespace Tasking;
using namespace Utils;

namespace ProjectExplorer::Internal {

class WindowsConfigurations : public QObject
{
    Q_OBJECT

public:
    static void applyConfig();
    static WindowsConfigurations *instance();

    static FilePath downloadLocation();
    static void setDownloadLocation(const FilePath &downloadLocation);

    static FilePath nugetLocation();
    static void setNugetLocation(const FilePath &nugetLocation);

    static FilePath windowsAppSdkLocation();
    static void setWindowsAppSdkLocation(const FilePath &windowsAppSdkLocation);

signals:
    void aboutToUpdate();
    void updated();

private:
    friend void setupWindowsConfigurations();
    WindowsConfigurations();

    void load();
    void save();
};

void setupWindowsConfigurations();

} // namespace ProjectExplorer::Internal
