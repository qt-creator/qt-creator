// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "globalsettings.h"

#include "cocoinstallation.h"
#include "cocopluginconstants.h"

#include <coreplugin/icore.h>
#include <utils/filepath.h>

#include <QProcess>
#include <QSettings>

namespace Coco::Internal {
namespace GlobalSettings {

static const char DIRECTORY[] = "CocoDirectory";

void read()
{
    CocoInstallation coco;
    bool directoryInSettings = false;

    Utils::QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::COCO_SETTINGS_GROUP);
    const QStringList keys = s->allKeys();
    for (const QString &keyString : keys) {
        Utils::Key key(keyString.toLatin1());
        if (key == DIRECTORY) {
            coco.setDirectory(Utils::FilePath::fromUserInput(s->value(key).toString()));
            directoryInSettings = true;
        } else
            s->remove(key);
    }
    s->endGroup();

    if (!directoryInSettings)
        coco.findDefaultDirectory();

    GlobalSettings::save();
}

void save()
{
    Utils::QtcSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::COCO_SETTINGS_GROUP);
    s->setValue(DIRECTORY, CocoInstallation().directory().toUserOutput());
    s->endGroup();
}

} // namespace GlobalSettings
} // namespace Coco::Internal
