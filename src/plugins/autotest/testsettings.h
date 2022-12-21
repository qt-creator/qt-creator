// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QHash>

namespace Utils {
class Id;
}

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {

enum class RunAfterBuildMode
{
    None,
    All,
    Selected
};

struct TestSettings
{
    TestSettings();
    void toSettings(QSettings *s) const;
    void fromSettings(QSettings *s);

    int timeout;
    bool omitInternalMssg = true;
    bool omitRunConfigWarn = false;
    bool limitResultOutput = true;
    bool limitResultDescription = false;
    int resultDescriptionMaxSize = 10;
    bool autoScroll = true;
    bool processArgs = false;
    bool displayApplication = false;
    bool popupOnStart = true;
    bool popupOnFinish = true;
    bool popupOnFail = false;
    RunAfterBuildMode runAfterBuild = RunAfterBuildMode::None;
    QHash<Utils::Id, bool> frameworks;
    QHash<Utils::Id, bool> frameworksGrouping;
    QHash<Utils::Id, bool> tools;
};

} // namespace Internal
} // namespace Autotest
