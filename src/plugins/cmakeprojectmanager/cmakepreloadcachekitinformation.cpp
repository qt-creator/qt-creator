/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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

#include "cmakepreloadcachekitinformation.h"
#include "cmakepreloadcachekitconfigwidget.h"
#include "cmaketoolmanager.h"
#include "cmaketool.h"

#include <utils/qtcassert.h>
#include <projectexplorer/task.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QDebug>

using namespace ProjectExplorer;

namespace CMakeProjectManager {

CMakePreloadCacheKitInformation::CMakePreloadCacheKitInformation()
{
    setObjectName(QLatin1String("CMakePreloadCacheKitInformation"));
    setId(CMakePreloadCacheKitInformation::id());
    setPriority(18000);
}

Core::Id CMakePreloadCacheKitInformation::id()
{
    return "CMakeProjectManager.CMakePreloadCacheKitInformation";
}

QVariant CMakePreloadCacheKitInformation::defaultValue(const Kit *) const
{
    return QString();
}

QList<Task> CMakePreloadCacheKitInformation::validate(const Kit *k) const
{
    Q_UNUSED(k);
    return QList<Task>();
}

void CMakePreloadCacheKitInformation::setup(Kit *k)
{
    Q_UNUSED(k);
}

void CMakePreloadCacheKitInformation::fix(Kit *k)
{
    Q_UNUSED(k);
}

KitInformation::ItemList CMakePreloadCacheKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("CMake Preload"), k->value(id()).toString());
}

KitConfigWidget *CMakePreloadCacheKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::CMakePreloadCacheKitConfigWidget(k, this);
}

void CMakePreloadCacheKitInformation::setPreloadCacheFile(Kit *k, const Utils::FileName &preload)
{
    if (!k)
        return;
    k->setValue(CMakePreloadCacheKitInformation::id(), preload.toString());
}

Utils::FileName CMakePreloadCacheKitInformation::preloadCacheFile(const Kit *k)
{
    if (!k)
        return Utils::FileName();
    return Utils::FileName::fromString(k->value(CMakePreloadCacheKitInformation::id()).toString());
}

} // namespace CMakeProjectManager
