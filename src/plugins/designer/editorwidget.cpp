/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "editorwidget.h"
#include "formeditorw.h"
#include "formeditorstack.h"

#include <utils/qtcassert.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QDockWidget>
#include <QtGui/QAbstractItemView>

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

bool EditorWidget::removeFormWindowEditor(Core::IEditor *xmlEditor)
{
    return m_stack->removeFormWindowEditor(xmlEditor);
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
