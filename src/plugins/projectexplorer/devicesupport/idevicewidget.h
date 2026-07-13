// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicefwd.h"
#include <projectexplorer/projectexplorer_export.h>

#include <QWidget>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT IDeviceWidget : public QWidget
{
    Q_OBJECT
public:

    virtual void updateDeviceFromUi() = 0;

    // Reports editable state that is not backed by the device's aspects (which
    // the device settings page already checks via IDevice::isDirty()).
    virtual bool isDirty() const { return false; }

    IDevicePtr device() const { return m_device; }

protected:
    explicit IDeviceWidget(const IDevicePtr &device) :
        m_device(device)
    { }


private:
    IDevicePtr m_device;
};

} // namespace ProjectExplorer
