// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "saferenderer.h"

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

namespace SafeRenderer::Internal {

SafeRendererPlugin::SafeRendererPlugin()
{
}

SafeRendererPlugin::~SafeRendererPlugin()
{
}

bool SafeRendererPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    return true;
}

} // namespace SafeRenderer::Internal
