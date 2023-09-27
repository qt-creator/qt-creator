// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfvisualizerplugin.h"

#include "ctfvisualizertool.h"

namespace CtfVisualizer::Internal {

class CtfVisualizerPluginPrivate
{
public:
    CtfVisualizerTool profilerTool;
};

CtfVisualizerPlugin::~CtfVisualizerPlugin()
{
    delete d;
}

void CtfVisualizerPlugin::initialize()
{
    d = new CtfVisualizerPluginPrivate;
}

} // namespace CtfVisualizer::Internal
