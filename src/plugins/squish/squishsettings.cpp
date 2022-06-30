/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishsettings.h"

#include <QSettings>

namespace Squish {
namespace Internal {

static const char group[] = "Squish";
static const char squishPathKey[] = "SquishPath";
static const char licensePathKey[] = "LicensePath";
static const char localKey[] = "Local";
static const char serverHostKey[] = "ServerHost";
static const char serverPortKey[] = "ServerPort";
static const char verboseKey[] = "Verbose";

void SquishSettings::toSettings(QSettings *s) const
{
    s->beginGroup(group);
    s->setValue(squishPathKey, squishPath.toString());
    s->setValue(licensePathKey, licensePath.toString());
    s->setValue(localKey, local);
    s->setValue(serverHostKey, serverHost);
    s->setValue(serverPortKey, serverPort);
    s->setValue(verboseKey, verbose);
    s->endGroup();
}

void SquishSettings::fromSettings(QSettings *s)
{
    s->beginGroup(group);
    squishPath = Utils::FilePath::fromVariant(s->value(squishPathKey));
    licensePath = Utils::FilePath::fromVariant(s->value(licensePathKey));
    local = s->value(localKey, true).toBool();
    serverHost = s->value(serverHostKey, "localhost").toString();
    serverPort = s->value(serverPortKey, 9999).toUInt();
    verbose = s->value(verboseKey, false).toBool();
    s->endGroup();
}

bool SquishSettings::operator==(const SquishSettings &other) const
{
    return local == other.local && verbose == other.verbose && serverPort == other.serverPort
           && squishPath == other.squishPath && licensePath == other.licensePath
           && serverHost == other.serverHost;
}

bool SquishSettings::operator!=(const SquishSettings &other) const
{
    return !(*this == other);
}

} // namespace Internal
} // namespace Squish
