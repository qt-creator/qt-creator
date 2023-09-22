// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Utils { class QtcSettings; }

namespace ModelEditor {
namespace Internal {

class SettingsController : public QObject
{
    Q_OBJECT

public:
    SettingsController();

signals:
    void resetSettings();
    void saveSettings(Utils::QtcSettings *settings);
    void loadSettings(Utils::QtcSettings *settings);

public:
    void reset();
    void save(Utils::QtcSettings *settings);
    void load(Utils::QtcSettings *settings);
};

} // namespace Internal
} // namespace ModelEditor
