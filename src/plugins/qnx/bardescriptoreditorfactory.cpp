/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditorfactory.h"

#include "qnxconstants.h"
#include "bardescriptoreditor.h"
#include "bardescriptoreditorwidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/texteditoractionhandler.h>

using namespace Qnx;
using namespace Qnx::Internal;

class BarDescriptorActionHandler : public TextEditor::TextEditorActionHandler
{
public:
    BarDescriptorActionHandler(QObject *parent)
        : TextEditor::TextEditorActionHandler(parent, Constants::QNX_BAR_DESCRIPTOR_EDITOR_CONTEXT)
    {
    }
protected:
    TextEditor::TextEditorWidget *resolveTextEditorWidget(Core::IEditor *editor) const
    {
        BarDescriptorEditorWidget *w = qobject_cast<BarDescriptorEditorWidget *>(editor->widget());
        return w ? w->sourceWidget() : 0;
    }
};

BarDescriptorEditorFactory::BarDescriptorEditorFactory(QObject *parent)
    : Core::IEditorFactory(parent)
{
    setId(Constants::QNX_BAR_DESCRIPTOR_EDITOR_ID);
    setDisplayName(tr("Bar descriptor editor"));
    addMimeType(Constants::QNX_BAR_DESCRIPTOR_MIME_TYPE);
    new BarDescriptorActionHandler(this);
}

Core::IEditor *BarDescriptorEditorFactory::createEditor()
{
    BarDescriptorEditor *editor = new BarDescriptorEditor();
    return editor;
}
