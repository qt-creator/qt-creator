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

#include <QObject>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class IDeviceWidget;


class PROJECTEXPLORER_EXPORT IDeviceFactory : public QObject
{
    Q_OBJECT

public:
    ~IDeviceFactory() override;
    static const QList<IDeviceFactory *> allDeviceFactories();

    virtual QString displayName() const = 0;
    virtual QIcon icon() const = 0;

    virtual bool canCreate() const;
    virtual IDevice::Ptr create() const = 0;

    virtual bool canRestore(const QVariantMap &map) const = 0;
    virtual IDevice::Ptr restore(const QVariantMap &map) const = 0;

    static IDeviceFactory *find(Core::Id type);
    Core::Id deviceType() const { return m_deviceType; }

protected:
    explicit IDeviceFactory(Core::Id deviceType);

private:
    const Core::Id m_deviceType;
};

} // namespace ProjectExplorer
