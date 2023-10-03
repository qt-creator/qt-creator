// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "shaderfeatures.h"
#include <QStringList>
#include <QDebug>

namespace EffectMaker {

ShaderFeatures::ShaderFeatures()
{
}

// Browse the shaders and check which features are used in them.
void ShaderFeatures::update(const QString &vs, const QString &fs, const QString &qml)
{
    QStringList vsList = vs.split("\n");
    QStringList fsList = fs.split("\n");

    const QStringList code = vsList + fsList;
    Features newFeatures = {};
    m_gridMeshWidth = 1;
    m_gridMeshHeight = 1;
    for (const QString &line : code)
        checkLine(line, newFeatures);

    // iTime may also be used in QML side, without being used in shaders.
    // In this case enable the time helpers creation.
    if (qml.contains("iTime"))
        newFeatures.setFlag(Time, true);

    if (newFeatures != m_enabledFeatures)
        m_enabledFeatures = newFeatures;
}

bool ShaderFeatures::enabled(ShaderFeatures::Feature feature) const
{
    return m_enabledFeatures.testFlag(feature);
}

void ShaderFeatures::checkLine(const QString &line, Features &features)
{
    if (line.contains("iTime"))
        features.setFlag(Time, true);

    if (line.contains("iFrame"))
        features.setFlag(Frame, true);

    if (line.contains("iResolution"))
        features.setFlag(Resolution, true);

    if (line.contains("iSource"))
        features.setFlag(Source, true);

    if (line.contains("iMouse"))
        features.setFlag(Mouse, true);

    if (line.contains("fragCoord"))
        features.setFlag(FragCoord, true);

    if (line.contains("@mesh")) {
        // Get the mesh size, remove "@mesh"
        QString l = line.trimmed().sliced(5);
        QStringList list = l.split(QLatin1Char(','));
        if (list.size() >= 2) {
            int w = list.at(0).trimmed().toInt();
            int h = list.at(1).trimmed().toInt();
            // Set size to max values
            m_gridMeshWidth = std::max(m_gridMeshWidth, w);
            m_gridMeshHeight = std::max(m_gridMeshHeight, h);
        }
        // If is bigger than default (1, 1), set the feature
        if (m_gridMeshWidth > 1 || m_gridMeshHeight > 1)
            features.setFlag(GridMesh, true);
    }
    if (line.contains("@blursources"))
        features.setFlag(BlurSources, true);
}

int ShaderFeatures::gridMeshHeight() const
{
    return m_gridMeshHeight;
}

int ShaderFeatures::gridMeshWidth() const
{
    return m_gridMeshWidth;
}

} // namespace EffectMaker

