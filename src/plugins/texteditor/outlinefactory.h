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

#pragma once

#include <texteditor/ioutlinewidget.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <QStackedWidget>
#include <QMenu>

namespace Core { class IEditor; }

namespace TextEditor {
namespace Internal {

class OutlineFactory;

class OutlineWidgetStack : public QStackedWidget
{
    Q_OBJECT
public:
    OutlineWidgetStack(OutlineFactory *factory);
    ~OutlineWidgetStack() override;

    QToolButton *toggleSyncButton();
    QToolButton *filterButton();
    QToolButton *sortButton();

    void saveSettings(QSettings *settings, int position);
    void restoreSettings(QSettings *settings, int position);

private:
    bool isCursorSynchronized() const;
    QWidget *dummyWidget() const;
    void updateFilterMenu();
    void toggleCursorSynchronization();
    void toggleSort();
    void updateEditor(Core::IEditor *editor);
    void updateCurrentEditor();

    QStackedWidget *m_widgetStack;
    OutlineFactory *m_factory;
    QToolButton *m_toggleSync;
    QToolButton *m_filterButton;
    QToolButton *m_toggleSort;
    QMenu *m_filterMenu;
    QVariantMap m_widgetSettings;
    bool m_syncWithEditor;
    bool m_sorted;
};

class OutlineFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    OutlineFactory();

    // from INavigationWidgetFactory
    Core::NavigationView createWidget() override;
    void saveSettings(QSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(QSettings *settings, int position, QWidget *widget) override;

signals:
    void updateOutline();
};

} // namespace Internal
} // namespace TextEditor
