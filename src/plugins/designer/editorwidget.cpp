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

#include "editorwidget.h"
#include "formeditorw.h"
#include "formeditorstack.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QDockWidget>
#include <QAbstractItemView>

using namespace Designer::Constants;

namespace Designer {
namespace Internal {

// ---------- EditorWidget

EditorWidget::EditorWidget(QWidget *parent) :
    Utils::FancyMainWindow(parent),
    m_stack(new FormEditorStack)
{
    setObjectName("EditorWidget");
    setCentralWidget(m_stack);
    setDocumentMode(true);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    QWidget * const*subs = FormEditorW::designerSubWindows();
    for (int i = 0; i < DesignerSubWindowCount; i++) {
        QWidget *subWindow = subs[i];
        subWindow->setWindowTitle(subs[i]->windowTitle());
        m_designerDockWidgets[i] = addDockForWidget(subWindow);

        // Since we have 1-pixel splitters, we generally want to remove
        // frames around item views. So we apply this hack for now.
        QList<QAbstractItemView*> frames = subWindow->findChildren<QAbstractItemView*>();
        for (int i = 0 ; i< frames.count(); ++i)
            frames[i]->setFrameStyle(QFrame::NoFrame);

    }
    resetToDefaultLayout();
}

void EditorWidget::resetToDefaultLayout()
{
    setTrackingEnabled(false);
    const QList<QDockWidget *> dockWidgetList = dockWidgets();
    for (QDockWidget *dockWidget : dockWidgetList) {
        dockWidget->setFloating(false);
        removeDockWidget(dockWidget);
    }

    addDockWidget(Qt::LeftDockWidgetArea, m_designerDockWidgets[WidgetBoxSubWindow]);
    addDockWidget(Qt::RightDockWidgetArea, m_designerDockWidgets[ObjectInspectorSubWindow]);
    addDockWidget(Qt::RightDockWidgetArea, m_designerDockWidgets[PropertyEditorSubWindow]);
    addDockWidget(Qt::BottomDockWidgetArea, m_designerDockWidgets[ActionEditorSubWindow]);
    addDockWidget(Qt::BottomDockWidgetArea, m_designerDockWidgets[SignalSlotEditorSubWindow]);

    tabifyDockWidget(m_designerDockWidgets[ActionEditorSubWindow],
                     m_designerDockWidgets[SignalSlotEditorSubWindow]);

    for (QDockWidget *dockWidget : dockWidgetList)
        dockWidget->show();

    setTrackingEnabled(true);
}

QDockWidget* const* EditorWidget::designerDockWidgets() const
{
    return m_designerDockWidgets;
}

void EditorWidget::add(SharedTools::WidgetHost *widgetHost, FormWindowEditor *formWindowEditor)
{
    EditorData data;
    data.formWindowEditor = formWindowEditor;
    data.widgetHost = widgetHost;
    m_stack->add(data);
}

void EditorWidget::removeFormWindowEditor(Core::IEditor *xmlEditor)
{
    m_stack->removeFormWindowEditor(xmlEditor);
}

bool EditorWidget::setVisibleEditor(Core::IEditor *xmlEditor)
{
    return m_stack->setVisibleEditor(xmlEditor);
}

SharedTools::WidgetHost *EditorWidget::formWindowEditorForXmlEditor(const Core::IEditor *xmlEditor) const
{
    return m_stack->formWindowEditorForXmlEditor(xmlEditor);
}

EditorData EditorWidget::activeEditor() const
{
    return m_stack->activeEditor();
}

SharedTools::WidgetHost *EditorWidget::formWindowEditorForFormWindow(const QDesignerFormWindowInterface *fw) const
{
    return m_stack->formWindowEditorForFormWindow(fw);
}

} // namespace Internal
} // namespace Designer
