// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace QmakeProjectManager::Internal {

class QmakeSettings : public Utils::AspectContainer
{
public:
    QmakeSettings();

    bool runSystemFunction() { return !ignoreSystemFunction(); }

    Utils::BoolAspect warnAgainstUnalignedBuildDir{this};
    Utils::BoolAspect alwaysRunQmake{this};
    Utils::BoolAspect ignoreSystemFunction{this};
};

QmakeSettings &settings();

} // QmakeProjectManager::Internal
