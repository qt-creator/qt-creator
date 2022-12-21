// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMap>
#include <QString>

namespace Debugger::Internal {

class DebuggerRunParameters;
using SourcePathMap = QMap<QString, QString>;

/* Merge settings for an installed Qt (unless another setting
 * is already in the map. */
SourcePathMap mergePlatformQtPath(const DebuggerRunParameters &sp, const SourcePathMap &in);

} // Debugger::Internal
