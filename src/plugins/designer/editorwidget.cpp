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

#include "editorwidget.h"
#include "formeditorw.h"

#include <coreplugin/minisplitter.h>
#include <utils/qtcassert.h>

#include <QtCore/QEvent>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTabWidget>

static const char *editorWidgetStateKeyC = "editorWidgetState";

using namespace Designer::Constants;

namespace Designer {
namespace Internal {

SharedSubWindow::SharedSubWindow(QWidget *shared, QWidget *parent) :
   QWidget(parent),
   m_shared(shared),
   m_layout(new QVBoxLayout)
{
    QTC_ASSERT(m_shared, /**/);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void SharedSubWindow::activate()
{
    // Take the widget off the other parent
    QTC_ASSERT(m_shared, return);
    QWidget *currentParent = m_shared->parentWidget();
    if (currentParent == this)
        return;

    m_layout->addWidget(m_shared);
    m_shared->show();
}

SharedSubWindow::~SharedSubWindow()
{
    // Do not destroy the shared sub window if we currently own it
    if (m_shared->parent() == this) {
        m_shared->hide();
        m_shared->setParent(0);
    }
}

// ---------- EditorWidget

QHash<QString, QVariant> EditorWidget::m_globalState = QHash<QString, QVariant>();

EditorWidget::EditorWidget(QWidget *formWindow)
    : m_mainWindow(new Utils::FancyMainWindow),
    m_initialized(false)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);
    layout->addWidget(m_mainWindow);
    m_mainWindow->setCentralWidget(formWindow);
    m_mainWindow->setDocumentMode(true);
    m_mainWindow->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
    m_mainWindow->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    m_mainWindow->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Get shared sub windows from Form Editor
    FormEditorW *few = FormEditorW::instance();
    QWidget * const*subs = few->designerSubWindows();

    // Create shared sub windows
    for (int i=0; i < DesignerSubWindowCount; i++) {
        m_designerSubWindows[i] = new SharedSubWindow(subs[i]);
        m_designerSubWindows[i]->setWindowTitle(subs[i]->windowTitle());
        m_designerDockWidgets[i] = m_mainWindow->addDockForWidget(m_designerSubWindows[i]);
    }
}

void EditorWidget::resetToDefaultLayout()
{
    m_mainWindow->setTrackingEnabled(false);
    QList<QDockWidget *> dockWidgets = m_mainWindow->dockWidgets();
    foreach (QDockWidget *dockWidget, dockWidgets) {
        dockWidget->setFloating(false);
        m_mainWindow->removeDockWidget(dockWidget);
    }

    m_mainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_designerDockWidgets[WidgetBoxSubWindow]);
    m_mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_designerDockWidgets[ObjectInspectorSubWindow]);
    m_mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_designerDockWidgets[PropertyEditorSubWindow]);
    m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, m_designerDockWidgets[ActionEditorSubWindow]);
    m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, m_designerDockWidgets[SignalSlotEditorSubWindow]);

    m_mainWindow->tabifyDockWidget(m_designerDockWidgets[ActionEditorSubWindow],
                                   m_designerDockWidgets[SignalSlotEditorSubWindow]);

    foreach (QDockWidget *dockWidget, dockWidgets) {
        dockWidget->show();
    }

    m_mainWindow->setTrackingEnabled(true);
}

void EditorWidget::activate()
{
    /*

       - now, settings are only changed when form is hidden.
       - they should be not restored when a new form is activated - the same settings should be kept.
       - only on initial load, settings should be loaded.

     */
    for (int i=0; i < DesignerSubWindowCount; i++)
        m_designerSubWindows[i]->activate();

    if (!m_initialized) {
        // set a default layout, so if something goes wrong with
        // restoring the settings below, there is a fallback
        // (otherwise we end up with a broken mainwindow layout)
        // we can't do it in the constructor, because the sub windows
        // don't have their widgets yet there
        resetToDefaultLayout();
        m_initialized = true;
        if (!m_globalState.isEmpty())
            m_mainWindow->restoreSettings(m_globalState);
    }

    if (m_globalState.isEmpty())
        m_globalState = m_mainWindow->saveSettings();
}

void EditorWidget::hideEvent(QHideEvent *)
{
    m_globalState = m_mainWindow->saveSettings();
}

void EditorWidget::saveState(QSettings *settings)
{
    settings->beginGroup(editorWidgetStateKeyC);
    QHashIterator<QString, QVariant> it(m_globalState);
    while (it.hasNext()) {
        it.next();
        settings->setValue(it.key(), it.value());
    }
    settings->endGroup();
}

void EditorWidget::restoreState(QSettings *settings)
{
    m_globalState.clear();
    settings->beginGroup(editorWidgetStateKeyC);
    foreach (const QString &key, settings->childKeys()) {
        m_globalState.insert(key, settings->value(key));
    }
    settings->endGroup();
}

void EditorWidget::toolChanged(int i)
{
    Q_UNUSED(i)
//    TODO: How to activate the right dock window?
//    if (m_bottomTab)
//        m_bottomTab->setCurrentIndex(i == EditModeSignalsSlotEditor ? SignalSlotEditorTab : ActionEditorTab);
}

} // namespace Internal
} // namespace Designer
