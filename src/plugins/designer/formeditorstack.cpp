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

#include "formeditorstack.h"
#include "designerxmleditor.h"
#include <QDesignerFormWindowInterface>
#include <texteditor/basetextdocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include "formwindoweditor.h"
#include "formeditorw.h"
#include "designerconstants.h"
#include "qt_private/formwindowbase_p.h"

#include <QtCore/QStringList>

namespace Designer {
namespace Internal {

FormEditorStack::FormEditorStack() : activeEditor(0)
{

}

FormEditorStack::~FormEditorStack()
{
    qDeleteAll(m_formEditors);
}

Designer::FormWindowEditor *FormEditorStack::createFormWindowEditor(DesignerXmlEditorEditable *xmlEditor)
{
    FormXmlData *data = new FormXmlData;
    data->formEditor = FormEditorW::instance()->createFormWindowEditor(this);
    data->formEditor->setFile(xmlEditor->file());
    data->xmlEditor = xmlEditor;
    data->widgetIndex = addWidget(data->formEditor->widget());
    m_formEditors.append(data);

    setFormEditorData(data->formEditor, xmlEditor->contents());

    TextEditor::BaseTextDocument *document = qobject_cast<TextEditor::BaseTextDocument*>(xmlEditor->file());
    connect(document, SIGNAL(reloaded()), SLOT(reloadDocument()));

    return data->formEditor;
}

bool FormEditorStack::removeFormWindowEditor(Core::IEditor *xmlEditor)
{
    for(int i = 0; i < m_formEditors.length(); ++i) {
        if (m_formEditors[i]->xmlEditor == xmlEditor) {
            disconnect(m_formEditors[i]->formEditor->formWindow(), SIGNAL(changed()), this, SLOT(formChanged()));
            removeWidget(m_formEditors[i]->formEditor->widget());
            delete m_formEditors[i]->formEditor;
            m_formEditors.removeAt(i);
            return true;
        }
    }
    return false;
}

bool FormEditorStack::setVisibleEditor(Core::IEditor *xmlEditor)
{
    for(int i = 0; i < m_formEditors.length(); ++i) {
        if (m_formEditors[i]->xmlEditor == xmlEditor) {
            setCurrentIndex(m_formEditors[i]->widgetIndex);
            activeEditor = m_formEditors[i];
            return true;
        }
    }
    return false;
}

Designer::FormWindowEditor *FormEditorStack::formWindowEditorForXmlEditor(Core::IEditor *xmlEditor)
{
    foreach(FormXmlData *data, m_formEditors) {
        if (data->xmlEditor == xmlEditor)
            return data->formEditor;
    }
    return 0;
}


void FormEditorStack::reloadDocument()
{
    if (activeEditor) {
        setFormEditorData(activeEditor->formEditor, activeEditor->xmlEditor->contents());
    }
}

void FormEditorStack::setFormEditorData(Designer::FormWindowEditor *formEditor, const QString &contents)
{
    disconnect(formEditor->formWindow(), SIGNAL(changed()), this, SLOT(formChanged()));
    formEditor->createNew(contents);
    connect(formEditor->formWindow(), SIGNAL(changed()), SLOT(formChanged()));
}

void FormEditorStack::formChanged()
{
    Core::ICore *core = Core::ICore::instance();

    if (core->editorManager()->currentEditor() && activeEditor
        && core->editorManager()->currentEditor() == activeEditor->xmlEditor)
    {
        TextEditor::BaseTextDocument *doc = qobject_cast<TextEditor::BaseTextDocument*>(activeEditor->xmlEditor->file());
        Q_ASSERT(doc);
        if (doc) {
            // Save quietly (without spacer's warning).
            if (const qdesigner_internal::FormWindowBase *fwb = qobject_cast<const qdesigner_internal::FormWindowBase *>(activeEditor->formEditor))
                doc->document()->setPlainText(fwb->fileContents());
        }
    }
}

} // Internal
} // Designer
