// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorfactory.h"

#include "designerconstants.h"
#include "designertr.h"
#include "formeditor.h"

#include <coreplugin/coreconstants.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/fsengine/fileiconprovider.h>

using namespace Core;
using namespace Designer::Constants;
using namespace Utils;

namespace Designer {
namespace Internal {

FormEditorFactory::FormEditorFactory()
{
    setId(K_DESIGNER_XML_EDITOR_ID);
    setDisplayName(Tr::tr(C_DESIGNER_XML_DISPLAY_NAME));
    addMimeType(FORM_MIMETYPE);
    setEditorCreator([] { return Designer::Internal::createEditor(); });

    FileIconProvider::registerIconOverlayForSuffix(ProjectExplorer::Constants::FILEOVERLAY_UI, "ui");
}

} // namespace Internal
} // namespace Designer
