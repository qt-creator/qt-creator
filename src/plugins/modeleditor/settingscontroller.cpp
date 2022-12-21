// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingscontroller.h"

#include "modeleditor_constants.h"

#include <QSettings>

namespace ModelEditor {
namespace Internal {

SettingsController::SettingsController()
{
}

void SettingsController::reset()
{
    emit resetSettings();
}

void SettingsController::save(QSettings *settings)
{
    settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP));
    emit saveSettings(settings);
    settings->endGroup();
    settings->sync();
}

void SettingsController::load(QSettings *settings)
{
    settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP));
    emit loadSettings(settings);
    settings->endGroup();
}

} // namespace Internal
} // namespace ModelEditor
