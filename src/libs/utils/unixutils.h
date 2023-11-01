// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Utils { class QtcSettings; }

namespace Utils::UnixUtils {

QTCREATOR_UTILS_EXPORT QString defaultFileBrowser();
QTCREATOR_UTILS_EXPORT QString fileBrowser(const QtcSettings *settings);
QTCREATOR_UTILS_EXPORT void setFileBrowser(QtcSettings *settings, const QString &term);
QTCREATOR_UTILS_EXPORT QString fileBrowserHelpText();
QTCREATOR_UTILS_EXPORT QString substituteFileBrowserParameters(const QString &command,
                                                               const QString &file);

} // Utils::UnixUtils
