// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <projectexplorer/devicesupport/idevice.h>

#include <devcontainer/devcontainer.h>

#include <utils/qtcprocess.h>

namespace DevContainer {

class Device : public ProjectExplorer::IDevice
{
public:
    Device();
    ProjectExplorer::IDeviceWidget *createWidget() override;

    Utils::Result<> up(const Utils::FilePath &path, InstanceConfig instanceConfig);

    Utils::ProcessInterface *createProcessInterface() const override;

public: // FilePath stuff
    bool handlesFile(const Utils::FilePath &filePath) const override;
    Utils::FilePath rootPath() const override;

private:
    Utils::Process::ProcessInterfaceCreator m_processInterfaceCreator;
    InstanceConfig m_instanceConfig;
};

} // namespace DevContainer
