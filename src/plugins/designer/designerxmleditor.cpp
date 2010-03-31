/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designerxmleditor.h"
#include "formwindoweditor.h"
#include "designerconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtCore/QDebug>

namespace Designer {
namespace Internal {

DesignerXmlEditor::DesignerXmlEditor(QDesignerFormWindowInterface *form,
                                     QWidget *parent) :
    TextEditor::PlainTextEditor(parent),
    m_designerEditor(new FormWindowEditor(this, form))
{
    setReadOnly(true);
}

TextEditor::BaseTextEditorEditable *DesignerXmlEditor::createEditableInterface()
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "DesignerXmlEditor::createEditableInterface()";
    return m_designerEditor->textEditable();
}

FormWindowEditor *DesignerXmlEditor::designerEditor() const
{
    return m_designerEditor;
}

}
} // namespace Designer

