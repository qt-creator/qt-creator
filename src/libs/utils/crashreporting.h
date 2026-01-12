// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "infobar.h"

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Layouting {
class Grid;
}

namespace Utils {

class AspectContainer;
class BoolAspect;

QTCREATOR_UTILS_EXPORT bool isCrashReportingAvailable();
QTCREATOR_UTILS_EXPORT bool setUpCrashReporting();
QTCREATOR_UTILS_EXPORT void addCrashReportSetting(
    AspectContainer *container, const std::function<void(QString)> &askForRestart);
QTCREATOR_UTILS_EXPORT void addCrashReportSettingsUi(QWidget *parent, Layouting::Grid *grid);
QTCREATOR_UTILS_EXPORT void warnAboutCrashReporting(
    InfoBar *infoBar,
    const QString &optionsButtonText,
    const Utils::InfoBarEntry::CallBack &optionsButtonCallback);
QTCREATOR_UTILS_EXPORT void shutdownCrashReporting();

} // namespace Utils
