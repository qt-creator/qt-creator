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

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>

#include <utils/qtcassert.h>

#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>
#include <QDesignerFormEditorInterface>

#include <QtCore/QDebug>

namespace Designer {
namespace Internal {

FormEditorStack::FormEditorStack(QWidget *parent) :
    QStackedWidget(parent),
    m_designerCore(0)
{
    setObjectName(QLatin1String("FormEditorStack"));
}

void FormEditorStack::add(const EditorData &data)
{
    if (m_designerCore == 0) { // Initialize first time here
        m_designerCore = data.formEditor->formWindow()->core();
        connect(m_designerCore->formWindowManager(), SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface*)),
                this, SLOT(updateFormWindowSelectionHandles()));
        connect(Core::ModeManager::instance(), SIGNAL(currentModeAboutToChange(Core::IMode*)),
                this, SLOT(modeAboutToChange(Core::IMode*)));
    }

    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::add"  << data.xmlEditor << data.formEditor;

    m_formEditors.append(data);
    addWidget(data.formEditor);

    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::add" << data.formEditor;
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

EditorData FormEditorStack::activeEditor() const
{
    // Should actually be in sync with current index.
    if (QDesignerFormWindowInterface *afw = m_designerCore->formWindowManager()->activeFormWindow()) {
        const int index = indexOf(afw);
        if (index >= 0)
            return m_formEditors.at(index);
    }
    return EditorData();
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
    foreach(const EditorData  &fdm, m_formEditors) {
        const bool active = activeFormWindow == fdm.formEditor->formWindow();
        fdm.formEditor->updateFormWindowSelectionHandles(active);
    }
}

Designer::FormWindowEditor *FormEditorStack::formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const
{
    const int i = indexOf(xmlEditor);
    return i != -1 ? m_formEditors.at(i).formEditor : static_cast<Designer::FormWindowEditor *>(0);
}

void FormEditorStack::modeAboutToChange(Core::IMode *m)
{
    if (Designer::Constants::Internal::debug && m)
        qDebug() << "FormEditorStack::modeAboutToChange"  << m->id();

    // Sync the editor when leaving design mode
    if (m && m->id() == QLatin1String(Core::Constants::MODE_DESIGN))
        foreach(const EditorData &data, m_formEditors)
            data.xmlEditor->syncXmlEditor();
}

} // Internal
} // Designer
