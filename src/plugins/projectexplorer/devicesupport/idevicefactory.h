// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicefwd.h"

#include <projectexplorer/projectexplorer_export.h>

#include <utils/id.h>
#include <utils/store.h>

#include <QIcon>

namespace Utils { class FilePath; }

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT IDeviceFactory
{
public:
    virtual ~IDeviceFactory();
    static const QList<IDeviceFactory *> allDeviceFactories();

    Utils::Id deviceType() const { return m_deviceType; }
    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    bool canCreate() const;
    IDevicePtr construct() const;
    IDevicePtr create() const;
    bool quickCreationAllowed() const;

    virtual bool canRestore(const Utils::Store &) const { return true; }

    static IDeviceFactory *find(Utils::Id type);

protected:
    explicit IDeviceFactory(Utils::Id deviceType);
    IDeviceFactory(const IDeviceFactory &) = delete;
    IDeviceFactory &operator=(const IDeviceFactory &) = delete;

    void setDisplayName(const QString &displayName);
    void setIcon(const QIcon &icon);
    void setCombinedIcon(const Utils::FilePath &smallIcon, const Utils::FilePath &largeIcon);
    void setConstructionFunction(const std::function<IDevicePtr ()> &constructor);
    void setCreator(const std::function<IDevicePtr()> &creator);
    void setQuickCreationAllowed(bool on);

private:
    std::function<IDevicePtr()> m_creator;
    const Utils::Id m_deviceType;
    QString m_displayName;
    QIcon m_icon;
    std::function<IDevicePtr()> m_constructor;
    bool m_quickCreationAllowed = false;
};

} // namespace ProjectExplorer
