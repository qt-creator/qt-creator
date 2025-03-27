// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Utils { class Perspective; }

namespace Valgrind::Internal {

void setupExternalAnalyzer(QAction *action, Utils::Perspective *perspective, Utils::Id runMode);

} // Valgrind::Internal
