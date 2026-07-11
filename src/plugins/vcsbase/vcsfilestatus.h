// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <coreplugin/vcsfilestate.h>

#include <QList>
#include <QString>

namespace VcsBase {

class VCSBASE_EXPORT VcsFileStatus
{
public:
    enum class Section : quint8 {
        Changed,    //!< Unstaged / plain change. VCSs without a staging area use only this.
        Staged,     //!< Staged for commit (git).
        Conflicted, //!< Merge conflicts.
    };

    QString relativePath; // Relative to the repository top level, '/' separators.
    Core::VcsFileState state = Core::VcsFileState::Unknown;
    Section section = Section::Changed;

    friend bool operator==(const VcsFileStatus &, const VcsFileStatus &) = default;
};

using VcsFileStatusList = QList<VcsFileStatus>;

} // namespace VcsBase
