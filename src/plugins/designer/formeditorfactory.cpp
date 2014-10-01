/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "formeditorfactory.h"
#include "formeditorw.h"
#include "formwindoweditor.h"
#include "editordata.h"
#include "designerxmleditorwidget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/modemanager.h>

#include <QCoreApplication>
#include <QDebug>

using namespace Designer::Constants;

namespace Designer {
namespace Internal {

FormEditorFactory::FormEditorFactory()
  : Core::IEditorFactory(Core::ICore::instance())
{
    setId(K_DESIGNER_XML_EDITOR_ID);
    setDisplayName(qApp->translate("Designer", C_DESIGNER_XML_DISPLAY_NAME));
    addMimeType(FORM_MIMETYPE);

    Core::FileIconProvider::registerIconOverlayForSuffix(":/formeditor/images/qt_ui.png", "ui");
}

Core::IEditor *FormEditorFactory::createEditor()
{
    const EditorData data = FormEditorW::instance()->createEditor();
    if (data.formWindowEditor) {
        Core::InfoBarEntry info(Core::Id(Constants::INFO_READ_ONLY),
                                tr("This file can only be edited in <b>Design</b> mode."));
        info.setCustomButtonInfo(tr("Switch Mode"), this, SLOT(designerModeClicked()));
        data.formWindowEditor->document()->infoBar()->addInfo(info);
    }
    return data.formWindowEditor;
}

void FormEditorFactory::designerModeClicked()
{
    Core::ModeManager::activateMode(Core::Constants::MODE_DESIGN);
}

} // namespace Internal
} // namespace Designer
