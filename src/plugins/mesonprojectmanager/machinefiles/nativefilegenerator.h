// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "kithelper/kitdata.h"

#include <projectexplorer/kit.h>

#include <QIODevice>

namespace MesonProjectManager {
namespace Internal {

class NativeFileGenerator
{
    NativeFileGenerator();

public:
    static void makeNativeFile(QIODevice *nativeFile, const KitData &kitData);
};

} // namespace Internal
} // namespace MesonProjectManager
