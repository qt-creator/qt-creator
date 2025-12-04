// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "multipropertyeditorview.h"

#include "multipropertyeditoraction.h"

#include <propertyeditorview.h>
#include <qmldesignerplugin.h>

#include <advanceddockingsystem/dockmanager.h>
#include <uniquename.h>
#include <utils/algorithm.h>

namespace QmlDesigner {

static ViewManager &viewManager()
{
    return QmlDesignerPlugin::viewManager();
}

static void focusView(AbstractView *view)
{
    if (!view->hasWidget())
        return;

    auto widgetInfo = view->widgetInfo();
    if (auto dockManager = QmlDesignerPlugin::dockManagerForPluginInitializationOnly()) {
        if (auto dockWidget = dockManager->findDockWidget(widgetInfo.uniqueId)) {
            dockWidget->raise();
            dockWidget->setFocus();
            return;
        }
    }
    widgetInfo.widget->raise();
    widgetInfo.widget->setFocus();
}

MultiPropertyEditorView::MultiPropertyEditorView(
    class AsynchronousImageCache &imageCache, ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_imageCache{imageCache}
    , m_externalDependencies{externalDependencies}
    , m_unifiedAction(new MultiPropertyEditorAction(this))
{
    auto standardPropertyEditorView = viewManager().propertyEditorView();
    setCallbacks(standardPropertyEditorView);
}

MultiPropertyEditorView::~MultiPropertyEditorView() = default;

// Controls and reflects the collective on/off state of all other individual view actions
MultiPropertyEditorAction *MultiPropertyEditorView::unifiedAction() const
{
    return m_unifiedAction;
}

PropertyEditorView *MultiPropertyEditorView::createView(const QString &parentId)
{
    auto newView = std::make_unique<PropertyEditorView>(m_imageCache, m_externalDependencies);

    WidgetInfo info = newView->widgetInfo();
    info.tabName = tr("Properties");
    info.parentId = parentId;
    newView->setWidgetInfo(info);
    newView->demoteCustomManagerRole();

    PropertyEditorView *propertyEditorView = viewManager().registerView(std::move(newView));
    setCallbacks(propertyEditorView);
    unifiedAction()->registerView(propertyEditorView);
    propertyEditorView->action()->setChecked(true);
    return propertyEditorView;
}

void MultiPropertyEditorView::setTargetNode(const ModelNode &node)
{
    PropertyEditorView *targetView = nullptr;

    // Select any existing view that contains the node.
    // else find the first unlocked property editor.
    // else create a new property editor.
    const QList<PropertyEditorView *> propertyEditorViews = unifiedAction()->registeredViews();
    if (auto it = std::ranges::find(propertyEditorViews, node, &PropertyEditorView::activeNode);
        it != propertyEditorViews.constEnd()) {
        targetView = *it;
    } else if (auto it = std::ranges::find_if_not(
                   propertyEditorViews, &PropertyEditorView::isSelectionLocked);
               it != propertyEditorViews.constEnd()) {
        targetView = *it;
    } else {
        targetView = createView(viewManager().propertyEditorView()->widgetInfo().uniqueId);
    }

    targetView->setTargetNode(node);
    focusView(targetView);
}

void MultiPropertyEditorView::setCallbacks(PropertyEditorView *view)
{
    view->setExtraPropertyViewsCallbacks({
        .addEditor = std::bind_front(&MultiPropertyEditorView::createView, this),

        .registerEditor = std::bind_front(&MultiPropertyEditorAction::registerView, unifiedAction()),

        .unregisterEditor
        = std::bind_front(&MultiPropertyEditorAction::unregisterView, unifiedAction()),

        .setTargetNode = std::bind_front(&MultiPropertyEditorView::setTargetNode, this),
    });
}

} // namespace QmlDesigner
