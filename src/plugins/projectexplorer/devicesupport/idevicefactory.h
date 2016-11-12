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

    virtual QString displayNameForId(Core::Id type) const = 0;

    virtual QList<Core::Id> availableCreationIds() const = 0;

    virtual QIcon iconForId(Core::Id type) const = 0;

    virtual bool canCreate() const;
    virtual IDevice::Ptr create(Core::Id id) const = 0;

    virtual bool canRestore(const QVariantMap &map) const = 0;
    virtual IDevice::Ptr restore(const QVariantMap &map) const = 0;

    static IDeviceFactory *find(Core::Id type);

protected:
    explicit IDeviceFactory(QObject *parent = 0);
};

} // namespace ProjectExplorer
