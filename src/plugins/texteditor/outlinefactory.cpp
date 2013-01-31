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

#include "outlinefactory.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QVBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QStackedWidget>

#include <QDebug>

namespace TextEditor {
namespace Internal {

OutlineWidgetStack::OutlineWidgetStack(OutlineFactory *factory) :
    QStackedWidget(),
    m_factory(factory),
    m_syncWithEditor(true),
    m_position(-1)
{
    QLabel *label = new QLabel(tr("No outline available"), this);
    label->setAlignment(Qt::AlignCenter);

    // set background to be white
    label->setAutoFillBackground(true);
    label->setBackgroundRole(QPalette::Base);

    addWidget(label);

    m_toggleSync = new QToolButton;
    m_toggleSync->setIcon(QIcon(QLatin1String(Core::Constants::ICON_LINK)));
    m_toggleSync->setCheckable(true);
    m_toggleSync->setChecked(true);
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    connect(m_toggleSync, SIGNAL(clicked(bool)), this, SLOT(toggleCursorSynchronization()));

    m_filterButton = new QToolButton;
    m_filterButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
    m_filterButton->setToolTip(tr("Filter tree"));
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterButton->setProperty("noArrow", true);
    m_filterMenu = new QMenu(m_filterButton);
    m_filterButton->setMenu(m_filterMenu);

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateCurrentEditor(Core::IEditor*)));
    updateCurrentEditor(Core::EditorManager::currentEditor());
}

OutlineWidgetStack::~OutlineWidgetStack()
{
}

QToolButton *OutlineWidgetStack::toggleSyncButton()
{
    return m_toggleSync;
}

QToolButton *OutlineWidgetStack::filterButton()
{
    return m_filterButton;
}

static inline QString outLineKey(int position)
{
    return QLatin1String("Outline.") + QString::number(position) + QLatin1String(".SyncWithEditor");
}

void OutlineWidgetStack::restoreSettings(int position)
{
    m_position = position; // save it so that we can save/restore in updateCurrentEditor

    QSettings *settings = Core::ICore::settings();
    const bool toggleSync = settings->value(outLineKey(position), true).toBool();
    toggleSyncButton()->setChecked(toggleSync);

    if (IOutlineWidget *outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget()))
        outlineWidget->restoreSettings(position);
}

void OutlineWidgetStack::saveSettings(int position)
{
    Q_ASSERT(position == m_position);

    QSettings *settings = Core::ICore::settings();
    settings->setValue(outLineKey(position), toggleSyncButton()->isEnabled());

    if (IOutlineWidget *outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget()))
        outlineWidget->saveSettings(position);
}

bool OutlineWidgetStack::isCursorSynchronized() const
{
    return m_syncWithEditor;
}

void OutlineWidgetStack::toggleCursorSynchronization()
{
    m_syncWithEditor = !m_syncWithEditor;
    if (IOutlineWidget *outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget()))
        outlineWidget->setCursorSynchronization(m_syncWithEditor);
}

void OutlineWidgetStack::updateFilterMenu()
{
    m_filterMenu->clear();
    if (IOutlineWidget *outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget())) {
        foreach (QAction *filterAction, outlineWidget->filterMenuActions()) {
            m_filterMenu->addAction(filterAction);
        }
    }
    m_filterButton->setEnabled(!m_filterMenu->actions().isEmpty());
}

void OutlineWidgetStack::updateCurrentEditor(Core::IEditor *editor)
{
    IOutlineWidget *newWidget = 0;

    if (editor) {
        foreach (IOutlineWidgetFactory *widgetFactory, m_factory->widgetFactories()) {
            if (widgetFactory->supportsEditor(editor)) {
                newWidget = widgetFactory->createWidget(editor);
                break;
            }
        }
    }

    if (newWidget != currentWidget()) {
        // delete old widget
        if (IOutlineWidget *outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget())) {
            if (m_position > -1)
                outlineWidget->saveSettings(m_position);
            removeWidget(outlineWidget);
            delete outlineWidget;
        }
        if (newWidget) {
            if (m_position > -1)
                newWidget->restoreSettings(m_position);
            newWidget->setCursorSynchronization(m_syncWithEditor);
            addWidget(newWidget);
            setCurrentWidget(newWidget);
        }

        updateFilterMenu();
    }
}

QList<IOutlineWidgetFactory*> OutlineFactory::widgetFactories() const
{
    return m_factories;
}

void OutlineFactory::setWidgetFactories(QList<IOutlineWidgetFactory*> factories)
{
    m_factories = factories;
}

QString OutlineFactory::displayName() const
{
    return tr("Outline");
}

int OutlineFactory::priority() const
{
    return 600;
}

Core::Id OutlineFactory::id() const
{
    return Core::Id("Outline");
}

QKeySequence OutlineFactory::activationSequence() const
{
    return QKeySequence();
}

Core::NavigationView OutlineFactory::createWidget()
{
    Core::NavigationView n;
    OutlineWidgetStack *placeHolder = new OutlineWidgetStack(this);
    n.widget = placeHolder;
    n.dockToolBarWidgets.append(placeHolder->filterButton());
    n.dockToolBarWidgets.append(placeHolder->toggleSyncButton());
    return n;
}

void OutlineFactory::saveSettings(int position, QWidget *widget)
{
    OutlineWidgetStack *widgetStack = qobject_cast<OutlineWidgetStack *>(widget);
    Q_ASSERT(widgetStack);
    widgetStack->saveSettings(position);
}

void OutlineFactory::restoreSettings(int position, QWidget *widget)
{
    OutlineWidgetStack *widgetStack = qobject_cast<OutlineWidgetStack *>(widget);
    Q_ASSERT(widgetStack);
    widgetStack->restoreSettings(position);
}

} // namespace Internal
} // namespace TextEditor
