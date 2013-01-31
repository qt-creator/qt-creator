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

#include "formeditorstack.h"
#include "formwindoweditor.h"
#include "formeditorw.h"
#include "designerconstants.h"

#include <widgethost.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>

#include <utils/qtcassert.h>

#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>
#include <QDesignerFormEditorInterface>
#include <QDesignerPropertyEditorInterface>

#include <QDebug>
#include <QVariant>
#include <QRect>

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
        m_designerCore = data.widgetHost->formWindow()->core();
        connect(m_designerCore->formWindowManager(), SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface*)),
                this, SLOT(updateFormWindowSelectionHandles()));
        connect(Core::ModeManager::instance(), SIGNAL(currentModeAboutToChange(Core::IMode*)),
                this, SLOT(modeAboutToChange(Core::IMode*)));
    }

    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::add"  << data.formWindowEditor << data.widgetHost;

    m_formEditors.append(data);
    addWidget(data.widgetHost);
    // Editors are normally removed by listening to EditorManager::editorsClosed.
    // However, in the case opening a file fails, EditorManager just deletes the editor, which
    // is caught by the destroyed() signal.
    connect(data.formWindowEditor, SIGNAL(destroyed(QObject*)),
            this, SLOT(removeFormWindowEditor(QObject*)));

    connect(data.widgetHost, SIGNAL(formWindowSizeChanged(int,int)),
            this, SLOT(formSizeChanged(int,int)));

    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::add" << data.widgetHost << m_formEditors.size();

    // Since we have 1 pixel splitters we enforce no frame
    // on the content widget
    if (QFrame *frame = qobject_cast<QFrame*>(data.widgetHost))
        frame->setFrameStyle(QFrame::NoFrame);
}

int FormEditorStack::indexOfFormWindow(const QDesignerFormWindowInterface *fw) const
{
    const int count = m_formEditors.size();
     for (int i = 0; i < count; ++i)
         if (m_formEditors[i].widgetHost->formWindow() == fw)
             return i;
     return -1;
}

int FormEditorStack::indexOfFormEditor(const QObject *xmlEditor) const
{
    const int count = m_formEditors.size();
    for (int i = 0; i < count; ++i)
        if (m_formEditors[i].formWindowEditor == xmlEditor)
            return i;
    return -1;
}

EditorData FormEditorStack::activeEditor() const
{
    // Should actually be in sync with current index.
    if (QDesignerFormWindowInterface *afw = m_designerCore->formWindowManager()->activeFormWindow()) {
        const int index = indexOfFormWindow(afw);
        if (index >= 0)
            return m_formEditors.at(index);
    }
    return EditorData();
}

SharedTools::WidgetHost *FormEditorStack::formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const
{
    const int i = indexOfFormWindow(fw);
    return i != -1 ? m_formEditors[i].widgetHost : static_cast<SharedTools::WidgetHost *>(0);
}

void FormEditorStack::removeFormWindowEditor(QObject *xmlEditor)
{
    const int i = indexOfFormEditor(xmlEditor);
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::removeFormWindowEditor"  << xmlEditor << i << " of " << m_formEditors.size() ;
    if (i == -1) // Fail silently as this is invoked for all editors from EditorManager
        return;  // and editor deletion signal.

    removeWidget(m_formEditors[i].widgetHost);
    m_formEditors[i].widgetHost->deleteLater();
    m_formEditors.removeAt(i);
}

bool FormEditorStack::setVisibleEditor(Core::IEditor *xmlEditor)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::setVisibleEditor"  << xmlEditor;
    const int i = indexOfFormEditor(xmlEditor);
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
    foreach (const EditorData  &fdm, m_formEditors) {
        const bool active = activeFormWindow == fdm.widgetHost->formWindow();
        fdm.widgetHost->updateFormWindowSelectionHandles(active);
    }
}

void FormEditorStack::formSizeChanged(int w, int h)
{
    // Handle main container resize.
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << w << h;
    if (const SharedTools::WidgetHost *wh = qobject_cast<const SharedTools::WidgetHost *>(sender())) {
        wh->formWindow()->setDirty(true);
        static const QString geometry = QLatin1String("geometry");
        m_designerCore->propertyEditor()->setPropertyValue(geometry, QRect(0,0,w,h) );
    }
}

SharedTools::WidgetHost *FormEditorStack::formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const
{
    const int i = indexOfFormEditor(xmlEditor);
    return i != -1 ? m_formEditors.at(i).widgetHost : static_cast<SharedTools::WidgetHost *>(0);
}

void FormEditorStack::modeAboutToChange(Core::IMode *m)
{
    if (Designer::Constants::Internal::debug && m)
        qDebug() << "FormEditorStack::modeAboutToChange"  << m->id().toString();

    // Sync the editor when entering edit mode
    if (m && m->id() == Core::Constants::MODE_EDIT)
        foreach (const EditorData &data, m_formEditors)
            data.formWindowEditor->syncXmlEditor();
}

} // Internal
} // Designer
