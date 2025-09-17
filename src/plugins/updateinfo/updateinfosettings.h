// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace UpdateInfo::Internal {

enum CheckUpdateInterval {
    DailyCheck,
    WeeklyCheck,
    MonthlyCheck
};

class UpdateInfoSettings : public Utils::AspectContainer
{
public:
    bool automaticCheck = true;
    CheckUpdateInterval checkInterval = WeeklyCheck;
    bool checkForQtVersions = true;

    QDate lastCheckDate() const { return m_lastCheckDate; }

// private:
    QDate m_lastCheckDate;
};

UpdateInfoSettings &settings();

void setupSettings(class UpdateInfoPlugin *plugin);

} // UpdateInfo::Internal
