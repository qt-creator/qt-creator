// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Cvs::Internal {

class CvsSettings : public VcsBase::VcsBaseSettings
{
public:
    CvsSettings();

    Utils::StringAspect cvsRoot{this};
    Utils::StringAspect diffOptions{this};
    Utils::BoolAspect diffIgnoreWhiteSpace{this};
    Utils::BoolAspect diffIgnoreBlankLines{this};
    Utils::BoolAspect describeByCommitId{this};

    QStringList addOptions(const QStringList &args) const;
};

CvsSettings &settings();

} // Cvs::Internal
