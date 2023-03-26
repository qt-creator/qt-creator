// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace ModelEditor {
namespace Internal {

class SettingsController :
        public QObject
{
    Q_OBJECT

public:
    SettingsController();

signals:
    void resetSettings();
    void saveSettings(QSettings *settings);
    void loadSettings(QSettings *settings);

public:
    void reset();
    void save(QSettings *settings);
    void load(QSettings *settings);
};

} // namespace Internal
} // namespace ModelEditor
