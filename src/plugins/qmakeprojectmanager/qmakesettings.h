// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace QmakeProjectManager::Internal {

class QmakeSettings : public Core::PagedSettings
{
public:
    QmakeSettings();

    bool runSystemFunction() { return !ignoreSystemFunction(); }

    Utils::BoolAspect warnAgainstUnalignedBuildDir;
    Utils::BoolAspect alwaysRunQmake;
    Utils::BoolAspect ignoreSystemFunction;
};

QmakeSettings &settings();

} // QmakeProjectManager::Internal
