/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "formeditorfactory.h"
#include "formeditorw.h"
#include "formwindoweditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QCoreApplication>
#include <QDebug>

using namespace Core;
using namespace Designer::Constants;

namespace Designer {
namespace Internal {

FormEditorFactory::FormEditorFactory()
{
    setId(K_DESIGNER_XML_EDITOR_ID);
    setDisplayName(qApp->translate("Designer", C_DESIGNER_XML_DISPLAY_NAME));
    addMimeType(FORM_MIMETYPE);

    FileIconProvider::registerIconOverlayForSuffix(ProjectExplorer::Constants::FILEOVERLAY_UI, "ui");
}

IEditor *FormEditorFactory::createEditor()
{
    return FormEditorW::createEditor();
}

} // namespace Internal
} // namespace Designer
