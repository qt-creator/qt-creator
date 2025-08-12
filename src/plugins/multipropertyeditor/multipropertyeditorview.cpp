// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "multipropertyeditorview.h"

#include "multipropertyeditoraction.h"

#include <propertyeditorview.h>
#include <qmldesignerplugin.h>

#include <uniquename.h>
#include <utils/algorithm.h>
namespace QmlDesigner {

MultiPropertyEditorView::MultiPropertyEditorView(
    class AsynchronousImageCache &imageCache, ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_imageCache{imageCache}
    , m_externalDependencies{externalDependencies}
    , m_unifiedAction(new MultiPropertyEditorAction(this))
{}

MultiPropertyEditorView::~MultiPropertyEditorView() {};

void MultiPropertyEditorView::customNotification(const AbstractView *view,
                                                 const QString &identifier,
                                                 const QList<ModelNode> &nodeList,
                                                 const QList<QVariant> &data)
{
    if (identifier == "add_extra_property_editor_widget") {
        if (data.isEmpty())
            return;
        createView(data.first().toString());
    } else if (identifier == "register_property_editor") {
        if (data.size() != 1)
            return;
        PropertyEditorView *propertyEditorView = data.first().value<PropertyEditorView *>();
        unifiedAction()->registerView(propertyEditorView);
    } else if (identifier == "unregister_property_editor") {
        if (data.size() != 1)
            return;
        PropertyEditorView *propertyEditorView = data.first().value<PropertyEditorView *>();
        unifiedAction()->unregisterView(propertyEditorView);
    } else if (identifier == "set_property_editor_target_node") {
        if (nodeList.isEmpty())
            return;

        const ModelNode &node = nodeList.first();

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
            targetView = createView(QmlDesignerPlugin::instance()
                                        ->viewManager()
                                        .propertyEditorView()
                                        ->widgetInfo()
                                        .uniqueId);
        }

        if (node != targetView->activeNode())
            targetView->setSelectedModelNode(node);
        targetView->widgetInfo().widget->setFocus();
    }
}

// Controls and reflects the collective on/off state of all other individual view actions
MultiPropertyEditorAction *MultiPropertyEditorView::unifiedAction() const
{
    return m_unifiedAction;
}

PropertyEditorView *MultiPropertyEditorView::createView(const QString &parentId)
{
    auto &viewManager = QmlDesignerPlugin::instance()->viewManager();
    auto newView = std::make_unique<PropertyEditorView>(m_imageCache, m_externalDependencies);

    WidgetInfo info = newView->widgetInfo();
    info.tabName = tr("Properties");
    info.parentId = parentId;
    newView->setWidgetInfo(info);
    newView->demoteCustomManagerRole();

    PropertyEditorView *propertyEditorView = viewManager.registerView(std::move(newView));
    unifiedAction()->registerView(propertyEditorView);
    propertyEditorView->action()->setChecked(true);
    return propertyEditorView;
}

} // namespace QmlDesigner
