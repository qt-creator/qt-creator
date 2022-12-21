// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/featureprovider.h>

namespace ProjectExplorer {
namespace Internal {

class KitFeatureProvider : public Core::IFeatureProvider
{
public:
    QSet<Utils::Id> availableFeatures(Utils::Id id) const override;
    QSet<Utils::Id> availablePlatforms() const override;
    QString displayNameForPlatform(Utils::Id id) const override;
};

} // namespace Internal
} // namespace ProjectExplorer
