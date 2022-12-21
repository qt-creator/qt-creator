// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

// Run the settings dialog and wait for it to finish.
// Returns if the changes have been applied.
bool executeSettingsDialog(QWidget *parent, Utils::Id initialPage);

} // namespace Internal
} // namespace Core
