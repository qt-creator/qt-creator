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
#include "formwindoweditor.h"
#include "formeditorw.h"
#include "designerconstants.h"

#include <texteditor/basetextdocument.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>
#include <QDesignerFormEditorInterface>
#include "qt_private/formwindowbase_p.h"

#include <QtCore/QDebug>

namespace Designer {
namespace Internal {

FormEditorStack::FormXmlData::FormXmlData() :
    xmlEditor(0), formEditor(0)
{
}

FormEditorStack::FormEditorStack(QWidget *parent) :
    QStackedWidget(parent),
    m_designerCore(0)
{
    setObjectName(QLatin1String("FormEditorStack"));
}

Designer::FormWindowEditor *FormEditorStack::createFormWindowEditor(DesignerXmlEditorEditable *xmlEditor)
{
    FormEditorW *few = FormEditorW::instance();
    if (m_designerCore == 0) { // Initialize first time here
        m_designerCore = few->designerEditor();
        connect(m_designerCore->formWindowManager(), SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface*)),
                this, SLOT(updateFormWindowSelectionHandles()));
    }
    FormXmlData data;
    data.formEditor = few->createFormWindowEditor(this);
    data.formEditor->setFile(xmlEditor->file());
    data.xmlEditor = xmlEditor;
    addWidget(data.formEditor);
    m_formEditors.append(data);

    setFormEditorData(data.formEditor, xmlEditor->contents());

    TextEditor::BaseTextDocument *document = qobject_cast<TextEditor::BaseTextDocument*>(xmlEditor->file());
    connect(document, SIGNAL(reloaded()), SLOT(reloadDocument()));
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::createFormWindowEditor" << data.formEditor;
    return data.formEditor;
}

int FormEditorStack::indexOf(const QDesignerFormWindowInterface *fw) const
{
    const int count = m_formEditors.size();
     for(int i = 0; i < count; ++i)
         if (m_formEditors[i].formEditor->formWindow() == fw)
             return i;
     return -1;
}

int FormEditorStack::indexOf(const Core::IEditor *xmlEditor) const
{
    const int count = m_formEditors.size();
    for(int i = 0; i < count; ++i)
        if (m_formEditors[i].xmlEditor == xmlEditor)
            return i;
    return -1;
}

FormWindowEditor *FormEditorStack::activeFormWindow() const
{
    if (QDesignerFormWindowInterface *afw = m_designerCore->formWindowManager()->activeFormWindow())
        if (FormWindowEditor *fwe  = formWindowEditorForFormWindow(afw))
            return fwe;
    return 0;
}

Designer::FormWindowEditor *FormEditorStack::formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const
{
    const int i = indexOf(fw);
    return i != -1 ? m_formEditors[i].formEditor : static_cast<Designer::FormWindowEditor *>(0);
}

bool FormEditorStack::removeFormWindowEditor(Core::IEditor *xmlEditor)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::removeFormWindowEditor"  << xmlEditor;
    const int i = indexOf(xmlEditor);
    if (i == -1) // Fail silently as this is invoked for all editors.
        return false;
    disconnect(m_formEditors[i].formEditor->formWindow(), SIGNAL(changed()), this, SLOT(formChanged()));
    removeWidget(m_formEditors[i].formEditor->widget());
    delete m_formEditors[i].formEditor;
    m_formEditors.removeAt(i);
    return true;
}

bool FormEditorStack::setVisibleEditor(Core::IEditor *xmlEditor)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::setVisibleEditor"  << xmlEditor;
    const int i = indexOf(xmlEditor);
    QTC_ASSERT(i != -1, return false);

    if (i != currentIndex())
        setCurrentIndex(i);
    return true;
}

void FormEditorStack::updateFormWindowSelectionHandles()
{
    // Display form selection handles only on active window
    if (Designer::Constants::Internal::debug)
        qDebug() << "updateFormWindowSelectionHandles";
    QDesignerFormWindowInterface *activeFormWindow = m_designerCore->formWindowManager()->activeFormWindow();
    foreach(const FormXmlData  &fdm, m_formEditors) {
        const bool active = activeFormWindow == fdm.formEditor->formWindow();
        fdm.formEditor->updateFormWindowSelectionHandles(active);
    }
}

Designer::FormWindowEditor *FormEditorStack::formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const
{
    const int i = indexOf(xmlEditor);
    return i != -1 ? m_formEditors.at(i).formEditor : static_cast<Designer::FormWindowEditor *>(0);
}

void FormEditorStack::reloadDocument()
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::reloadDocument()";
    const int index = currentIndex();
    if (index >= 0)
        setFormEditorData(m_formEditors[index].formEditor, m_formEditors[index].xmlEditor->contents());
}

void FormEditorStack::setFormEditorData(Designer::FormWindowEditor *formEditor, const QString &contents)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::setFormEditorData()";
    disconnect(formEditor->formWindow(), SIGNAL(changed()), this, SLOT(formChanged()));
    formEditor->createNew(contents);
    connect(formEditor->formWindow(), SIGNAL(changed()), SLOT(formChanged()));
}

void FormEditorStack::formChanged()
{
    const int index = currentIndex();
    if (index < 0)
        return;
    if (Core::IEditor *currentEditor = Core::EditorManager::instance()->currentEditor()) {
        if (m_formEditors[index].xmlEditor == currentEditor) {
            FormXmlData &xmlData = m_formEditors[index];
            TextEditor::BaseTextDocument *doc = qobject_cast<TextEditor::BaseTextDocument*>(xmlData.xmlEditor->file());
            QTC_ASSERT(doc, return);
            if (doc)   // Save quietly (without spacer's warning).
                if (const qdesigner_internal::FormWindowBase *fwb = qobject_cast<const qdesigner_internal::FormWindowBase *>(xmlData.formEditor->formWindow()))
                   doc->document()->setPlainText(fwb->fileContents());
        }
    }
}

} // Internal
} // Designer
