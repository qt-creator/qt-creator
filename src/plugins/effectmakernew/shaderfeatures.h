// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QFlags>
#include <QString>

namespace EffectMaker {

class ShaderFeatures
{
public:
    enum Feature {
        Time        = 1 << 0,
        Frame       = 1 << 1,
        Resolution  = 1 << 2,
        Source      = 1 << 3,
        Mouse       = 1 << 4,
        FragCoord   = 1 << 5,
        GridMesh    = 1 << 6,
        BlurSources = 1 << 7
    };
    Q_DECLARE_FLAGS(Features, Feature)

    ShaderFeatures();
    void update(const QString &vs, const QString &fs, const QString &qml);

    bool enabled(ShaderFeatures::Feature feature) const;

    int gridMeshWidth() const;

    int gridMeshHeight() const;

private:
    void checkLine(const QString &line, ShaderFeatures::Features &features);
    ShaderFeatures::Features m_enabledFeatures;
    int m_gridMeshWidth = 1;
    int m_gridMeshHeight = 1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ShaderFeatures::Features)
} // namespace EffectMaker

