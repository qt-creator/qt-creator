// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QVersionNumber>

namespace WebAssembly::Internal {

const QVersionNumber &minimumSupportedEmSdkVersion();
void registerToolChains();
bool areToolChainsRegistered();

void setupWebAssemblyToolchain();

} // WebAssembly::Internal
