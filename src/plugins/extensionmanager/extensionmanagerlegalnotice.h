// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "extensionmanager_global.h"

#include <QString>

namespace ExtensionManager {

EXTENSIONMANAGER_EXPORT void setLegalNoticeVisible(bool show, const QString &text = {});

} // ExtensionManager
