// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    QList<QToolButton *> toolButtons();

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
    void saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(QSettings *settings, int position, QWidget *widget) override;

signals:
    void updateOutline();
};

} // namespace Internal
} // namespace TextEditor
