// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/processinterface.h>

namespace Valgrind::Internal {

std::optional<Utils::ProcessRunData> runStartRemoteDialog();

} // Valgrind::Internal
