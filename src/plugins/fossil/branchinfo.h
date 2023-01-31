// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Fossil {
namespace Internal {

class BranchInfo
{
public:
    enum BranchFlag {
        Current = 0x01,
        Closed = 0x02,
        Private = 0x04
    };
    Q_DECLARE_FLAGS(BranchFlags, BranchFlag)

    bool isCurrent() const { return flags.testFlag(Current); }

    QString name;
    BranchFlags flags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(BranchInfo::BranchFlags)

} // namespace Internal
} // namespace Fossil
