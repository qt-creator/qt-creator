// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

namespace BareMetal::Internal {

class BareMetalDevice final : public ProjectExplorer::IDevice
{
public:
    using Ptr = std::shared_ptr<BareMetalDevice>;
    using ConstPtr = std::shared_ptr<const BareMetalDevice>;

    static Ptr create() { return Ptr(new BareMetalDevice); }
    ~BareMetalDevice() final;

    static QString defaultDisplayName();

    ProjectExplorer::IDeviceWidget *createWidget() final;

    QString debugServerProviderId() const;
    void setDebugServerProviderId(const QString &id);
    void unregisterDebugServerProvider(class IDebugServerProvider *provider);

private:
    void fromMap(const Utils::Store &map) final;

    BareMetalDevice();
    Utils::StringAspect m_debugServerProviderId{this};
};

void setupBareMetalDevice();

} // BareMetal::Internal
