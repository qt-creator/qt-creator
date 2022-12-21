// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>

#include <QSet>
#include <QStringList>

namespace Core {

class CORE_EXPORT FeatureSet;

class CORE_EXPORT IFeatureProvider
{
public:
    virtual ~IFeatureProvider() = default;

    virtual QSet<Utils::Id> availableFeatures(Utils::Id id) const = 0;
    virtual QSet<Utils::Id> availablePlatforms() const = 0;
    virtual QString displayNameForPlatform(Utils::Id id) const = 0;
};

} // namespace Core
