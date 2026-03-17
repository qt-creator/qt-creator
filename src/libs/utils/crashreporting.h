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
QTCREATOR_UTILS_EXPORT void setCrashReportingEnabled(bool enable);
QTCREATOR_UTILS_EXPORT QString breakpadInformation();
QTCREATOR_UTILS_EXPORT Key crashReportSettingsKey();

} // namespace Utils
