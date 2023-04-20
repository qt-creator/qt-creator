// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourceeditorfactory.h"

#include "resourceeditorconstants.h"
#include "resourceeditorplugin.h"
#include "resourceeditorw.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreplugintr.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/fsengine/fileiconprovider.h>

#include <QCoreApplication>

using namespace ResourceEditor::Constants;

namespace ResourceEditor::Internal {

ResourceEditorFactory::ResourceEditorFactory(ResourceEditorPlugin *plugin)
{
    setId(RESOURCEEDITOR_ID);
    setMimeTypes(QStringList(QLatin1String(C_RESOURCE_MIMETYPE)));
    setDisplayName(::Core::Tr::tr(C_RESOURCEEDITOR_DISPLAY_NAME));

    Utils::FileIconProvider::registerIconOverlayForSuffix(
                ProjectExplorer::Constants::FILEOVERLAY_QRC, "qrc");

    setEditorCreator([plugin] {
        return new ResourceEditorW(Core::Context(C_RESOURCEEDITOR), plugin);
    });
}

} // ResourceEditor::Internal
