// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Autotest {
namespace GTest {
namespace Constants {

const char FRAMEWORK_ID[]                = "AutoTest.Framework.GTest";
const char FRAMEWORK_SETTINGS_CATEGORY[] = QT_TRANSLATE_NOOP("QtC::Autotest", "Google Test");
const unsigned FRAMEWORK_PRIORITY        = 10;
const char DEFAULT_FILTER[]              = "*.*";

enum GroupMode
{
    None,
    Directory,
    GTestFilter
};

} // namespace Constants
} // namespace GTest
} // namespace AutoTest
