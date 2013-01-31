/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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
#include "designerconstants.h"
#include "designerxmleditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/modemanager.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

using namespace Designer::Constants;

namespace Designer {
namespace Internal {

FormEditorFactory::FormEditorFactory()
  : Core::IEditorFactory(Core::ICore::instance()),
    m_mimeTypes(QLatin1String(FORM_MIMETYPE))
{
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/formeditor/images/qt_ui.png")),
                                               QLatin1String("ui"));
}

Core::Id FormEditorFactory::id() const
{
    return Core::Id(K_DESIGNER_XML_EDITOR_ID);
}

QString FormEditorFactory::displayName() const
{
    return qApp->translate("Designer", C_DESIGNER_XML_DISPLAY_NAME);
}

Core::IDocument *FormEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::openEditor(fileName, id());
    if (!iface)
        return 0;
    if (qobject_cast<FormWindowEditor *>(iface)) {
        Core::InfoBarEntry info(Core::Id(Constants::INFO_READ_ONLY),
                                tr("This file can only be edited in <b>Design</b> mode."));
        info.setCustomButtonInfo(tr("Switch mode"), this, SLOT(designerModeClicked()));
        iface->document()->infoBar()->addInfo(info);
    }
    return iface->document();
}

Core::IEditor *FormEditorFactory::createEditor(QWidget *parent)
{
    const EditorData data = FormEditorW::instance()->createEditor(parent);
    return data.formWindowEditor;
}

QStringList FormEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

void FormEditorFactory::designerModeClicked()
{
    Core::ModeManager::activateMode(Core::Constants::MODE_DESIGN);
}

} // namespace Internal
} // namespace Designer


