// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/hostosinfo.h>
#include <utils/smallstring.h>
#include <utils/temporarydirectory.h>

inline
bool operator==(const QString &first, const char *second)
{
    return first == QString::fromUtf8(second, int(std::strlen(second)));
}

namespace UnitTest {

inline ::Utils::PathString temporaryDirPath()
{
    return ::Utils::PathString::fromQString(Utils::TemporaryDirectory::masterDirectoryPath());
}

} // namespace UnitTest
