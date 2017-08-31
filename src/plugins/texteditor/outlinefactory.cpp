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

#include "outlinefactory.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <utils/utilsicons.h>

#include <QToolButton>
#include <QLabel>
#include <QStackedWidget>

#include <QDebug>

namespace TextEditor {
namespace Internal {

OutlineWidgetStack::OutlineWidgetStack(OutlineFactory *factory) :
    QStackedWidget(),
    m_factory(factory),
    m_syncWithEditor(true)
{
    QLabel *label = new QLabel(tr("No outline available"), this);
    label->setAlignment(Qt::AlignCenter);

    // set background to be white
    label->setAutoFillBackground(true);
    label->setBackgroundRole(QPalette::Base);

    addWidget(label);

    m_toggleSync = new QToolButton;
    m_toggleSync->setIcon(Utils::Icons::LINK.icon());
    m_toggleSync->setCheckable(true);
    m_toggleSync->setChecked(true);
    m_toggleSync->setToolTip(tr("Synchronize with Editor"));
    connect(m_toggleSync, &QAbstractButton::clicked,
            this, &OutlineWidgetStack::toggleCursorSynchronization);

    m_filterButton = new QToolButton(this);
    // The ToolButton needs a parent because updateFilterMenu() sets
    // it visible. That would open a top-level window if the button
    // did not have a parent in that moment.

    m_filterButton->setIcon(Utils::Icons::FILTER.icon());
    m_filterButton->setToolTip(tr("Filter tree"));
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterButton->setProperty("noArrow", true);
    m_filterMenu = new QMenu(m_filterButton);
    m_filterButton->setMenu(m_filterMenu);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &OutlineWidgetStack::updateCurrentEditor);
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

void OutlineWidgetStack::saveSettings(QSettings *settings, int position)
{
    const QString baseKey = QStringLiteral("Outline.%1.").arg(position);
    settings->setValue(baseKey + QLatin1String("SyncWithEditor"), toggleSyncButton()->isChecked());
    for (auto iter = m_widgetSettings.constBegin(); iter != m_widgetSettings.constEnd(); ++iter)
        settings->setValue(baseKey + iter.key(), iter.value());
}

void OutlineWidgetStack::restoreSettings(QSettings *settings, int position)
{
    const QString baseKey = QStringLiteral("Outline.%1.").arg(position);

    bool syncWithEditor = true;
    m_widgetSettings.clear();
    foreach (const QString &longKey, settings->allKeys()) {
        if (!longKey.startsWith(baseKey))
            continue;

        const QString key = longKey.mid(baseKey.length());

        if (key == QLatin1String("SyncWithEditor")) {
            syncWithEditor = settings->value(longKey).toBool();
            continue;
        }
        m_widgetSettings.insert(key, settings->value(longKey));
    }

    toggleSyncButton()->setChecked(syncWithEditor);
    if (IOutlineWidget *outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget()))
        outlineWidget->restoreSettings(m_widgetSettings);
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
    m_filterButton->setVisible(!m_filterMenu->actions().isEmpty());
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
            QVariantMap widgetSettings = outlineWidget->settings();
            for (auto iter = widgetSettings.constBegin(); iter != widgetSettings.constEnd(); ++iter)
                m_widgetSettings.insert(iter.key(), iter.value());
            removeWidget(outlineWidget);
            delete outlineWidget;
        }
        if (newWidget) {
            newWidget->restoreSettings(m_widgetSettings);
            newWidget->setCursorSynchronization(m_syncWithEditor);
            addWidget(newWidget);
            setCurrentWidget(newWidget);
            setFocusProxy(newWidget);
        }

        updateFilterMenu();
    }
}

OutlineFactory::OutlineFactory()
{
    setDisplayName(tr("Outline"));
    setId("Outline");
    setPriority(600);
}

QList<IOutlineWidgetFactory*> OutlineFactory::widgetFactories() const
{
    return m_factories;
}

void OutlineFactory::setWidgetFactories(QList<IOutlineWidgetFactory*> factories)
{
    m_factories = factories;
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

void OutlineFactory::saveSettings(QSettings *settings, int position, QWidget *widget)
{
    OutlineWidgetStack *widgetStack = qobject_cast<OutlineWidgetStack *>(widget);
    Q_ASSERT(widgetStack);
    widgetStack->saveSettings(settings, position);
}

void OutlineFactory::restoreSettings(QSettings *settings, int position, QWidget *widget)
{
    OutlineWidgetStack *widgetStack = qobject_cast<OutlineWidgetStack *>(widget);
    Q_ASSERT(widgetStack);
    widgetStack->restoreSettings(settings, position);
}

} // namespace Internal
} // namespace TextEditor
