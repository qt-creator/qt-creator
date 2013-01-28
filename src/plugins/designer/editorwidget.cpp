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

#include "editorwidget.h"
#include "formeditorw.h"
#include "formeditorstack.h"

#include <coreplugin/editormanager/ieditor.h>

#include <utils/qtcassert.h>

#include <QVBoxLayout>
#include <QDockWidget>
#include <QAbstractItemView>

using namespace Designer::Constants;

namespace Designer {
namespace Internal {

// ---------- EditorWidget

EditorWidget::EditorWidget(FormEditorW *few, QWidget *parent) :
    Utils::FancyMainWindow(parent),
    m_stack(new FormEditorStack)
{
    setObjectName(QLatin1String("EditorWidget"));
    setCentralWidget(m_stack);
    setDocumentMode(true);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    QWidget * const*subs = few->designerSubWindows();
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
    QList<QDockWidget *> dockWidgetList = dockWidgets();
    foreach (QDockWidget *dockWidget, dockWidgetList) {
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

    foreach (QDockWidget *dockWidget, dockWidgetList)
        dockWidget->show();

    setTrackingEnabled(true);
}

QDockWidget* const* EditorWidget::designerDockWidgets() const
{
    return m_designerDockWidgets;
}

void EditorWidget::add(const EditorData &d)
{
    m_stack->add(d);
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
