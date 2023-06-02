// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

#include <QHash>

namespace Autotest {

class ITestFramework;

namespace Internal {
namespace QuickTestUtils {

bool isQuickTestMacro(const QByteArray &macro);
QHash<Utils::FilePath, Utils::FilePath> proFilesForQmlFiles(ITestFramework *framework,
                                                            const QSet<Utils::FilePath> &files);

} // namespace QuickTestUtils
} // namespace Internal
} // namespace Autotest
