// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdownviewerplugin.h"
#include "markdownviewerfactory.h"

#include <extensionsystem/pluginmanager.h>

using namespace Core;

namespace Markdown {
namespace Internal {

bool MarkdownViewerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    ExtensionSystem::PluginManager::addObject(new MarkdownViewerFactory);

    return true;
}

} // namespace Internal
} // namespace Markdown
