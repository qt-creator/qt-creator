// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Bazaar {
namespace Internal {

class BranchInfo
{
public:
    BranchInfo(const QString &branchLoc, bool isBound);
    const QString branchLocation;
    const bool isBoundToBranch;
};

} // namespace Internal
} // namespace Bazaar
