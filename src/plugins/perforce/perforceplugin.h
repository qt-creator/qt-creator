// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Perforce::Internal {

// Map a perforce name "//xx" to its real name in the file system
QString fileNameFromPerforceName(const QString &perforceName, bool quiet, QString *errorMessage);

} // Perforce::Internal
