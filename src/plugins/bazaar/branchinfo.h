// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Bazaar::Internal {

class BranchInfo
{
public:
    BranchInfo(const QString &branchLoc, bool isBound);
    const QString branchLocation;
    const bool isBoundToBranch;
};

} // Bazaar::Internal
