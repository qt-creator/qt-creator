// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Autotest {
namespace QtTest {
namespace Constants {

const char FRAMEWORK_ID[]                = "AutoTest.Framework.QtTest";
const char FRAMEWORK_SETTINGS_CATEGORY[] = QT_TRANSLATE_NOOP("QtC::Autotest", "Qt Test");
const unsigned FRAMEWORK_PRIORITY        = 1;

} // namespace Constants
} // namespace QtTest

namespace Internal {
enum class TestType
{
    QtTest,
    QuickTest
};

} // namespace Internal
} // namespace Autotest
