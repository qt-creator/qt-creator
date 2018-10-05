/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "idevice.h"
#include <projectexplorer/projectexplorer_export.h>

#include <QIcon>
#include <QVariantMap>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT IDeviceFactory : public QObject
{
    Q_OBJECT

public:
    ~IDeviceFactory() override;
    static const QList<IDeviceFactory *> allDeviceFactories();

    Core::Id deviceType() const { return m_deviceType; }
    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    bool canCreate() const;
    IDevice::Ptr construct() const;

    virtual IDevice::Ptr create() const { return IDevice::Ptr(); }

    virtual bool canRestore(const QVariantMap &) const { return true; }

    static IDeviceFactory *find(Core::Id type);

protected:
    explicit IDeviceFactory(Core::Id deviceType);

    void setDisplayName(const QString &displayName);
    void setIcon(const QIcon &icon);
    void setCombinedIcon(const QString &small, const QString &large);
    void setCanCreate(bool canCreate);
    void setConstructionFunction(const std::function<IDevice::Ptr ()> &constructor);

private:
    const Core::Id m_deviceType;
    QString m_displayName;
    QIcon m_icon;
    bool m_canCreate = false;
    std::function<IDevice::Ptr()> m_constructor;
};

} // namespace ProjectExplorer
