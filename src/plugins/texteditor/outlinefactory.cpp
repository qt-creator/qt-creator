// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "outlinefactory.h"

#include "texteditortr.h"
#include "ioutlinewidget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <utils/utilsicons.h>
#include <utils/qtcassert.h>
#include <utils/store.h>
#include <utils/stylehelper.h>

#include <QDebug>
#include <QLabel>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>

using namespace Utils;

namespace TextEditor {

static QList<IOutlineWidgetFactory *> g_outlineWidgetFactories;
static QPointer<Internal::OutlineFactory> g_outlineFactory;

IOutlineWidgetFactory::IOutlineWidgetFactory()
{
    g_outlineWidgetFactories.append(this);
}

IOutlineWidgetFactory::~IOutlineWidgetFactory()
{
    g_outlineWidgetFactories.removeOne(this);
}

void IOutlineWidgetFactory::updateOutline()
{
    if (QTC_GUARD(!g_outlineFactory.isNull()))
        emit g_outlineFactory->updateOutline();
}

namespace Internal {

class OutlineWidgetStack : public QStackedWidget
{
    Q_OBJECT

public:
    OutlineWidgetStack(OutlineFactory *factory);
    ~OutlineWidgetStack() override;

    QList<QToolButton *> toolButtons();

    void saveSettings(Utils::QtcSettings *settings, int position);
    void restoreSettings(Utils::QtcSettings *settings, int position);

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

OutlineWidgetStack::OutlineWidgetStack(OutlineFactory *factory) :
    m_syncWithEditor(true),
    m_sorted(false)
{
    QLabel *label = new QLabel(Tr::tr("No outline available"), this);
    label->setAlignment(Qt::AlignCenter);

    // set background to be white
    label->setAutoFillBackground(true);
    label->setBackgroundRole(QPalette::Base);

    addWidget(label);

    m_toggleSync = new QToolButton(this);
    m_toggleSync->setIcon(Utils::Icons::LINK_TOOLBAR.icon());
    m_toggleSync->setCheckable(true);
    m_toggleSync->setChecked(true);
    m_toggleSync->setToolTip(Tr::tr("Synchronize with Editor"));
    connect(m_toggleSync, &QAbstractButton::clicked,
            this, &OutlineWidgetStack::toggleCursorSynchronization);

    m_filterButton = new QToolButton(this);
    Utils::StyleHelper::setPanelWidget(m_filterButton);
    // The ToolButton needs a parent because updateFilterMenu() sets
    // it visible. That would open a top-level window if the button
    // did not have a parent in that moment.

    m_filterButton->setIcon(Utils::Icons::FILTER.icon());
    m_filterButton->setToolTip(Tr::tr("Filter tree"));
    m_filterButton->setPopupMode(QToolButton::InstantPopup);
    m_filterButton->setProperty(Utils::StyleHelper::C_NO_ARROW, true);
    m_filterMenu = new QMenu(m_filterButton);
    m_filterButton->setMenu(m_filterMenu);

    m_toggleSort = new QToolButton(this);
    Utils::StyleHelper::setPanelWidget(m_toggleSort);
    m_toggleSort->setIcon(Utils::Icons::SORT_ALPHABETICALLY_TOOLBAR.icon());
    m_toggleSort->setCheckable(true);
    m_toggleSort->setChecked(false);
    m_toggleSort->setToolTip(Tr::tr("Sort Alphabetically"));
    connect(m_toggleSort, &QAbstractButton::clicked, this, &OutlineWidgetStack::toggleSort);

    connect(Core::EditorManager::instance(),
            &Core::EditorManager::currentEditorChanged,
            this,
            &OutlineWidgetStack::updateCurrentEditor);
    connect(factory, &OutlineFactory::updateOutline,
            this, &OutlineWidgetStack::updateCurrentEditor);
    updateCurrentEditor();
}

QList<QToolButton *> OutlineWidgetStack::toolButtons()
{
    return {m_filterButton, m_toggleSort, m_toggleSync};
}

OutlineWidgetStack::~OutlineWidgetStack() = default;

void OutlineWidgetStack::saveSettings(QtcSettings *settings, int position)
{
    const Key baseKey = numberedKey("Outline.", position) + '.';
    settings->setValue(baseKey + "SyncWithEditor", m_toggleSync->isChecked());
    for (auto iter = m_widgetSettings.constBegin(); iter != m_widgetSettings.constEnd(); ++iter)
        settings->setValue(baseKey + keyFromString(iter.key()), iter.value());
}

void OutlineWidgetStack::restoreSettings(Utils::QtcSettings *settings, int position)
{
    const Key baseKey = numberedKey("Outline.", position) + '.';
    const QString baseKeyString = stringFromKey(baseKey);

    bool syncWithEditor = true;
    m_widgetSettings.clear();
    const QStringList longKeys = settings->allKeys();
    for (const QString &longKey : longKeys) {
        if (!longKey.startsWith(baseKeyString))
            continue;

        const QString key = longKey.mid(baseKeyString.length());

        if (key == QLatin1String("SyncWithEditor")) {
            syncWithEditor = settings->value(keyFromString(longKey)).toBool();
            continue;
        }
        m_widgetSettings.insert(key, settings->value(keyFromString(longKey)));
    }

    m_toggleSync->setChecked(syncWithEditor);
    if (auto outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget()))
        outlineWidget->restoreSettings(m_widgetSettings);
}

bool OutlineWidgetStack::isCursorSynchronized() const
{
    return m_syncWithEditor;
}

void OutlineWidgetStack::toggleCursorSynchronization()
{
    m_syncWithEditor = !m_syncWithEditor;
    if (auto outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget()))
        outlineWidget->setCursorSynchronization(m_syncWithEditor);
}

void OutlineWidgetStack::toggleSort()
{
    m_sorted = !m_sorted;
    if (auto outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget()))
        outlineWidget->setSorted(m_sorted);
}

void OutlineWidgetStack::updateFilterMenu()
{
    m_filterMenu->clear();
    if (auto outlineWidget = qobject_cast<IOutlineWidget *>(currentWidget())) {
        const QList<QAction *> filterActions = outlineWidget->filterMenuActions();
        for (QAction *filterAction : filterActions)
            m_filterMenu->addAction(filterAction);
    }
    m_filterButton->setVisible(!m_filterMenu->actions().isEmpty());
}

void OutlineWidgetStack::updateCurrentEditor()
{
    updateEditor(Core::EditorManager::currentEditor());
}

void OutlineWidgetStack::updateEditor(Core::IEditor *editor)
{
    IOutlineWidget *newWidget = nullptr;

    if (editor) {
        for (IOutlineWidgetFactory *widgetFactory : std::as_const(g_outlineWidgetFactories)) {
            if (widgetFactory->supportsEditor(editor)) {
                newWidget = widgetFactory->createWidget(editor);
                m_toggleSort->setVisible(widgetFactory->supportsSorting());
                break;
            }
        }
    }

    if (newWidget != currentWidget()) {
        // delete old widget
        if (auto outlineWidget = qobject_cast<IOutlineWidget*>(currentWidget())) {
            QVariantMap widgetSettings = outlineWidget->settings();
            for (auto iter = widgetSettings.constBegin(); iter != widgetSettings.constEnd(); ++iter)
                m_widgetSettings.insert(iter.key(), iter.value());
            removeWidget(outlineWidget);
            delete outlineWidget;
        }
        if (newWidget) {
            newWidget->restoreSettings(m_widgetSettings);
            newWidget->setCursorSynchronization(m_syncWithEditor);
            m_toggleSort->setChecked(newWidget->isSorted());
            addWidget(newWidget);
            setCurrentWidget(newWidget);
            setFocusProxy(newWidget);
        }

        updateFilterMenu();
    }
}

OutlineFactory::OutlineFactory()
{
    QTC_CHECK(g_outlineFactory.isNull());
    g_outlineFactory = this;
    setDisplayName(Tr::tr("Outline"));
    setId("Outline");
    setPriority(600);
}

Core::NavigationView OutlineFactory::createWidget()
{
    auto placeHolder = new OutlineWidgetStack(this);
    return {placeHolder, placeHolder->toolButtons()};
}

void OutlineFactory::saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget)
{
    auto widgetStack = qobject_cast<OutlineWidgetStack *>(widget);
    Q_ASSERT(widgetStack);
    widgetStack->saveSettings(settings, position);
}

void OutlineFactory::restoreSettings(Utils::QtcSettings *settings, int position, QWidget *widget)
{
    auto widgetStack = qobject_cast<OutlineWidgetStack *>(widget);
    Q_ASSERT(widgetStack);
    widgetStack->restoreSettings(settings, position);
}

} // namespace Internal
} // namespace TextEditor

#include "outlinefactory.moc"
