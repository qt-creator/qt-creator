// Copyright (C) 2025 Andre Hartmann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Core {

enum class VcsFileState : quint8 {
    Unknown = 0x00,
    Untracked,
    Added,
    Modified,
    Deleted,
    Renamed,
    Unmerged,
};

}
