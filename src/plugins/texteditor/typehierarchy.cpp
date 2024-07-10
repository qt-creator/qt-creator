// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typehierarchy.h"

#include "texteditorconstants.h"
#include "texteditortr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <utils/utilsicons.h>

#include <QLabel>
#include <QPalette>
#include <QStackedWidget>
#include <QToolButton>

using namespace Utils;

namespace TextEditor {
namespace Internal {

static QList<TypeHierarchyWidgetFactory *> g_widgetFactories;

class TypeHierarchyFactory : public Core::INavigationWidgetFactory
{
public:
    TypeHierarchyFactory()
    {
        setDisplayName(Tr::tr("Type Hierarchy"));
        setPriority(649);
        setId(Constants::TYPE_HIERARCHY_FACTORY_ID);
    }

private:
    Core::NavigationView createWidget() override;
};

class TypeHierarchyWidgetStack : public QStackedWidget
{
    Q_OBJECT

public:
    TypeHierarchyWidgetStack();

    void reload();
};

static TypeHierarchyFactory &typeHierarchyFactory()
{
    static TypeHierarchyFactory theFactory;
    return theFactory;
}

void setupTypeHierarchyFactory()
{
    (void) typeHierarchyFactory();
}

TypeHierarchyWidgetStack::TypeHierarchyWidgetStack()
{
    QLabel *label = new QLabel(Tr::tr("No type hierarchy available"), this);
    label->setAlignment(Qt::AlignCenter);

    label->setAutoFillBackground(true);
    label->setBackgroundRole(QPalette::Base);

    addWidget(label);
    reload();
}

void TypeHierarchyWidgetStack::reload()
{
    const auto editor = Core::EditorManager::currentEditor();
    TypeHierarchyWidget *newWidget = nullptr;

    if (editor) {
        for (TypeHierarchyWidgetFactory * const widgetFactory : std::as_const(g_widgetFactories)) {
            if ((newWidget = widgetFactory->createWidget(editor)))
                break;
        }
    }

    QWidget * const current = currentWidget();
    if (current) {
        removeWidget(current);
        current->deleteLater();
    }
    if (newWidget) {
        addWidget(newWidget);
        setCurrentWidget(newWidget);
        setFocusProxy(newWidget);
        newWidget->reload();
    }
}

Core::NavigationView TypeHierarchyFactory::createWidget()
{
    const auto placeholder = new TypeHierarchyWidgetStack;
    const auto reloadButton = new QToolButton;
    reloadButton->setIcon(Icons::RELOAD_TOOLBAR.icon());
    reloadButton->setToolTip(Tr::tr("Reloads the type hierarchy for the symbol under the cursor."));
    connect(reloadButton, &QToolButton::clicked, placeholder, &TypeHierarchyWidgetStack::reload);
    return {placeholder, {reloadButton}};
}

void updateTypeHierarchy(QWidget *widget)
{
    if (const auto w = qobject_cast<TypeHierarchyWidgetStack *>(widget))
        w->reload();
}

} // namespace Internal

TypeHierarchyWidgetFactory::TypeHierarchyWidgetFactory()
{
    Internal::g_widgetFactories.append(this);
}

TypeHierarchyWidgetFactory::~TypeHierarchyWidgetFactory()
{
    Internal::g_widgetFactories.removeOne(this);
}

void openTypeHierarchy()
{
    if (const auto action = Core::ActionManager::command(Constants::OPEN_TYPE_HIERARCHY)->action())
        action->trigger();
}

} // namespace TextEditor

#include <typehierarchy.moc>
