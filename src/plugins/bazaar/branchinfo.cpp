// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "branchinfo.h"

namespace Bazaar::Internal {

BranchInfo::BranchInfo(const QString &branchLoc, bool isBound) : branchLocation(branchLoc),
    isBoundToBranch(isBound)
{ }

} // Bazaar::Internal
