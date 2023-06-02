// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorstack.h"

#include "designerconstants.h"
#include "formwindoweditor.h"
#include "formeditor.h"
#include "formwindowfile.h"

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
#include <QRect>

namespace Designer {
namespace Internal {

FormEditorStack::FormEditorStack(QWidget *parent) :
    QStackedWidget(parent)
{
    setObjectName("FormEditorStack");
}

FormEditorStack::~FormEditorStack()
{
    if (m_designerCore) {
        if (auto fwm = m_designerCore->formWindowManager()) {
            disconnect(fwm, &QDesignerFormWindowManagerInterface::activeFormWindowChanged,
                       this, &FormEditorStack::updateFormWindowSelectionHandles);
        }
    }
}

void FormEditorStack::add(const EditorData &data)
{
    if (m_designerCore == nullptr) { // Initialize first time here
        m_designerCore = data.widgetHost->formWindow()->core();
        connect(m_designerCore->formWindowManager(), &QDesignerFormWindowManagerInterface::activeFormWindowChanged,
                this, &FormEditorStack::updateFormWindowSelectionHandles);
        connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeAboutToChange,
                this, &FormEditorStack::modeAboutToChange);
    }

    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::add"  << data.formWindowEditor << data.widgetHost;

    m_formEditors.append(data);
    addWidget(data.widgetHost);
    // Editors are normally removed by listening to EditorManager::editorsClosed.
    // However, in the case opening a file fails, EditorManager just deletes the editor, which
    // is caught by the destroyed() signal.
    connect(data.formWindowEditor, &FormWindowEditor::destroyed,
            this, &FormEditorStack::removeFormWindowEditor);

    connect(data.widgetHost, &SharedTools::WidgetHost::formWindowSizeChanged, this,
            [this, wh = data.widgetHost](int w, int h) { formSizeChanged(wh, w, h); });

    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::add" << data.widgetHost << m_formEditors.size();

    // Since we have 1 pixel splitters we enforce no frame
    // on the content widget
    if (auto frame = qobject_cast<QFrame*>(data.widgetHost))
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
    return {};
}

SharedTools::WidgetHost *FormEditorStack::formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const
{
    const int i = indexOfFormWindow(fw);
    return i != -1 ? m_formEditors[i].widgetHost : static_cast<SharedTools::WidgetHost *>(nullptr);
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
    for (const EditorData  &fdm : std::as_const(m_formEditors)) {
        const bool active = activeFormWindow == fdm.widgetHost->formWindow();
        fdm.widgetHost->updateFormWindowSelectionHandles(active);
    }
}

void FormEditorStack::formSizeChanged(const SharedTools::WidgetHost *widgetHost, int w, int h)
{
    // Handle main container resize.
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << w << h;
    widgetHost->formWindow()->setDirty(true);
    m_designerCore->propertyEditor()->setPropertyValue("geometry", QRect(0, 0, w, h));
}

SharedTools::WidgetHost *FormEditorStack::formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const
{
    const int i = indexOfFormEditor(xmlEditor);
    return i != -1 ? m_formEditors.at(i).widgetHost : static_cast<SharedTools::WidgetHost *>(nullptr);
}

void FormEditorStack::modeAboutToChange(Utils::Id mode)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorStack::modeAboutToChange"  << mode.toString();

    // Sync the editor when entering edit mode
    if (mode == Core::Constants::MODE_EDIT)
        for (const EditorData &data : std::as_const(m_formEditors))
            data.formWindowEditor->formWindowFile()->syncXmlFromFormWindow();
}

} // Internal
} // Designer
