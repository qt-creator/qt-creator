// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

/* Helper functions to extract information from PE Win32 executable
 * files (cf dumpbin utility). */
namespace Debugger {
namespace Internal {

// Return a list of Program-Database (*.pdb) files a PE executable refers to. */
bool getPDBFiles(const QString &peExecutableFileName, QStringList *rc, QString *errorMessage);

} // namespace Internal
} // namespace Debugger
