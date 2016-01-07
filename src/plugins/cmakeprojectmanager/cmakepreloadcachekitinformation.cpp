/****************************************************************************
**
** Copyright (C) 2015 Canonical Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    setPriority(19000);
}

Core::Id CMakePreloadCacheKitInformation::id()
{
    return "CMakeProjectManager.CMakePreloadCacheKitInformation";
}

QVariant CMakePreloadCacheKitInformation::defaultValue(Kit *) const
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

void CMakePreloadCacheKitInformation::setPreloadCacheFile(Kit *k, const QString &preload)
{
    if (!k)
        return;
    k->setValue(CMakePreloadCacheKitInformation::id(), preload);
}

QString CMakePreloadCacheKitInformation::preloadCacheFile(const Kit *k)
{
    if (!k)
        return QString();
    return k->value(CMakePreloadCacheKitInformation::id()).toString();
}

} // namespace CMakeProjectManager
