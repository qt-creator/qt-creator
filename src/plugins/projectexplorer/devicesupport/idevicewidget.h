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

protected:
    explicit IDeviceWidget(const IDevicePtr &device) :
        m_device(device)
    { }

    IDevicePtr device() const { return m_device; }

private:
    IDevicePtr m_device;
};

} // namespace ProjectExplorer
