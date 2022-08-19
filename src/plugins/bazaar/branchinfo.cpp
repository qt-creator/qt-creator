// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "branchinfo.h"

namespace Bazaar {
namespace Internal {

BranchInfo::BranchInfo(const QString &branchLoc, bool isBound) : branchLocation(branchLoc),
    isBoundToBranch(isBound)
{ }

} // namespace Internal
} // namespace Bazaar
