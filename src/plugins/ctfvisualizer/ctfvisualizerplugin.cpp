// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "ctfvisualizerplugin.h"

#include "ctfvisualizertool.h"

namespace CtfVisualizer {
namespace Internal {

class CtfVisualizerPluginPrivate
{
public:
    CtfVisualizerTool profilerTool;
};

CtfVisualizerPlugin::~CtfVisualizerPlugin()
{
    delete d;
}

bool CtfVisualizerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new CtfVisualizerPluginPrivate;
    return true;
}

} // namespace Internal
} // namespace CtfVisualizer
