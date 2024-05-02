// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

namespace ProjectExplorer {

void setupWorkspaceProject(QObject *guard);

} // namespace ProjectExplorer
