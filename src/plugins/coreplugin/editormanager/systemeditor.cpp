// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "systemeditor.h"

#include "../coreplugintr.h"

#include <utils/filepath.h>

#include <QUrl>
#include <QDesktopServices>

using namespace Utils;

namespace Core::Internal {

SystemEditor::SystemEditor()
{
    setId("CorePlugin.OpenWithSystemEditor");
    setDisplayName(Tr::tr("System Editor"));
    setMimeTypes({"application/octet-stream"});

    setEditorStarter([](const FilePath &filePath, QString *errorMessage) {
        Q_UNUSED(errorMessage)
        QUrl url;
        url.setPath(filePath.toString());
        url.setScheme(QLatin1String("file"));
        if (!QDesktopServices::openUrl(url)) {
            if (errorMessage)
                *errorMessage = Tr::tr("Could not open URL %1.").arg(url.toString());
            return false;
        }
        return true;
    });
}

} // Core::Internal
